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
	AccessorSymbol* make_virtual_getter(SymbolTable& table, PropertySymbol* property, StructSymbol* type)
	{
		AccessorSymbol* getter = table.emplace<AccessorSymbol>(L"get", property);
		getter->set_return_type(type);
		getter->set_linking(LINK_INSTANCE);
		getter->set_accesibility(ACS_PUBLIC);
		getter->set_is_virtual(true);

		property->set_getter(getter);
		return getter;
	}

	MethodSymbol* make_virtual_method(SymbolTable& table, StructSymbol* owner, const wchar_t* name, StructSymbol* returnType)
	{
		MethodSymbol* method = table.emplace<MethodSymbol>(name, owner);
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
		InterfaceSymbol* throwable = m_arena.emplace<InterfaceSymbol>(L"IThrowable", m_rootNamespace);

		PropertySymbol* message = m_arena.emplace<PropertySymbol>(L"message", SyntaxKind::PropertyDeclaration, throwable);
		message->set_return_type(m_string);
		message->set_accesibility(ACS_PUBLIC);
		message->set_linking(LINK_INSTANCE);

		PropertySymbol* stackTrace = m_arena.emplace<PropertySymbol>(L"stack_trace", SyntaxKind::PropertyDeclaration, throwable);
		stackTrace->set_return_type(m_string);
		stackTrace->set_accesibility(ACS_PUBLIC);
		stackTrace->set_linking(LINK_INSTANCE);

		AccessorSymbol* messageGetter = make_virtual_getter(*this, message, m_string);
		AccessorSymbol* stackTraceGetter = make_virtual_getter(*this, stackTrace, m_string);

		throwable->properties.push_back(message);
		throwable->properties.push_back(stackTrace);

		m_rootNamespace->members.push_back(throwable);
		set_standard_type(StandardTypes::IThrowable, throwable);
		set_standard_type(StandardTypes::IThrowable_getMessage, messageGetter);
		set_standard_type(StandardTypes::IThrowable_getStackTrace, stackTraceGetter);
	}

	// IDisposable
	{
		InterfaceSymbol* disposable = m_arena.emplace<InterfaceSymbol>(L"IDisposable", m_rootNamespace);
		MethodSymbol* dispose = make_virtual_method(*this, disposable, L"Dispose", m_void);

		m_rootNamespace->members.push_back(disposable);
		set_standard_type(StandardTypes::IDisposable, disposable);
		set_standard_type(StandardTypes::IDisposable_Dispose, dispose);
	}

	// IPrintable
	{
		InterfaceSymbol* printable = m_arena.emplace<InterfaceSymbol>(L"IPrintable", m_rootNamespace);
		MethodSymbol* toString = make_virtual_method(*this, printable, L"ToString", m_string);

		m_rootNamespace->members.push_back(printable);
		set_standard_type(StandardTypes::IPrintable, printable);
		set_standard_type(StandardTypes::IPrintable_ToString, toString);
	}

	// IEnumerator<T>
	InterfaceSymbol* enumerator = nullptr;
	TypeParameterSymbol* enumeratorT = nullptr;
	{
		enumerator = m_arena.emplace<InterfaceSymbol>(L"IEnumerator", m_rootNamespace);

		enumeratorT = m_arena.emplace<TypeParameterSymbol>(L"T", enumerator);
		enumerator->type_parameters.push_back(enumeratorT);

		MethodSymbol* moveNext = make_virtual_method(*this, enumerator, L"MoveNext", m_boolean);

		PropertySymbol* current = m_arena.emplace<PropertySymbol>(L"Current", SyntaxKind::PropertyDeclaration, enumerator);
		current->set_return_type(enumeratorT);
		current->set_accesibility(ACS_PUBLIC);
		current->set_linking(LINK_INSTANCE);

		AccessorSymbol* currentGetter = make_virtual_getter(*this, current, enumeratorT);
		enumerator->properties.push_back(current);

		m_rootNamespace->members.push_back(enumerator);
		set_standard_type(StandardTypes::IEnumerator, enumerator);
		set_standard_type(StandardTypes::IEnumerator_MoveNext, moveNext);
		set_standard_type(StandardTypes::IEnumerator_Current_get, currentGetter);
	}

	// IEnumerable<T>
	{
		InterfaceSymbol* enumerable = m_arena.emplace<InterfaceSymbol>(L"IEnumerable", m_rootNamespace);

		TypeParameterSymbol* enumerableT = m_arena.emplace<TypeParameterSymbol>(L"T", enumerable);
		enumerable->type_parameters.push_back(enumerableT);

		std::array<StructSymbol*, 1> enumeratorArgs = { enumerableT };
		GenericTypeSymbol* enumeratorType = find_or_create_generic_type(enumerator, enumeratorArgs);
		MethodSymbol* getEnumerator = make_virtual_method(*this, enumerable, L"GetEnumerator", enumeratorType);

		m_rootNamespace->members.push_back(enumerable);
		set_standard_type(StandardTypes::IEnumerable, enumerable);
		set_standard_type(StandardTypes::IEnumerable_GetEnumerator, getEnumerator);
	}
}