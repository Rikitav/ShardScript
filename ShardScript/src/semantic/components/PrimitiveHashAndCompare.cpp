#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>

#include <shard/libframework/FuncCallState.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

using namespace shard;

namespace
{
	std::int64_t sign_of(std::int64_t left, std::int64_t right)
	{
		return left < right ? -1 : (left > right ? 1 : 0);
	}

	// --- IHashable callbacks (argument 0 is the receiver) --------------------

	void primitive_boolean_hash(FuncCallState& context)
	{
		context.set_return_integer(context.get_boolean_argument(0) ? 1 : 0);
	}

	void primitive_integer_hash(FuncCallState& context)
	{
		context.set_return_integer(context.get_integer_argument(0));
	}

	void primitive_float_hash(FuncCallState& context)
	{
		const double value = context.get_float_argument(0);

		// -0.0 folds onto 0.0 so values that compare equal hash equally
		std::uint64_t bits = 0;
		if (value != 0.0)
			std::memcpy(&bits, &value, sizeof(bits));

		context.set_return_integer(static_cast<std::int64_t>(bits));
	}

	void primitive_char_hash(FuncCallState& context)
	{
		context.set_return_integer(static_cast<std::int64_t>(context.get_char_argument(0)));
	}

	void primitive_byte_hash(FuncCallState& context)
	{
		context.set_return_integer(context.get_integer_argument(0));
	}

	void primitive_nint_hash(FuncCallState& context)
	{
		context.set_return_integer(static_cast<std::int64_t>(context.get_nint_argument(0)));
	}

	// FNV-1a over the code units: stable across runs, unlike address-based hashes
	void primitive_string_hash(FuncCallState& context)
	{
		const std::wstring_view text = context.get_string_argument(0);

		std::uint64_t hash = 0xCBF29CE484222325ULL;
		for (const wchar_t symbol : text)
		{
			hash ^= static_cast<std::uint64_t>(symbol);
			hash *= 0x100000001B3ULL;
		}

		context.set_return_integer(static_cast<std::int64_t>(hash));
	}

	// --- IComparable<T> callbacks (argument 0 is the receiver) ---------------

	void primitive_boolean_compare(FuncCallState& context)
	{
		const bool left = context.get_boolean_argument(0);
		const bool right = context.get_boolean_argument(1);

		context.set_return_integer(left == right ? 0 : (left ? 1 : -1));
	}

	void primitive_integer_compare(FuncCallState& context)
	{
		context.set_return_integer(sign_of(context.get_integer_argument(0), context.get_integer_argument(1)));
	}

	void primitive_float_compare(FuncCallState& context)
	{
		const double left = context.get_float_argument(0);
		const double right = context.get_float_argument(1);

		// NaN sorts below everything, including NaN, so the order stays total
		const bool leftNaN = left != left;
		const bool rightNaN = right != right;
		if (leftNaN || rightNaN)
		{
			context.set_return_integer(leftNaN == rightNaN ? 0 : (leftNaN ? -1 : 1));
			return;
		}

		context.set_return_integer(left < right ? -1 : (left > right ? 1 : 0));
	}

	void primitive_char_compare(FuncCallState& context)
	{
		const wchar_t left = context.get_char_argument(0);
		const wchar_t right = context.get_char_argument(1);

		context.set_return_integer(left < right ? -1 : (left > right ? 1 : 0));
	}

	void primitive_byte_compare(FuncCallState& context)
	{
		context.set_return_integer(sign_of(context.get_integer_argument(0), context.get_integer_argument(1)));
	}

	void primitive_nint_compare(FuncCallState& context)
	{
		const std::int64_t left = static_cast<std::int64_t>(context.get_nint_argument(0));
		const std::int64_t right = static_cast<std::int64_t>(context.get_nint_argument(1));

		context.set_return_integer(sign_of(left, right));
	}

	void primitive_string_compare(FuncCallState& context)
	{
		const std::wstring_view left = context.get_string_argument(0);
		const std::wstring_view right = context.get_string_argument(1);

		context.set_return_integer(left < right ? -1 : (left > right ? 1 : 0));
	}

	// --- conformance --------------------------------------------------------

	void make_hashable(SymbolTable& table, StructSymbol* symbol, StructSymbol* hashable, FuncHandler callback)
	{
		MethodSymbol* getHashCode = table.emplace<MethodSymbol>(L"GetHashCode", symbol);
		getHashCode->set_return_type(table.get_integer_primitive());
		getHashCode->set_accesibility(ACS_PUBLIC);
		getHashCode->set_linking(LINK_INSTANCE);
		getHashCode->set_calling_convention(CallingConvention::ShardDecl);
		getHashCode->set_function_pointer(reinterpret_cast<void*>(callback));
		getHashCode->set_effects_computed(true);

		symbol->methods.push_back(getHashCode);
		symbol->interfaces.push_back(hashable);
	}

	void make_comparable(SymbolTable& table, StructSymbol* symbol, StructSymbol* comparable, FuncHandler callback)
	{
		MethodSymbol* compareTo = table.emplace<MethodSymbol>(L"CompareTo", symbol);
		compareTo->set_return_type(table.get_integer_primitive());
		compareTo->set_accesibility(ACS_PUBLIC);
		compareTo->set_linking(LINK_INSTANCE);
		compareTo->set_calling_convention(CallingConvention::ShardDecl);
		compareTo->set_function_pointer(reinterpret_cast<void*>(callback));
		compareTo->set_effects_computed(true);

		ParameterSymbol* other = table.emplace<ParameterSymbol>(L"other", compareTo);
		other->set_type(symbol);
		compareTo->on_symbol_declared(other);

		symbol->methods.push_back(compareTo);

		// conformance is nominal, and the argument makes it IComparable<itself>
		std::array<StructSymbol*, 1> arguments = { symbol };
		symbol->interfaces.push_back(table.find_or_create_generic_type(comparable, arguments));
	}
}

void SymbolTable::resolve_primitive_hashables()
{
	StructSymbol* hashable = static_cast<StructSymbol*>(get_standard_type(StandardTypes::IHashable));

	make_hashable(*this, m_boolean, hashable, &primitive_boolean_hash);
	make_hashable(*this, m_integer, hashable, &primitive_integer_hash);
	make_hashable(*this, m_float, hashable, &primitive_float_hash);
	make_hashable(*this, m_char, hashable, &primitive_char_hash);
	make_hashable(*this, m_byte, hashable, &primitive_byte_hash);
	make_hashable(*this, m_nativeInteger, hashable, &primitive_nint_hash);
	make_hashable(*this, m_string, hashable, &primitive_string_hash);
}

void SymbolTable::resolve_primitive_comparables()
{
	StructSymbol* comparable = static_cast<StructSymbol*>(get_standard_type(StandardTypes::IComparable));

	make_comparable(*this, m_boolean, comparable, &primitive_boolean_compare);
	make_comparable(*this, m_integer, comparable, &primitive_integer_compare);
	make_comparable(*this, m_float, comparable, &primitive_float_compare);
	make_comparable(*this, m_char, comparable, &primitive_char_compare);
	make_comparable(*this, m_byte, comparable, &primitive_byte_compare);
	make_comparable(*this, m_nativeInteger, comparable, &primitive_nint_compare);
	make_comparable(*this, m_string, comparable, &primitive_string_compare);
}
