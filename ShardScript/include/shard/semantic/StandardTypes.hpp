#pragma once
#include <shard/Definitions.hpp>

namespace shard
{
	// Roslyn-style well-known symbol registry: standard library types and their
	// members are resolved by enum identity instead of v0.6's process-wide
	// statics, which cannot point into an arena-owned SymbolTable
	enum class StandardTypes
	{
		IAsyncState,
		IAsyncState_MoveNext,

		IAwaitable,
		IAwaitable_GetAwaiter,

		IAwaiter,
		IAwaiter_IsCompleted,
		IAwaiter_OnCompleted,
		IAwaiter_GetResult,

		Task,
		Task_StateField,
		Task_ContinuationField,
		Task_ExceptionField,
		Task_MoveNext,
		Task_IsCompleted,
		Task_GetAwaiter,
		Task_GetResult,
		Task_OnCompleted,
		Task_Complete,
		Task_SetException,
		Task_InternalRoot,
		Task_Delay,
		Task_Shoot,
		Task_ShootGeneric,
		Task_Wait,

		ValueTask,
		ValueTask_T,
		ValueTask_StateField,
		ValueTask_ResultField,
		ValueTask_ContinuationField,
		ValueTask_ExceptionField,
		ValueTask_IsCompleted_get,
		ValueTask_Result_get,
		ValueTask_MoveNext,
		ValueTask_GetAwaiter,
		ValueTask_GetResult,
		ValueTask_OnCompleted,
		ValueTask_InternalRoot,
		ValueTask_FromResult,
		ValueTask_SetResult,
		ValueTask_SetException,
		ValueTask_Shoot,

		Wait_Task,
		Wait_ValueTask,

		IThrowable,
		IThrowable_getMessage,
		IThrowable_getStackTrace,

		IDisposable,
		IDisposable_Dispose,

		IPrintable,
		IPrintable_ToString,

		IEnumerable,
		IEnumerable_GetEnumerator,

		IEnumerator,
		IEnumerator_MoveNext,
		IEnumerator_Current_get,

		IIndexable,

		ArrayEnumerator,
		ArrayEnumerator_T,
		ArrayEnumerator_SourceField,
		ArrayEnumerator_IndexField,
		ArrayEnumerator_LengthField,

		RangeEnumerable,
		RangeEnumerable_FromField,
		RangeEnumerable_ToField,
		RangeEnumerable_CurrentField,
		RangeEnumerable_MoveNext,
		RangeEnumerable_Current_get,

		Runtime,
		Runtime_CaptureStackTrace,

		RuntimeException,
		RuntimeException_MessageField,
		RuntimeException_StackTraceField,

		UndefinedBehaviour,
		UndefinedBehaviour_MessageField,
		UndefinedBehaviour_StackTraceField,

		NativeContinuation,
		NativeContinuation_MoveNext,

		Count
	};
}
