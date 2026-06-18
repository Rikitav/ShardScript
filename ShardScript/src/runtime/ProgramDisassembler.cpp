#include <shard/runtime/ProgramDisassembler.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>

#include <iomanip>
#include <iostream>
#include <functional>

using namespace shard;

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
    out << L"; --- DISASSEMBLY START ---\n";
    out << L"; Method: " << method->FullName << L"\n\n";

    if (method->HandleType == MethodHandleType::External)
    {
		out << L"; Method " << method->FullName << L" is an external method. No bytecode to disassemble.\n";
        out << L"; --- DISASSEMBLY END ---\n";
		return;
    }

    ByteCodeDecoder decoder(method->ExecutableByteCode);
    while (!decoder.IsEOF())
    {
        std::size_t currentOffset = decoder.Index();

        out << L"SS_"
            << std::hex << std::uppercase << std::setw(4) << std::setfill(L'0')
            << currentOffset
            << L": "
            << std::dec;

        OpCode op = decoder.AbsorbOpCode();
        switch (op)
        {
            case OpCode::NOP:                       out << L"nop"; break;
            case OpCode::HALT:                      out << L"halt"; break;
            case OpCode::POPSTACK:                  out << L"pop"; break;
            case OpCode::LOADCONST_NULL:            out << L"ldnull"; break;
            case OpCode::RETURN:                    out << L"ret"; break;

            case OpCode::MATH_ADDITION:             out << L"add"; break;
            case OpCode::MATH_SUBSTRACTION:         out << L"sub"; break;
            case OpCode::MATH_MULTIPLICATION:       out << L"mul"; break;
            case OpCode::MATH_DIVISION:             out << L"div"; break;
            case OpCode::MATH_MODULE:               out << L"mod"; break;
            case OpCode::MATH_POWER:                out << L"pow"; break;

            case OpCode::COMPARE_EQUAL:             out << L"cmp_eq"; break;
            case OpCode::COMPARE_NOTEQUAL:          out << L"cmp_neq"; break;
            case OpCode::COMPARE_LESS:              out << L"cmp_l"; break;
            case OpCode::COMPARE_LESSOREQUAL:       out << L"cmp_le"; break;
            case OpCode::COMPARE_GREATER:           out << L"cmp_gt"; break;
            case OpCode::COMPARE_GREATEROREQUAL:    out << L"cmp_ge"; break;

            case OpCode::LOGICAL_NOT:               out << L"not"; break;
            case OpCode::LOGICAL_AND:               out << L"and"; break;
            case OpCode::LOGICAL_OR:                out << L"or"; break;

            case OpCode::LOADARRAYELEMENT:          out << L"ldelem"; break;
            case OpCode::STOREARRAYELEMENT:         out << L"stelem"; break;

            case OpCode::CREATEDUPLICATE:           out << "dup"; break;

            case OpCode::LOADCONST_BOOLEAN:
                out << L"ldc.bool " << (decoder.AbsorbBoolean() ? L"true" : L"false");
                break;

            case OpCode::LOADCONST_INTEGER64:
                out << L"ldc.i8 " << decoder.AbsorbInt64();
                break;

            case OpCode::LOADCONST_RATIONAL64:
                out << L"ldc.r8 " << decoder.AbsorbDouble64();
                break;

            case OpCode::LOADCONST_CHAR:
                out << L"ldc.char '" << (char)decoder.AbsorbChar16() << L"'";
                break;

            case OpCode::LOADCONST_STRING:
            {
                std::size_t strIdx = decoder.AbsorbString();
                out << L"ldstr pool[" << strIdx << L"]";
                // out << L" \"" << program.DataSection[strIdx] << L"\"";
                break;
            }

            case OpCode::LOADVARIABLE:
                out << L"ldloc " << decoder.AbsorbVariableSlot();
                break;

            case OpCode::STOREVARIABLE:
                out << L"stloc " << decoder.AbsorbVariableSlot();
                break;

            case OpCode::JUMP:
                out << L"jmp SS_" << std::hex << std::setw(4) << std::setfill(L'0') << decoder.AbsorbJump();
                break;

            case OpCode::JUMP_FALSE:
                out << L"jmpf SS_" << std::hex << std::setw(4) << std::setfill(L'0') << decoder.AbsorbJump();
                break;

            case OpCode::JUMP_TRUE:
                out << L"jmpt SS_" << std::hex << std::setw(4) << std::setfill(L'0') << decoder.AbsorbJump();
                break;

            case OpCode::CALLMETHODSYMBOL:
            {
                auto* sym = decoder.AbsorbMethodSymbol();
                if (sym == nullptr)
                {
                    out << L"call null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"call " << name;
                break;
            }

            case OpCode::NEWOBJECT:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                auto* ctor = decoder.AbsorbConstructorSymbol();

                if (sym == nullptr)
                {
                    out << L"newobj null";
                    break;
                }

                std::wstring symName = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"newobj " << symName; // << L" " << ctor->Name;
                break;
            }

            case OpCode::LOADFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                if (sym == nullptr)
                {
                    out << L"ldfld null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"ldfld " << name;
                break;
            }

            case OpCode::STOREFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                if (sym == nullptr)
                {
                    out << L"stfld null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"stfld " << name;
                break;
            }

            case OpCode::LOADSTATICFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                if (sym == nullptr)
                {
                    out << L"ldsfld null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"ldsfld " << name;
                break;
            }

            case OpCode::STORESTATICFIELD:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                if (sym == nullptr)
                {
                    out << L"stsfld null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"stsfld " << name;
                break;
            }

            case OpCode::NEWARRAY:
            {
                auto* sym = decoder.AbsorbArraySymbol();
                if (sym == nullptr)
                {
                    out << L"newarr null";
                    break;
                }

                if (sym->UnderlayingType == nullptr)
                {
                    out << L"newarr null[]";
                    break;
                }

                std::wstring name = sym->UnderlayingType->FullName.size() > 0 ? sym->UnderlayingType->FullName : sym->Name;
                out << L"newarr " << name << "[]";
                break;
            }

            case OpCode::NEWDYNAMICARRAY:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                if (sym == nullptr)
                {
                    out << L"newdynarr null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"newdynarr " << name;
                break;
            }

            case OpCode::ARRAYLENGTH:
            {
                out << L"ldlen";
                break;
            }

            case OpCode::LOAD_TYPEARGUMENT:
            {
                std::uint16_t index = decoder.AbsorbUInt16();
				auto* sym = decoder.AbsorbTypeSymbol();

                out << L"ldtypearg " << index;
				if (sym != nullptr)
				{
					std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
					out << L" (" << name << L")";
				}

                break;
            }

            case OpCode::CALLINTERFACE:
            {
                auto* sym = decoder.AbsorbMethodSymbol();
                if (sym == nullptr)
                {
                    out << L"callinterface null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"callinterface " << name;
                break;
            }

            case OpCode::ISINSTANCE:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                if (sym == nullptr)
                {
                    out << L"isinst null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"isinst " << name;
                break;
            }

            case OpCode::CASTINTERFACE:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                if (sym == nullptr)
                {
                    out << L"castinterface null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"castinterface " << name;
                break;
            }

            case OpCode::THROW:
            {
                out << L"throw";
                break;
            }

            case OpCode::ENTER_TRY:
            {
                out << L"enter_try SS_" << std::hex << std::setw(4) << std::setfill(L'0') << decoder.AbsorbJump();
                break;
            }

            case OpCode::LEAVE_TRY:
            {
                out << L"leave_try";
                break;
            }

            case OpCode::RETHROW:
            {
                out << L"rethrow";
                break;
            }

            case OpCode::END_CATCH:
            {
                out << L"end_catch";
                break;
            }

            default:
            {
                out << L"unknown (0x" << std::hex << static_cast<std::uint16_t>(op) << L")";
                break;
            }
        }

        out << L"\n";
    }

    out << L"; --- DISASSEMBLY END ---\n";
}
