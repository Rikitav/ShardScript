#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/symbols/AccessorSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>

#include <shard/LibraryMetadata.hpp>
#include <shard/semantic/SemanticModel.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/VirtualMachine.hpp>

using namespace shard;

namespace
{
	static void read_reference_field(MethodCallState& context, StandardTypes fieldSlot)
	{
		SymbolTable& table = context.get_vm().get_model().get_table();
		gmt::Ref<FieldSymbol> field = table.get_standard_type(fieldSlot).as<FieldSymbol>();

		// get_field returns a view onto the slot storage; reference payloads
		// are pointers and must be dereferenced before boxing the result
		ObjectInstance slot = context.get_argument(0).get_field(field);
		context.set_return(ObjectInstance(slot.get_shape(), slot.as_pointer()));
	}

	static void runtime_exception_message_get(MethodCallState& context)
	{
		read_reference_field(context, StandardTypes::RuntimeException_MessageField);
	}

	static void runtime_exception_stack_trace_get(MethodCallState& context)
	{
		read_reference_field(context, StandardTypes::RuntimeException_StackTraceField);
	}

	static void undefined_behaviour_message_get(MethodCallState& context)
	{
		read_reference_field(context, StandardTypes::UndefinedBehaviour_MessageField);
	}

	static void undefined_behaviour_stack_trace_get(MethodCallState& context)
	{
		read_reference_field(context, StandardTypes::UndefinedBehaviour_StackTraceField);
	}

	static gmt::Ref<FieldSymbol> make_backing_field(SymbolTable& table, gmt::Ref<StructSymbol> owner, const wchar_t* name)
	{
		gmt::Ref<FieldSymbol> field = table.emplace<FieldSymbol>(name, owner);
		field->set_return_type(table.get_string_primitive());
		field->set_accesibility(ACS_PRIVATE);
		field->set_linking(LINK_INSTANCE);

		owner->fields.push_back(field);
		return field;
	}

	static void make_throwable_property(SymbolTable& table, gmt::Ref<StructSymbol> owner, const wchar_t* name, MethodSymbolDelegate getterCallback)
	{
		gmt::Ref<PropertySymbol> property = table.emplace<PropertySymbol>(name, SyntaxKind::PropertyDeclaration, owner);
		property->set_return_type(table.get_string_primitive());
		property->set_accesibility(ACS_PUBLIC);
		property->set_linking(LINK_INSTANCE);

		gmt::Ref<AccessorSymbol> getter = table.emplace<AccessorSymbol>(L"get", property);
		getter->set_return_type(table.get_string_primitive());
		getter->set_linking(LINK_INSTANCE);
		getter->set_accesibility(ACS_PUBLIC);
		getter->set_calling_convention(CallingConvention::ShardDecl);
		getter->set_function_pointer(reinterpret_cast<void*>(getterCallback));
		getter->set_effects_computed(true);

		property->set_getter(getter);
		owner->properties.push_back(property);
	}
}

void SymbolTable::resolve_exceptions()
{
	gmt::Ref<StructSymbol> throwable = get_standard_type(StandardTypes::IThrowable).as<StructSymbol>();

	// RuntimeException
	{
		gmt::Ref<StructSymbol> exception = m_arena.emplace<StructSymbol>(L"RuntimeException", SyntaxKind::StructDeclaration, m_rootNamespace);
		exception->interfaces.push_back(throwable);

		gmt::Ref<FieldSymbol> messageField = make_backing_field(*this, exception, L"_message");
		gmt::Ref<FieldSymbol> stackTraceField = make_backing_field(*this, exception, L"_stackTrace");

		make_throwable_property(*this, exception, L"message", &runtime_exception_message_get);
		make_throwable_property(*this, exception, L"stack_trace", &runtime_exception_stack_trace_get);

		m_rootNamespace->members.push_back(exception);
		set_standard_type(StandardTypes::RuntimeException, exception);
		set_standard_type(StandardTypes::RuntimeException_MessageField, messageField);
		set_standard_type(StandardTypes::RuntimeException_StackTraceField, stackTraceField);
	}

	// UndefinedBehaviour — the managed face of native contract violations
	{
		gmt::Ref<StructSymbol> undefined = m_arena.emplace<StructSymbol>(L"UndefinedBehaviour", SyntaxKind::StructDeclaration, m_rootNamespace);
		undefined->interfaces.push_back(throwable);

		gmt::Ref<FieldSymbol> messageField = make_backing_field(*this, undefined, L"_message");
		gmt::Ref<FieldSymbol> stackTraceField = make_backing_field(*this, undefined, L"_stackTrace");

		make_throwable_property(*this, undefined, L"message", &undefined_behaviour_message_get);
		make_throwable_property(*this, undefined, L"stack_trace", &undefined_behaviour_stack_trace_get);

		m_rootNamespace->members.push_back(undefined);
		set_standard_type(StandardTypes::UndefinedBehaviour, undefined);
		set_standard_type(StandardTypes::UndefinedBehaviour_MessageField, messageField);
		set_standard_type(StandardTypes::UndefinedBehaviour_StackTraceField, stackTraceField);
	}
}