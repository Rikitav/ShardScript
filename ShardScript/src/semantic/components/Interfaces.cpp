#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/InterfaceSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <array>

using namespace shard;

namespace
{
	gmt::Ref<AccessorSymbol> make_virtual_getter(SymbolTable& table, gmt::Ref<PropertySymbol> property, gmt::Ref<StructSymbol> type)
	{
		gmt::Ref<AccessorSymbol> getter = table.emplace<AccessorSymbol>(L"get", property);
		getter->set_return_type(type);
		getter->set_linking(LINK_INSTANCE);
		getter->set_accesibility(ACS_PUBLIC);
		getter->set_is_virtual(true);

		property->set_getter(getter);
		return getter;
	}

	gmt::Ref<MethodSymbol> make_virtual_method(SymbolTable& table, gmt::Ref<StructSymbol> owner, const wchar_t* name, gmt::Ref<StructSymbol> returnType)
	{
		gmt::Ref<MethodSymbol> method = table.emplace<MethodSymbol>(name, owner);
		method->set_return_type(returnType);
		method->set_accesibility(ACS_PUBLIC);
		method->set_linking(LINK_INSTANCE);
		method->set_is_virtual(true);

		owner->methods.push_back(method);
		return method;
	}
}

void SymbolTable::resolve_interfaces()
{
	// IThrowable
	{
		gmt::Ref<InterfaceSymbol> throwable = m_arena.emplace<InterfaceSymbol>(L"IThrowable", m_rootNamespace);

		gmt::Ref<PropertySymbol> message = m_arena.emplace<PropertySymbol>(L"message", SyntaxKind::PropertyDeclaration, throwable);
		message->set_return_type(m_string);
		message->set_accesibility(ACS_PUBLIC);
		message->set_linking(LINK_INSTANCE);

		gmt::Ref<PropertySymbol> stackTrace = m_arena.emplace<PropertySymbol>(L"stack_trace", SyntaxKind::PropertyDeclaration, throwable);
		stackTrace->set_return_type(m_string);
		stackTrace->set_accesibility(ACS_PUBLIC);
		stackTrace->set_linking(LINK_INSTANCE);

		gmt::Ref<AccessorSymbol> messageGetter = make_virtual_getter(*this, message, m_string);
		gmt::Ref<AccessorSymbol> stackTraceGetter = make_virtual_getter(*this, stackTrace, m_string);

		throwable->properties.push_back(message);
		throwable->properties.push_back(stackTrace);

		m_rootNamespace->members.push_back(throwable);
		set_standard_type(StandardTypes::IThrowable, throwable);
		set_standard_type(StandardTypes::IThrowable_getMessage, messageGetter);
		set_standard_type(StandardTypes::IThrowable_getStackTrace, stackTraceGetter);
	}

	// IDisposable
	{
		gmt::Ref<InterfaceSymbol> disposable = m_arena.emplace<InterfaceSymbol>(L"IDisposable", m_rootNamespace);
		gmt::Ref<MethodSymbol> dispose = make_virtual_method(*this, disposable, L"Dispose", m_void);

		m_rootNamespace->members.push_back(disposable);
		set_standard_type(StandardTypes::IDisposable, disposable);
		set_standard_type(StandardTypes::IDisposable_Dispose, dispose);
	}

	// IPrintable
	{
		gmt::Ref<InterfaceSymbol> printable = m_arena.emplace<InterfaceSymbol>(L"IPrintable", m_rootNamespace);
		gmt::Ref<MethodSymbol> toString = make_virtual_method(*this, printable, L"ToString", m_string);

		m_rootNamespace->members.push_back(printable);
		set_standard_type(StandardTypes::IPrintable, printable);
		set_standard_type(StandardTypes::IPrintable_ToString, toString);
	}

	// IEnumerator<T>
	gmt::Ref<InterfaceSymbol> enumerator;
	gmt::Ref<TypeParameterSymbol> enumeratorT;
	{
		enumerator = m_arena.emplace<InterfaceSymbol>(L"IEnumerator", m_rootNamespace);

		enumeratorT = m_arena.emplace<TypeParameterSymbol>(L"T", enumerator);
		enumerator->type_parameters.push_back(enumeratorT);

		gmt::Ref<MethodSymbol> moveNext = make_virtual_method(*this, enumerator, L"MoveNext", m_boolean);

		gmt::Ref<PropertySymbol> current = m_arena.emplace<PropertySymbol>(L"Current", SyntaxKind::PropertyDeclaration, enumerator);
		current->set_return_type(enumeratorT);
		current->set_accesibility(ACS_PUBLIC);
		current->set_linking(LINK_INSTANCE);

		gmt::Ref<AccessorSymbol> currentGetter = make_virtual_getter(*this, current, enumeratorT);
		enumerator->properties.push_back(current);

		m_rootNamespace->members.push_back(enumerator);
		set_standard_type(StandardTypes::IEnumerator, enumerator);
		set_standard_type(StandardTypes::IEnumerator_MoveNext, moveNext);
		set_standard_type(StandardTypes::IEnumerator_Current_get, currentGetter);
	}

	// IEnumerable<T>
	{
		gmt::Ref<InterfaceSymbol> enumerable = m_arena.emplace<InterfaceSymbol>(L"IEnumerable", m_rootNamespace);

		gmt::Ref<TypeParameterSymbol> enumerableT = m_arena.emplace<TypeParameterSymbol>(L"T", enumerable);
		enumerable->type_parameters.push_back(enumerableT);

		std::array<gmt::Ref<StructSymbol>, 1> enumeratorArgs = { enumerableT };
		gmt::Ref<GenericTypeSymbol> enumeratorType = find_or_create_generic_type(enumerator, enumeratorArgs);
		gmt::Ref<MethodSymbol> getEnumerator = make_virtual_method(*this, enumerable, L"GetEnumerator", enumeratorType);

		m_rootNamespace->members.push_back(enumerable);
		set_standard_type(StandardTypes::IEnumerable, enumerable);
		set_standard_type(StandardTypes::IEnumerable_GetEnumerator, getEnumerator);
	}
}