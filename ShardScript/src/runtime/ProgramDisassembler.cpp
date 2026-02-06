#include <shard/runtime/ProgramDisassembler.h>
#include <shard/compilation/ByteCodeDecoder.h>
#include <shard/compilation/OperationCode.h>

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
            case OpCode::Nop:               out << L"nop"; break;
            case OpCode::Halt:              out << L"halt"; break;
            case OpCode::PopStack:          out << L"pop"; break;
            case OpCode::LoadConst_Null:    out << L"ldnull"; break;
            case OpCode::Return:            out << L"ret"; break;

            case OpCode::Math_Addition:        out << L"add"; break;
            case OpCode::Math_Substraction:    out << L"sub"; break;
            case OpCode::Math_Multiplication:  out << L"mul"; break;
            case OpCode::Math_Division:        out << L"div"; break;
            case OpCode::Math_Module:          out << L"mod"; break;
            case OpCode::Math_Power:           out << L"pow"; break;

            case OpCode::Compare_Equal:          out << L"ceq"; break;
            case OpCode::Compare_NotEqual:       out << L"cne"; break;
            case OpCode::Compare_Less:           out << L"clt"; break;
            case OpCode::Compare_LessOrEqual:    out << L"cle"; break;
            case OpCode::Compare_Greater:        out << L"cgt"; break;
            case OpCode::Compare_GreaterOrEqual: out << L"cge"; break;
            case OpCode::Compare_Not:            out << L"not"; break;

            case OpCode::LoadArrayElement:  out << L"ldelem"; break;
            case OpCode::StoreArrayElement: out << L"stelem"; break;

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
                out << L"call " << (sym ? (sym->Name) : L"null");
                break;
            }

            case OpCode::NewObject:
            {
                auto* sym = decoder.AbsorbTypeSymbol();
                out << L"newobj " << (sym ? sym->Name : L"null");
                break;
            }

            case OpCode::LoadField:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                out << L"ldfld " << (sym ? sym->Name : L"null");
                break;
            }

            case OpCode::StoreField:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                out << L"stfld " << (sym ? sym->Name : L"null");
                break;
            }

            case OpCode::LoadStaticField:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                out << L"ldsfld " << (sym ? sym->Name : L"null");
                break;
            }

            case OpCode::StoreStaticField:
            {
                auto* sym = decoder.AbsorbFieldSymbol();
                out << L"stsfld " << (sym ? sym->Name : L"null");
                break;
            }

            case OpCode::NewArray:
            {
                auto* sym = decoder.AbsorbArraySymbol();
                out << L"newarr ";
                if (sym && sym->UnderlayingType)
                    out << sym->UnderlayingType->Name << L"[]";
                else
                    out << L"null";
                
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
