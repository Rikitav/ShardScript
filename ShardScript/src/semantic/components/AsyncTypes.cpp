#include <shard/semantic/SymbolTable.hpp>

#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/symbols/DelegateTypeSymbol.hpp>
#include <shard/semantic/symbols/FieldSymbol.hpp>
#include <shard/semantic/symbols/GenericTypeSymbol.hpp>
#include <shard/semantic/symbols/MethodSymbol.hpp>
#include <shard/semantic/symbols/PropertySymbol.hpp>
#include <shard/semantic/symbols/StructSymbol.hpp>
#include <shard/semantic/symbols/TypeParameterSymbol.hpp>

#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>
#include <shard/runtime/VirtualMachine.hpp>

#include <span>

using namespace shard;

namespace
{
	ObjectInstance task_state_of(MethodCallState& state, ObjectInstance source, StandardTypes stateFieldSlot)
	{
		SymbolTable& table = state.get_vm().get_model().get_table();
		FieldSymbol* stateField = static_cast<FieldSymbol*>(table.get_standard_type(stateFieldSlot));

		// the field stores a reference; dereference the slot payload
		ObjectInstance slot = source.get_field(stateField);
		return ObjectInstance(slot.get_shape(), slot.as_pointer());
	}

	void task_completion_source_ctor(MethodCallState& state)
	{
		ObjectInstance self = state.get_argument(0);
		SymbolTable& table = state.get_vm().get_model().get_table();
		FieldSymbol* stateField = static_cast<FieldSymbol*>(table.get_standard_type(StandardTypes::TaskCompletionSource_StateField));

		ObjectInstance slot = self.get_field(stateField);
		ObjectInstance taskState = state.get_vm().get_collector().allocate_instance(slot.get_shape());
		self.set_field(stateField, taskState);
	}

	void task_completion_source_set_result(MethodCallState& state)
	{
		ObjectInstance self = state.get_argument(0);
		ObjectInstance value = state.get_argument(1);
		state.get_vm().complete_task_state(task_state_of(state, self, StandardTypes::TaskCompletionSource_StateField), value);
	}

	void task_completion_source_set_exception(MethodCallState& state)
	{
		ObjectInstance self = state.get_argument(0);
		ObjectInstance exception = state.get_argument(1);
		state.get_vm().fail_task_state(task_state_of(state, self, StandardTypes::TaskCompletionSource_StateField), exception);
	}

	void task_completion_source_task_get(MethodCallState& state)
	{
		ObjectInstance self = state.get_argument(0);
		ObjectInstance taskState = task_state_of(state, self, StandardTypes::TaskCompletionSource_StateField);
		state.set_return(state.get_vm().create_task_wrapper(taskState));
	}

	void task_delay(MethodCallState& state)
	{
		state.set_return(state.get_vm().delay_task(state.get_integer_argument(0)));
	}

	void task_continue_with(MethodCallState& state)
	{
		ObjectInstance self = state.get_argument(0);
		ObjectInstance continuation = state.get_argument(1);
		state.set_return(state.get_vm().continue_task(self, continuation));
	}
}

void SymbolTable::resolve_async_types()
{
	StructSymbol* voidType = m_void;
	StructSymbol* integerType = m_integer;
	StructSymbol* throwableType = static_cast<StructSymbol*>(get_standard_type(StandardTypes::IThrowable));

	// ---- shared task state ------------------------------------------------
	// non-generic state for 'Task', generic state for 'Task<T>'
	SymbolBuilder<StructSymbol> taskStateBuilder(*this, L"__TaskState", m_rootNamespace);
	StructSymbol* taskState = taskStateBuilder.get();
	taskState->set_accesibility(ACS_PRIVATE);

	FieldSymbol* statusField = taskStateBuilder.add_field(L"_status", integerType, LINK_INSTANCE, ACS_PRIVATE);
	FieldSymbol* exceptionField = taskStateBuilder.add_field(L"_exception", throwableType, LINK_INSTANCE, ACS_PRIVATE);

	SymbolBuilder<StructSymbol> taskStateTBuilder(*this, L"__TaskState", m_rootNamespace);
	StructSymbol* taskStateT = taskStateTBuilder.get();
	taskStateT->set_accesibility(ACS_PRIVATE);

	TypeParameterSymbol* taskStateTParameter = taskStateTBuilder.add_type_parameter(L"T").get();

	taskStateTBuilder.add_field(L"_status", integerType, LINK_INSTANCE, ACS_PRIVATE);
	taskStateTBuilder.add_field(L"_exception", throwableType, LINK_INSTANCE, ACS_PRIVATE);
	FieldSymbol* resultFieldT = taskStateTBuilder.add_field(L"_result", taskStateTParameter, LINK_INSTANCE, ACS_PRIVATE);

	set_standard_type(StandardTypes::TaskState, taskState);
	set_standard_type(StandardTypes::TaskState_T, taskStateT);
	set_standard_type(StandardTypes::TaskState_StatusField, statusField);
	set_standard_type(StandardTypes::TaskState_ExceptionField, exceptionField);
	set_standard_type(StandardTypes::TaskState_ResultField, resultFieldT);

	// ---- continuation delegate -------------------------------------------
	DelegateTypeSymbol* action = m_arena.emplace<DelegateTypeSymbol>(L"Action", m_rootNamespace);
	action->set_accesibility(ACS_PUBLIC);
	action->set_return_type(voidType);
	m_rootNamespace->members.push_back(action);
	set_standard_type(StandardTypes::AsyncAction, action);

	// ---- Task -------------------------------------------------------------
	SymbolBuilder<StructSymbol> taskBuilder(*this, L"Task", m_rootNamespace);
	StructSymbol* task = taskBuilder.get();

	FieldSymbol* taskStateField = taskBuilder.add_field(L"_state", taskState, LINK_INSTANCE, ACS_PRIVATE);

	MethodSymbol* delayMethod = taskBuilder.add_method(L"Delay", task, LINK_STATIC)
		.add_parameter(L"milliseconds", integerType)
		.set_callback(&task_delay)
		.get();

	set_standard_type(StandardTypes::Task, task);
	set_standard_type(StandardTypes::Task_StateField, taskStateField);
	set_standard_type(StandardTypes::Task_Delay, delayMethod);

	// TODO: port 'Task.Shoot' once its semantics for the rewrite are decided

	// ---- Task<T> ----------------------------------------------------------
	SymbolBuilder<StructSymbol> taskTBuilder(*this, L"Task", m_rootNamespace);
	StructSymbol* taskT = taskTBuilder.get();

	TypeParameterSymbol* taskTParameter = taskTBuilder.add_type_parameter(L"T").get();
	StructSymbol* taskTArguments[1] = { taskTParameter };

	GenericTypeSymbol* stateOfT = find_or_create_generic_type(taskStateT, std::span<StructSymbol* const>(taskTArguments, 1));
	FieldSymbol* taskTStateField = taskTBuilder.add_field(L"_state", stateOfT, LINK_INSTANCE, ACS_PRIVATE);

	set_standard_type(StandardTypes::Task_T, taskT);
	set_standard_type(StandardTypes::TaskT_StateField, taskTStateField);

	// ---- ContinueWith extensions -----------------------------------------
	// a static function whose first parameter is the receiver binds as an
	// extension call: 'task.ContinueWith(continuation)'
	MethodSymbol* continueMethod = SymbolBuilder<MethodSymbol>(*this, L"ContinueWith", task, LINK_STATIC, ACS_PUBLIC, m_rootNamespace)
		.add_parameter(L"task", task)
		.add_parameter(L"continuation", action)
		.set_callback(&task_continue_with)
		.get();

	SymbolBuilder<MethodSymbol> continueGeneric(*this, L"ContinueWith", task, LINK_STATIC, ACS_PUBLIC, m_rootNamespace);
	TypeParameterSymbol* continueTParameter = continueGeneric.add_type_parameter(L"T").get();
	StructSymbol* continueArguments[1] = { continueTParameter };
	GenericTypeSymbol* continueTaskType = find_or_create_generic_type(taskT, std::span<StructSymbol* const>(continueArguments, 1));

	continueGeneric.get()->set_return_type(continueTaskType);
	MethodSymbol* continueTMethod = continueGeneric
		.add_parameter(L"task", continueTaskType)
		.add_parameter(L"continuation", action)
		.set_callback(&task_continue_with)
		.get();

	set_standard_type(StandardTypes::Task_ContinueWith, continueMethod);
	set_standard_type(StandardTypes::TaskT_ContinueWith, continueTMethod);

	// ---- TaskCompletionSource<T> -----------------------------------------
	SymbolBuilder<StructSymbol> completionSourceBuilder(*this, L"TaskCompletionSource", m_rootNamespace);
	StructSymbol* completionSource = completionSourceBuilder.get();

	TypeParameterSymbol* completionTParameter = completionSourceBuilder.add_type_parameter(L"T").get();
	StructSymbol* completionArguments[1] = { completionTParameter };

	GenericTypeSymbol* completionState = find_or_create_generic_type(taskStateT, std::span<StructSymbol* const>(completionArguments, 1));
	FieldSymbol* completionStateField = completionSourceBuilder.add_field(L"_state", completionState, LINK_INSTANCE, ACS_PRIVATE);

	GenericTypeSymbol* completionTaskType = find_or_create_generic_type(taskT, std::span<StructSymbol* const>(completionArguments, 1));

	ConstructorSymbol* completionConstructor = completionSourceBuilder.add_init(ACS_PUBLIC)
		.set_callback(&task_completion_source_ctor)
		.get();

	MethodSymbol* setResultMethod = completionSourceBuilder.add_method(L"SetResult", voidType, LINK_INSTANCE)
		.add_parameter(L"result", completionTParameter)
		.set_callback(&task_completion_source_set_result)
		.get();

	MethodSymbol* setExceptionMethod = completionSourceBuilder.add_method(L"SetException", voidType, LINK_INSTANCE)
		.add_parameter(L"exception", throwableType)
		.set_callback(&task_completion_source_set_exception)
		.get();

	AccessorSymbol* taskGetter = completionSourceBuilder.add_property(L"task", completionTaskType, LINK_INSTANCE)
		.add_getter()
		.set_callback(&task_completion_source_task_get)
		.get();

	set_standard_type(StandardTypes::TaskCompletionSource_T, completionSource);
	set_standard_type(StandardTypes::TaskCompletionSource_StateField, completionStateField);
	set_standard_type(StandardTypes::TaskCompletionSource_Constructor, completionConstructor);
	set_standard_type(StandardTypes::TaskCompletionSource_SetResult, setResultMethod);
	set_standard_type(StandardTypes::TaskCompletionSource_SetException, setExceptionMethod);
	set_standard_type(StandardTypes::TaskCompletionSource_Task_get, taskGetter);
}
