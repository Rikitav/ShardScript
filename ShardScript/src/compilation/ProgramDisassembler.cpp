#include <shard/compilation/ProgramDisassembler.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/TypeSymbol.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace shard;

namespace
{
    struct Colorizer
    {
        bool enabled = false;

        const wchar_t* reset = L"\x1B[0m";
        const wchar_t* bold = L"\x1B[1m";
        const wchar_t* dim = L"\x1B[2m";
        const wchar_t* gray = L"\x1B[90m";
        const wchar_t* red = L"\x1B[91m";
        const wchar_t* green = L"\x1B[92m";
        const wchar_t* yellow = L"\x1B[93m";
        const wchar_t* blue = L"\x1B[94m";
        const wchar_t* magenta = L"\x1B[95m";
        const wchar_t* cyan = L"\x1B[96m";
        const wchar_t* white = L"\x1B[97m";

        explicit Colorizer(std::wostream& out)
            : enabled(out.rdbuf() == std::wcout.rdbuf())
        {
        }

        void apply(std::wostream& out, const wchar_t* code) const
        {
            if (enabled)
                out << code;
        }

        void reset_color(std::wostream& out) const
        {
            if (enabled)
                out << reset;
        }
    };

    static std::wstring FormatAccess(MethodSymbol* method)
    {
        std::wstring result;
        if (method->Accesibility == SymbolAccesibility::Public)
            result += L"public";
        else
            result += L"private";

        if (method->Linking == LINK_STATIC)
            result += L" static";
        else
            result += L" instance";

        return result;
    }

    static std::wstring FormatTypeName(TypeSymbol* type)
    {
        if (type == nullptr)
            return L"<unknown>";
        return type->FullName.empty() ? type->Name : type->FullName;
    }

    static std::wstring FormatParameters(MethodSymbol* method)
    {
        if (method->Parameters.empty())
            return L"()";

        std::wostringstream oss;
        oss << L"(";
        for (std::size_t i = 0; i < method->Parameters.size(); ++i)
        {
            if (i > 0)
                oss << L", ";

            ParameterSymbol* param = method->Parameters[i];
            if (param == nullptr)
            {
                oss << L"<unknown>";
                continue;
            }

            oss << FormatTypeName(param->Type);
            if (!param->Name.empty())
                oss << L" " << param->Name;
        }
        oss << L")";
        return oss.str();
    }

    static std::wstring FormatHandleType(MethodHandleType type)
    {
        switch (type)
        {
            case MethodHandleType::Body: return L"body";
            case MethodHandleType::External: return L"external";
            case MethodHandleType::Lambda: return L"lambda";
            default: return L"none";
        }
    }

    static std::wstring FormatParent(MethodSymbol* method)
    {
        if (method->Parent == nullptr)
            return L"<global>";
        return method->Parent->FullName.empty() ? method->Parent->Name : method->Parent->FullName;
    }

    static void WriteHexOffset(std::wostream& out, std::size_t offset)
    {
        auto oldFlags = out.flags();
        auto oldFill = out.fill(L'0');
        out << std::hex << std::uppercase << std::setw(4) << offset;
        out.fill(oldFill);
        out.flags(oldFlags);
    }

    static void WriteHexTarget(std::wostream& out, std::size_t target)
    {
        auto oldFlags = out.flags();
        auto oldFill = out.fill(L'0');
        out << std::hex << std::uppercase << std::setw(4) << target;
        out.fill(oldFill);
        out.flags(oldFlags);
    }

    static void WriteMethodHeader(std::wostream& out, MethodSymbol* method, const Colorizer& color)
    {
        color.apply(out, color.bold);
        color.apply(out, color.cyan);
        out << L"; ============================================================\n";
        out << L";  DISASSEMBLY: ";
        color.apply(out, color.yellow);
        out << (method->FullName.empty() ? method->Name : method->FullName);
        color.apply(out, color.cyan);
        out << L"\n";
        out << L"; ============================================================\n";
        color.reset_color(out);

        auto write_line = [&](const wchar_t* label, const std::wstring& value)
        {
            color.apply(out, color.gray);
            out << L";  ";
            color.apply(out, color.green);
            out << std::setw(12) << std::left << label;
            color.reset_color(out);
            out << L" ";
            color.apply(out, color.yellow);
            out << value;
            color.reset_color(out);
            out << L"\n";
        };

        write_line(L"Handle:", FormatHandleType(method->HandleType));
        write_line(L"Access:", FormatAccess(method));
        write_line(L"Return:", FormatTypeName(method->ReturnType));
        write_line(L"Parameters:", FormatParameters(method));
        write_line(L"Declared:", FormatParent(method));

        if (method->HandleType == MethodHandleType::Body)
        {
            write_line(L"Locals:", std::to_wstring(method->GetEvalStackLocalsCount()));
            write_line(L"Variables:", std::to_wstring(method->GetEvalStackVariablesCount()));
        }

        color.apply(out, color.cyan);
        out << L"; ============================================================\n";
        color.reset_color(out);

        out << L"\n";
    }
}

void ProgramDisassembler::Disassemble(std::wostream& out, ProgramVirtualImage& program)
{
    if (!program.EntryPoint)
    {
        out << L"; Error: No EntryPoint found in ProgramVirtualImage.\n";
        return;
    }

    Disassemble(out, program.EntryPoint);
}

void ProgramDisassembler::Disassemble(std::wostream& out, MethodSymbol* method)
{
    Colorizer color(out);

    WriteMethodHeader(out, method, color);

    if (method->HandleType == MethodHandleType::External)
    {
        color.apply(out, color.dim);
        out << L"; Method " << method->FullName << L" is an external method. No bytecode to disassemble.\n";
        color.reset_color(out);
        out << L"\n";
        color.apply(out, color.gray);
        out << L"; --- DISASSEMBLY END ---\n";
        color.reset_color(out);
        return;
    }

    if (method->ExecutableByteCode.empty())
    {
        color.apply(out, color.dim);
        out << L"; No bytecode available for this method.\n";
        color.reset_color(out);
        out << L"\n";
        color.apply(out, color.gray);
        out << L"; --- DISASSEMBLY END ---\n";
        color.reset_color(out);
        return;
    }

    color.apply(out, color.gray);
    out << L"; Offset           Opcode Arguments\n";
    out << L"; -------- -------------- -----------------------------------------\n";
    color.reset_color(out);

    ByteCodeDecoder decoder(method->ExecutableByteCode);
    while (!decoder.IsEOF())
    {
        std::size_t currentOffset = decoder.Index();

        std::wostringstream line;
        line << L"SS_";
        WriteHexOffset(line, currentOffset);
        line << L":";

        OpCode op = decoder.AbsorbOpCode();
        std::wstring opcode;
        std::wostringstream args;

        switch (op)
        {
            case OpCode::NOP:                       opcode = L"nop"; break;
            case OpCode::HALT:                      opcode = L"halt"; break;
            case OpCode::POPSTACK:                  opcode = L"pop"; break;
            case OpCode::POPSTACK_N:
                opcode = L"pop.n";
                args << static_cast<int>(decoder.AbsorbUInt8());
                break;
            case OpCode::LOADCONST_NULL:            opcode = L"ldnull"; break;
            case OpCode::RETURN:                    opcode = L"ret"; break;

            case OpCode::MATH_ADDITION:             opcode = L"add"; break;
            case OpCode::MATH_SUBSTRACTION:         opcode = L"sub"; break;
            case OpCode::MATH_MULTIPLICATION:       opcode = L"mul"; break;
            case OpCode::MATH_DIVISION:             opcode = L"div"; break;
            case OpCode::MATH_MODULE:               opcode = L"mod"; break;
            case OpCode::MATH_POWER:                opcode = L"pow"; break;
            case OpCode::MATH_NEGATIVE:             opcode = L"neg"; break;
            case OpCode::MATH_POSITIVE:             opcode = L"pos"; break;
            case OpCode::MATH_LEFTSHIFT:            opcode = L"shl"; break;
            case OpCode::MATH_RIGHTSHIFT:           opcode = L"shr"; break;

            case OpCode::COMPARE_EQUAL:             opcode = L"cmp_eq"; break;
            case OpCode::COMPARE_NOTEQUAL:          opcode = L"cmp_neq"; break;
            case OpCode::COMPARE_LESS:              opcode = L"cmp_l"; break;
            case OpCode::COMPARE_LESSOREQUAL:       opcode = L"cmp_le"; break;
            case OpCode::COMPARE_GREATER:           opcode = L"cmp_gt"; break;
            case OpCode::COMPARE_GREATEROREQUAL:    opcode = L"cmp_ge"; break;

            case OpCode::LOGICAL_NOT:               opcode = L"not"; break;
            case OpCode::LOGICAL_AND:               opcode = L"and"; break;
            case OpCode::LOGICAL_OR:                opcode = L"or"; break;

            case OpCode::LOADARRAYELEMENT:          opcode = L"ldelem"; break;
            case OpCode::STOREARRAYELEMENT:         opcode = L"stelem"; break;

            case OpCode::CREATEDUPLICATE:           opcode = L"dup"; break;

            case OpCode::LOADCONST_BOOLEAN:
                opcode = L"ldc.bool";
                args << (decoder.AbsorbBoolean() ? L"true" : L"false");
                break;

            case OpCode::LOADCONST_INTEGER64:
                opcode = L"ldc.i8";
                args << decoder.AbsorbInt64();
                break;

            case OpCode::LOADCONST_RATIONAL64:
                opcode = L"ldc.r8";
                args << decoder.AbsorbDouble64();
                break;

            case OpCode::LOADCONST_CHAR:
            {
                opcode = L"ldc.char";
                wchar_t ch = decoder.AbsorbChar16();
                args << L"'" << ch << L"'";
                break;
            }

            case OpCode::LOADCONST_STRING:
            {
                std::size_t strIdx = decoder.AbsorbString();
                opcode = L"ldstr";
                args << L"pool[" << strIdx << L"]";
                break;
            }

            case OpCode::LOADVARIABLE:
                opcode = L"ldloc";
                args << decoder.AbsorbVariableSlot();
                break;

            case OpCode::STOREVARIABLE:
                opcode = L"stloc";
                args << decoder.AbsorbVariableSlot();
                break;

            case OpCode::JUMP:
            {
                opcode = L"jmp";
                args << L"SS_";
                WriteHexTarget(args, decoder.AbsorbJump());
                break;
            }

            case OpCode::JUMP_FALSE:
            {
                opcode = L"jmpf";
                args << L"SS_";
                WriteHexTarget(args, decoder.AbsorbJump());
                break;
            }

            case OpCode::JUMP_TRUE:
            {
                opcode = L"jmpt";
                args << L"SS_";
                WriteHexTarget(args, decoder.AbsorbJump());
                break;
            }

            case OpCode::CALLMETHODSYMBOL:
            {
                auto* sym = decoder.AbsorbMethodSymbol();
                opcode = L"call";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::CALLGENERICMETHOD:
            {
                auto* sym = decoder.AbsorbMethodSymbol();
                opcode = L"callgeneric";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::CALLDELEGATE:
                opcode = L"calldelegate";
                break;

            case OpCode::NEWDELEGATE:
            {
                auto* sym = decoder.AbsordDelegateTypeSymbol();
                opcode = L"newdelegate";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::NEWOBJECT:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                decoder.AbsorbConstructorSymbol();
                opcode = L"newobj";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::LOADFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                opcode = L"ldfld";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::STOREFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                opcode = L"stfld";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::LOADSTATICFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                opcode = L"ldsfld";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::LOADENUMFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                opcode = L"ldenum";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::STORESTATICFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                opcode = L"stsfld";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::NEWARRAY:
            {
                auto* sym = decoder.AbsorbArraySymbol();
                opcode = L"newarr";
                if (sym == nullptr || sym->UnderlayingType == nullptr)
                    args << L"null[]";
                else
                    args << (sym->UnderlayingType->FullName.empty() ? sym->UnderlayingType->Name : sym->UnderlayingType->FullName) << L"[]";
                break;
            }

            case OpCode::NEWDYNAMICARRAY:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                opcode = L"newdynarr";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::ARRAYLENGTH:
                opcode = L"ldlen";
                break;

            case OpCode::CREATERANGE:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                opcode = L"newrange";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::LOAD_TYPEARGUMENT:
            {
                std::uint16_t index = decoder.AbsorbUInt16();
                auto* sym = decoder.AbsorbTypeSymbol();
                opcode = L"ldtypearg";
                args << index;
                if (sym != nullptr)
                    args << L" (" << (sym->FullName.empty() ? sym->Name : sym->FullName) << L")";
                break;
            }

            case OpCode::CALLINTERFACE:
            {
                auto* sym = decoder.AbsorbMethodSymbol();
                opcode = L"callinterface";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::ISINSTANCE:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                opcode = L"isinst";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::CASTINTERFACE:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                opcode = L"castinterface";
                if (sym == nullptr)
                    args << L"null";
                else
                    args << (sym->FullName.empty() ? sym->Name : sym->FullName);
                break;
            }

            case OpCode::THROW:
                opcode = L"throw";
                break;

            case OpCode::ENTER_TRY:
            {
                opcode = L"enter_try";
                args << L"SS_";
                WriteHexTarget(args, decoder.AbsorbJump());
                break;
            }

            case OpCode::LEAVE_TRY:
                opcode = L"leave_try";
                break;

            case OpCode::RETHROW:
                opcode = L"rethrow";
                break;

            case OpCode::END_CATCH:
                opcode = L"end_catch";
                break;

            default:
            {
                opcode = L"unknown";
                args << L"0x" << std::hex << static_cast<std::uint16_t>(op);
                break;
            }
        }

        color.apply(out, color.gray);
        out << std::setw(9) << std::left << line.str();
        color.reset_color(out);

        color.apply(out, color.cyan);
        out << std::setw(16) << std::right << opcode << " ";
        color.reset_color(out);

        if (!args.str().empty())
        {
            color.apply(out, color.yellow);
            out << args.str();
            color.reset_color(out);
        }

        out << L"\n";
    }

    out << L"\n";
    color.apply(out, color.gray);
    out << L"; --- DISASSEMBLY END ---\n";
    color.reset_color(out);
}
