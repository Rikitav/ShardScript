#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <shard/libframework/FuncCallState.hpp>

#include <array>

using namespace shard;

namespace
{
	// Equals(left, right) is left.CompareTo(right) == 0, so a single interface
	// carries both ordering and equality: all a hash map key needs.
	void equality_equals(FuncCallState& context)
	{
		SymbolTable& table = context.get_vm().get_model().get_table();
		MethodSymbol* compareTo = static_cast<MethodSymbol*>(table.get_standard_type(StandardTypes::IComparable_CompareTo));

		if (compareTo == nullptr)
		{
			context.set_return_boolean(false);
			return;
		}

		InvokeResult result = context.try_invoke_method(compareTo, { context.get_argument(0), context.get_argument(1) });
		if (!result.is_ok())
		{
			context.propagate(result.get_exception());
			return;
		}

		context.set_return_boolean(result.get_value().as_integer() == 0);
	}
}

void SymbolTable::resolve_equality()
{
	StructSymbol* comparable = static_cast<StructSymbol*>(get_standard_type(StandardTypes::IComparable));

	// registered in the root namespace, which doubles as the extension surface:
	// 'Equals(left, right)' and 'left.Equals(right)' both resolve
	MethodSymbol* equals = m_arena.emplace<MethodSymbol>(L"Equals", m_rootNamespace);
	equals->set_calling_convention(CallingConvention::ShardDecl);
	equals->set_function_pointer(reinterpret_cast<void*>(&equality_equals));
	equals->set_return_type(m_boolean);
	equals->set_linking(LINK_STATIC);
	equals->set_accesibility(ACS_PUBLIC);
	equals->set_effects_computed(true);

	TypeParameterSymbol* key = m_arena.emplace<TypeParameterSymbol>(L"T", equals);
	equals->on_symbol_declared(key);

	std::array<StructSymbol*, 1> arguments = { key };
	GenericTypeSymbol* comparableOfT = find_or_create_generic_type(comparable, arguments);
	key->add_constraint(comparableOfT);

	ParameterSymbol* left = m_arena.emplace<ParameterSymbol>(L"left", equals);
	left->set_type(key);
	equals->on_symbol_declared(left);

	ParameterSymbol* right = m_arena.emplace<ParameterSymbol>(L"right", equals);
	right->set_type(key);
	equals->on_symbol_declared(right);

	m_rootNamespace->members.push_back(equals);
}
