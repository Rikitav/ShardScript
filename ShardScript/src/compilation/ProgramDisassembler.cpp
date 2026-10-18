#include <shard/compilation/ProgramDisassembler.hpp>
#include <shard/compilation/ByteCodeDecoder.hpp>
#include <shard/compilation/OperationCode.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/semantic/SyntaxSymbol.hpp>
#include <shard/semantic/symbols/ArrayTypeSymbol.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

using namespace shard;

namespace
{
	void write_hex_offset(std::wostream& out, std::size_t offset)
	{
		const auto oldFlags = out.flags();
		const auto oldFill = out.fill(L'0');
		out << std::hex << std::uppercase << std::setw(4) << offset;
		out.fill(oldFill);
		out.flags(oldFlags);
	}

	void write_symbol_name(std::wostream& out, gmt::Ref<SyntaxSymbol> symbol)
	{
		if (symbol.is_null())
		{
			out << L"null";
			return;
		}

		out << symbol->get_name();
	}

	void disassemble_instruction(std::wostream& out, ByteCodeDecoder& decoder)
	{
		const std::size_t currentOffset = decoder.index();

		std::wostringstream line;
		line << L"SS_";
		write_hex_offset(line, currentOffset);
		line << L":";

		const OpCode op = decoder.absorb_opcode();
		std::wstring opcode;
		std::wostringstream args;

		switch (op)
		{
			case OpCode::NOP:                       opcode = L"nop"; break;
			case OpCode::HALT:                      opcode = L"halt"; break;
			case OpCode::POP:                       opcode = L"pop"; break;
			case OpCode::LOADCONST_NULL:            opcode = L"ldnull"; break;
			case OpCode::DUP:                       opcode = L"dup"; break;
			case OpCode::RETURN:                    opcode = L"ret"; break;
			case OpCode::THROW:                     opcode = L"throw"; break;

			case OpCode::MATH_ADDITION:             opcode = L"add"; break;
			case OpCode::MATH_SUBTRACT:             opcode = L"sub"; break;
			case OpCode::MATH_MULTIPLY:             opcode = L"mul"; break;
			case OpCode::MATH_DIVISION:             opcode = L"div"; break;
			case OpCode::MATH_MODULO:               opcode = L"mod"; break;
			case OpCode::MATH_POWER:                opcode = L"pow"; break;
			case OpCode::MATH_NEGATIVE:             opcode = L"neg"; break;
			case OpCode::MATH_SHL:                  opcode = L"shl"; break;
			case OpCode::MATH_SHR:                  opcode = L"shr"; break;

			case OpCode::COMPARE_EQUAL:             opcode = L"cmp_eq"; break;
			case OpCode::COMPARE_NOTEQUAL:          opcode = L"cmp_neq"; break;
			case OpCode::COMPARE_LESS:              opcode = L"cmp_l"; break;
			case OpCode::COMPARE_LESS_OR_EQUAL:     opcode = L"cmp_le"; break;
			case OpCode::COMPARE_GREATER:           opcode = L"cmp_gt"; break;
			case OpCode::COMPARE_GREATER_OR_EQUAL:  opcode = L"cmp_ge"; break;

			case OpCode::LOGICAL_NOT:               opcode = L"not"; break;
			case OpCode::LOGICAL_AND:               opcode = L"and"; break;
			case OpCode::LOGICAL_OR:                opcode = L"or"; break;

			case OpCode::LOADARRAYELEMENT:          opcode = L"ldelem"; break;
			case OpCode::STOREARRAYELEMENT:         opcode = L"stelem"; break;
			case OpCode::ARRAYLENGTH:               opcode = L"ldlen"; break;

			case OpCode::LEAVE_TRY:                 opcode = L"leave_try"; break;
			case OpCode::RETHROW:                   opcode = L"rethrow"; break;
			case OpCode::END_CATCH:                 opcode = L"end_catch"; break;
			case OpCode::LOAD_CURRENT_EXCEPTION:    opcode = L"load_current_exception"; break;
			case OpCode::STORE_CURRENT_EXCEPTION:   opcode = L"store_current_exception"; break;
			case OpCode::DEFER_BREAK:               opcode = L"defer_break"; break;

			case OpCode::LOADCONST_BOOLEAN:
			{
				opcode = L"ldc.bool";
				args << (decoder.absorb_boolean() ? L"true" : L"false");
				break;
			}

			case OpCode::LOADCONST_INTEGER8:
			{
				opcode = L"ldc.i1";
				const auto value = static_cast<std::int8_t>(decoder.absorb_uint8());
				args << static_cast<std::int32_t>(value);
				break;
			}

			case OpCode::LOADCONST_INTEGER64:
			{
				opcode = L"ldc.i8";
				args << decoder.absorb_int64();
				break;
			}

			case OpCode::LOADCONST_NATIVE_INTEGER:
			{
				opcode = L"ldc.nint";
				args << decoder.absorb_int_ptr();
				break;
			}

			case OpCode::LOADCONST_RATIONAL64:
			{
				opcode = L"ldc.r8";
				args << decoder.absorb_double64();
				break;
			}

			case OpCode::LOADCONST_CHAR:
			{
				opcode = L"ldc.char";
				const wchar_t ch = decoder.absorb_char16();
				args << L"'" << ch << L"'";
				break;
			}

			case OpCode::LOADCONST_STRING:
			{
				opcode = L"ldstr";
				const std::size_t dataOffset = decoder.absorb_string();
				args << L"pool[" << dataOffset << L"]";
				break;
			}

			case OpCode::LOAD_LOCAL:
			{
				opcode = L"ldloc";
				args << decoder.absorb_variable_slot();
				break;
			}

			case OpCode::STORE_LOCAL:
			{
				opcode = L"stloc";
				args << decoder.absorb_variable_slot();
				break;
			}

			case OpCode::LOAD_ARG:
			{
				opcode = L"ldarg";
				args << decoder.absorb_variable_slot();
				break;
			}

			case OpCode::STORE_ARG:
			{
				opcode = L"starg";
				args << decoder.absorb_variable_slot();
				break;
			}

			case OpCode::LOAD_SLOT_ADDR:
			{
				opcode = L"load_slot_addr";
				args << decoder.absorb_variable_slot();
				break;
			}

			case OpCode::JUMP:
			{
				opcode = L"jmp";
				args << L"SS_";
				write_hex_offset(args, decoder.absorb_jump());
				break;
			}

			case OpCode::JUMP_FALSE:
			{
				opcode = L"jmpf";
				args << L"SS_";
				write_hex_offset(args, decoder.absorb_jump());
				break;
			}

			case OpCode::JUMP_TRUE:
			{
				opcode = L"jmpt";
				args << L"SS_";
				write_hex_offset(args, decoder.absorb_jump());
				break;
			}

			case OpCode::BR_NULL:
			{
				opcode = L"brnull";
				args << L"SS_";
				write_hex_offset(args, decoder.absorb_jump());
				break;
			}

			case OpCode::JUMP_TABLE:
			{
				opcode = L"jmptable";
				const std::uint32_t count = decoder.absorb_uint32();
				args << count << L" [";
				for (std::uint32_t i = 0; i < count; ++i)
				{
					if (i > 0)
						args << L", ";

					args << L"SS_";
					write_hex_offset(args, decoder.absorb_jump());
				}

				args << L"]";
				break;
			}

			case OpCode::ENTER_TRY:
			{
				opcode = L"enter_try";
				args << L"SS_";
				write_hex_offset(args, decoder.absorb_jump());
				break;
			}

			case OpCode::DEFER:
			{
				opcode = L"defer";
				args << L"SS_";
				write_hex_offset(args, decoder.absorb_jump());
				break;
			}

			case OpCode::DEFER_DRAIN:
			{
				opcode = L"defer_drain";
				args << decoder.absorb_jump();
				break;
			}

			case OpCode::ALLOC_HEAP:
			{
				opcode = L"alloc_heap";
				write_symbol_name(args, decoder.absorb_symbol());
				break;
			}

			case OpCode::NEWDELEGATE:
			{
				opcode = L"newdelegate";
				write_symbol_name(args, decoder.absorb_delegate_type_symbol());
				break;
			}

			case OpCode::LOADFIELD:
			{
				opcode = L"ldfld";
				args << L"slot_" << decoder.absorb_field_slot();
				break;
			}

			case OpCode::STOREFIELD:
			{
				opcode = L"stfld";
				args << L"slot_" << decoder.absorb_field_slot();
				break;
			}

			case OpCode::LOADSTATICFIELD:
			{
				opcode = L"ldsfld";
				write_symbol_name(args, decoder.absorb_field_symbol());
				break;
			}

			case OpCode::LOADENUMFIELD:
			{
				opcode = L"ldenum";
				write_symbol_name(args, decoder.absorb_field_symbol());
				break;
			}

			case OpCode::STORESTATICFIELD:
			{
				opcode = L"stsfld";
				write_symbol_name(args, decoder.absorb_field_symbol());
				break;
			}

			case OpCode::NEWARRAY:
			{
				opcode = L"newarr";
				write_symbol_name(args, decoder.absorb_array_symbol());
				break;
			}

			case OpCode::NEWARRAY_DYNAMIC:
			{
				opcode = L"newdynarr";
				write_symbol_name(args, decoder.absorb_type_symbol());
				break;
			}

			case OpCode::CREATERANGE:
			{
				opcode = L"newrange";
				write_symbol_name(args, decoder.absorb_type_symbol());
				break;
			}

			case OpCode::LOAD_TYPEARGUMENT:
			{
				opcode = L"ldtypearg";
				const std::uint16_t index = decoder.absorb_uint16();
				args << index << L" (";
				write_symbol_name(args, decoder.absorb_type_symbol());
				args << L")";
				break;
			}

			case OpCode::CALLMETHODSYMBOL:
			{
				opcode = L"call";
				write_symbol_name(args, decoder.absorb_method_symbol());
				break;
			}

			case OpCode::CALLDELEGATE:
			{
				opcode = L"calldelegate";
				write_symbol_name(args, decoder.absorb_delegate_type_symbol());
				break;
			}

			case OpCode::CALLINTERFACE:
			{
				opcode = L"callinterface";
				write_symbol_name(args, decoder.absorb_method_symbol());
				break;
			}

			case OpCode::ISINSTANCE:
			{
				opcode = L"isinst";
				write_symbol_name(args, decoder.absorb_type_symbol());
				break;
			}

			case OpCode::CAST_AS:
			{
				opcode = L"castinterface";
				write_symbol_name(args, decoder.absorb_type_symbol());
				break;
			}

			case OpCode::CAST:
			{
				opcode = L"cast";
				write_symbol_name(args, decoder.absorb_type_symbol());
				break;
			}

			case OpCode::CASTPRIMITIVE:
			{
				opcode = L"castprimitive";
				write_symbol_name(args, decoder.absorb_type_symbol());
				break;
			}

			default:
			{
				opcode = L"unknown";
				args << L"0x" << std::hex << static_cast<std::uint16_t>(op);
				break;
			}
		}

		out << std::setw(9) << std::left << line.str();
		out << std::setw(16) << std::right << opcode << L" ";

		if (!args.str().empty())
			out << args.str();

		out << L"\n";
	}
}

void ProgramDisassembler::disassemble(std::wostream& out, const VirtualProgramImage& program)
{
	const std::vector<VirtualProgramImage::MethodEntry>& entries = program.get_method_entries();
	for (const VirtualProgramImage::MethodEntry& entry : entries)
	{
		out << L"; ============================================================\n";
		out << L";  DISASSEMBLY: ";

		if (entry.method.is_null())
			out << L"<unknown>";
		else
			out << entry.method->get_name();

		out << L"\n";
		out << L"; ============================================================\n";
		out << L"; Offset           Opcode Arguments\n";
		out << L"; -------- -------------- -----------------------------------------\n";

		ByteCodeDecoder decoder(program, entry.start, entry.length);
		while (!decoder.is_eof())
			disassemble_instruction(out, decoder);

		out << L"; --- DISASSEMBLY END ---\n\n";
	}
}
