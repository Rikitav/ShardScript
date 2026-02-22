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

    ByteCodeDecoder decoder(program.EntryPoint->ExecutableByteCode);

    out << L"; --- DISASSEMBLY START ---\n";
    out << L"; Method: " << program.EntryPoint->Name << L"\n\n";

    while (!decoder.IsEOF())
    {
        size_t currentOffset = decoder.Index();

        out << L"SS_"
            << std::hex << std::uppercase << std::setw(4) << std::setfill(L'0')
            << currentOffset
            << L": "
            << std::dec;

        OpCode op = decoder.AbsorbOpCode();
        switch (op)
        {
            case OpCode::Nop:                       out << L"nop"; break;
            case OpCode::Halt:                      out << L"halt"; break;
            case OpCode::PopStack:                  out << L"pop"; break;
            case OpCode::LoadConst_Null:            out << L"ldnull"; break;
            case OpCode::Return:                    out << L"ret"; break;

            case OpCode::Math_Addition:             out << L"add"; break;
            case OpCode::Math_Substraction:         out << L"sub"; break;
            case OpCode::Math_Multiplication:       out << L"mul"; break;
            case OpCode::Math_Division:             out << L"div"; break;
            case OpCode::Math_Module:               out << L"mod"; break;
            case OpCode::Math_Power:                out << L"pow"; break;

            case OpCode::Compare_Equal:             out << L"cmp_eq"; break;
            case OpCode::Compare_NotEqual:          out << L"cmp_neq"; break;
            case OpCode::Compare_Less:              out << L"cmp_l"; break;
            case OpCode::Compare_LessOrEqual:       out << L"cmp_le"; break;
            case OpCode::Compare_Greater:           out << L"cmp_gt"; break;
            case OpCode::Compare_GreaterOrEqual:    out << L"cmp_ge"; break;

            case OpCode::Logical_Not:               out << L"not"; break;
            case OpCode::Logical_And:               out << L"and"; break;
            case OpCode::Logical_Or:                out << L"or"; break;

            case OpCode::LoadArrayElement:          out << L"ldelem"; break;
            case OpCode::StoreArrayElement:         out << L"stelem"; break;

            case OpCode::CreateDuplicate:           out << "dup"; break;

            case OpCode::LoadConst_Boolean:
                out << L"ldc.bool " << (decoder.AbsorbBoolean() ? L"true" : L"false");
                break;

            case OpCode::LoadConst_Integer64:
                out << L"ldc.i8 " << decoder.AbsorbInt64();
                break;

            case OpCode::LoadConst_Rational64:
                out << L"ldc.r8 " << decoder.AbsorbDouble64();
                break;

            case OpCode::LoadConst_Char:
                out << L"ldc.char '" << (char)decoder.AbsorbChar16() << L"'";
                break;

            case OpCode::LoadConst_String:
            {
                size_t strIdx = decoder.AbsorbString();
                out << L"ldstr pool[" << strIdx << L"]";
                // out << L" \"" << program.DataSection[strIdx] << L"\"";
                break;
            }

            case OpCode::LoadVariable:
                out << L"ldloc " << decoder.AbsorbVariableSlot();
                break;

            case OpCode::StoreVariable:
                out << L"stloc " << decoder.AbsorbVariableSlot();
                break;

            case OpCode::Jump:
                out << L"jmp SS_" << std::hex << std::setw(4) << std::setfill(L'0') << decoder.AbsorbJump();
                break;

            case OpCode::Jump_False:
                out << L"jmpf SS_" << std::hex << std::setw(4) << std::setfill(L'0') << decoder.AbsorbJump();
                break;

            case OpCode::Jump_True:
                out << L"jmpt SS_" << std::hex << std::setw(4) << std::setfill(L'0') << decoder.AbsorbJump();
                break;

            case OpCode::CallMethodSymbol:
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

            case OpCode::NewObject:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                if (sym == nullptr)
                {
                    out << L"newobj null";
                    break;
                }

                std::wstring name = sym->FullName.size() > 0 ? sym->FullName : sym->Name;
                out << L"newobj " << name;
                break;
            }

            case OpCode::LoadField:
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

            case OpCode::StoreField:
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

            case OpCode::LoadStaticField:
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

            case OpCode::StoreStaticField:
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

            case OpCode::NewArray:
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

            default:
            {
                out << L"unknown (0x" << std::hex << static_cast<uint16_t>(op) << L")";
                break;
            }
        }

        out << L"\n";
    }

    out << L"; --- DISASSEMBLY END ---\n";
}
