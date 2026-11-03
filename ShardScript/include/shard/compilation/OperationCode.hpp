#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace shard
{
	// CALL_METHOD_INTO destination slot meaning "the current frame's own return slot",
	// used to forward a tail call's result into the caller's chained return storage
	inline constexpr std::uint16_t ReturnSlotSentinel = 0xFFFF;

	// second operand of CALLMETHODSYMBOL: an interface member's vtable slot, or
	// this sentinel for a statically bound call
	inline constexpr std::uint32_t DirectCallSlot = 0xFFFFFFFF;

	enum class OpCode : std::uint16_t
	{
		/// <summary>
		/// Upon receiving this byte, VM does nothing.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		NOP = 1,

		/// <summary>
		/// VM immediately stops execution upon receiving this byte.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		HALT,

		/// <summary>
		/// Pops current top value of the evaluation stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		POP,

		/// <summary>
		/// Loads `null` and pushes its ObjectInstance to the evaluation stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOADCONST_NULL,

		/// <summary>
		/// Loads `bool` and pushes its ObjectInstance to the evaluation stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> bool Value - value to load.</para>
		/// </summary>
		LOADCONST_BOOLEAN,

		/// <summary>
		/// Loads `signed 8-bit integer` and pushes its ObjectInstance to the evaluation stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::int8_t Value - value to load.</para>
		/// </summary>
		LOADCONST_INTEGER8,

		/// <summary>
		/// Loads `signed 64-bit integer` and pushes its ObjectInstance to the evaluation stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::int64_t Value - value to load.</para>
		/// </summary>
		LOADCONST_INTEGER64,

		/// <summary>
		/// Loads `signed platform word-sized integer` and pushes its ObjectInstance to the evaluation stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::intptr_t Value - value to load.</para>
		/// </summary>
		LOADCONST_NATIVE_INTEGER,

		/// <summary>
		/// Loads `signed 64-bit rational` and pushes its ObjectInstance to the evaluation stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> double Value - value to load.</para>
		/// </summary>
		LOADCONST_RATIONAL64,

		/// <summary>
		/// Loads `UTF16 character` and pushes its ObjectInstance to the evaluation stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> wchar_t Value - UTF16 character to load.</para>
		/// </summary>
		LOADCONST_CHAR,

		/// <summary>
		/// Pushes the UTF16 string literal registered in the program image's literal table.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint32_t Literal - index of the string literal, which keeps its data section offset and length.</para>
		/// </summary>
		LOADCONST_STRING,

		/// <summary>
		/// Peeks 1 instance from the evaluation stack and pushes a copy of it.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		DUP,

		/// <summary>
		/// Pops an ObjectInstance and stores it to the local variable slot at given index.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint16_t Value - zero-based index of the local slot to write to.</para>
		/// </summary>
		STORE_LOCAL,

		/// <summary>
		/// Pops an ObjectInstance and stores it to the argument slot at given index.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint16_t Value - zero-based index of the argument slot to write to.</para>
		/// </summary>
		STORE_ARG,

		/// <summary>
		/// Pushes a copy of the ObjectInstance held by the local variable slot at given index.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint16_t Value - zero-based index of the local slot to read from.</para>
		/// </summary>
		LOAD_LOCAL,

		/// <summary>
		/// Pushes a copy of the ObjectInstance held by the argument slot at given index.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint16_t Value - zero-based index of the argument slot to read from.</para>
		/// </summary>
		LOAD_ARG,

		/// <summary>
		/// Pushes the address of the frame slot at given index instead of its value.
		/// Used to reach stack-placed storage without copying it.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint16_t Slot - zero-based index of the frame slot to address.</para>
		/// </summary>
		LOAD_SLOT_ADDR,

		/// <summary>
		/// Unconditional jump to an absolute offset.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Offset - absolute byte offset inside the text section.</para>
		/// </summary>
		JUMP,

		/// <summary>
		/// Pops a boolean and jumps to an absolute offset if it is True.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Offset - absolute byte offset inside the text section.</para>
		/// </summary>
		JUMP_TRUE,

		/// <summary>
		/// Pops a boolean and jumps to an absolute offset if it is False.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Offset - absolute byte offset inside the text section.</para>
		/// </summary>
		JUMP_FALSE,

		/// <summary>
		/// Pops an instance and jumps to an absolute offset if it is null.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Offset - absolute byte offset inside the text section.</para>
		/// </summary>
		JUMP_NULL,

		/// <summary>
		/// Multi-way jump table for dense switch statements.
		/// Pops an integer index from the evaluation stack and jumps to the corresponding target.
		/// <para>Includes 2 parameters :</para>
		/// <para>> std::uint32_t Count - number of jump targets.</para>
		/// <para>> std::size_t Targets[Count] - absolute byte offsets of each target.</para>
		/// </summary>
		JUMP_TABLE,

		/// <summary>
		/// Returns from the current method call, destroying the current CallStackFrame.
		/// If the method is non-void, the top of the evaluation stack is treated as the return value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		RETURN,

		/// <summary>
		/// Pops an exception instance and interrupts the current method flow with it.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		THROW,

		/// <summary>
		/// Pops two values, adds them, pushes the result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_ADDITION,

		/// <summary>
		/// Pops two values, subtracts the right one from the left one, pushes the result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_SUBTRACT,

		/// <summary>
		/// Pops two values, multiplies them, pushes the result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_MULTIPLY,

		/// <summary>
		/// Pops two values, divides the left one by the right one, pushes the result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_DIVISION,

		/// <summary>
		/// Pops two values, pushes the remainder of their integer division.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_MODULO,

		/// <summary>
		/// Pops two values, raises the left one to the power of the right one, pushes the result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_POWER,

		/// <summary>
		/// Pops one value, pushes its arithmetic negation.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_NEGATIVE,

		/// <summary>
		/// Pops two values, shifts the left one left by the right one, pushes the result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_SHL,

		/// <summary>
		/// Pops two values, shifts the left one right by the right one, pushes the result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		MATH_SHR,

		/// <summary>
		/// Pops two values, compares them for equality, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_EQUAL,

		/// <summary>
		/// Pops two values, compares them for inequality, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_NOTEQUAL,

		/// <summary>
		/// Pops two values, checks if the right one is greater than the left one, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_GREATER,

		/// <summary>
		/// Pops two values, checks if the right one is greater or equal to the left one, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_GREATER_OR_EQUAL,

		/// <summary>
		/// Pops two values, checks if the right one is less than the left one, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_LESS,

		/// <summary>
		/// Pops two values, checks if the right one is less or equal to the left one, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_LESS_OR_EQUAL,

		/// <summary>
		/// Pops a boolean and pushes its negation.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOGICAL_NOT,

		/// <summary>
		/// Pops two booleans, pushes their logical disjunction.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOGICAL_OR,

		/// <summary>
		/// Pops two booleans, pushes their logical conjunction.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOGICAL_AND,

		/// <summary>
		/// Allocates a new heap instance of the given type and pushes its reference.
		/// Emitted when the placement verdict of a construction is not stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the type to instantiate.</para>
		/// </summary>
		ALLOC_HEAP,

		/// <summary>
		/// Creates a delegate instance bound to the given method and pushes it.
		/// <para>Includes 2 parameters :</para>
		/// <para>> DelegateTypeSymbol* pType - the delegate type.</para>
		/// <para>> MethodSymbol* pMethod - the method the delegate invokes.</para>
		/// </summary>
		NEWDELEGATE,

		/// <summary>
		/// Pops an object instance, loads the field at the given slot from it and pushes the field value.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint32_t Slot - zero-based field slot inside the instance's TypeShape.</para>
		/// </summary>
		LOADFIELD,

		/// <summary>
		/// Pops an object instance and a value, then stores the value into the instance's field at the given slot.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint32_t Slot - zero-based field slot inside the instance's TypeShape.</para>
		/// </summary>
		STOREFIELD,

		/// <summary>
		/// Creates a new array of the given type, whose length is part of the type.
		/// Pops one element value per slot, last element first.
		/// <para>Includes 1 parameter :</para>
		/// <para>> ArrayTypeSymbol* pType - the array type to instantiate.</para>
		/// </summary>
		NEWARRAY,

		/// <summary>
		/// Loads an element from an array. Pops index and array reference. Pushes element.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOADARRAYELEMENT,

		/// <summary>
		/// Stores a value into an array. Pops value, index, and array reference.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		STOREARRAYELEMENT,

		/// <summary>
		/// Loads the length of an array. Pops array reference, pushes int64 length.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		ARRAYLENGTH,

		/// <summary>
		/// Creates a new array of the given element type and dynamic size.
		/// Pops size (int) from the evaluation stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> ArrayTypeSymbol* pType - the array type, whose element type and length are filled at runtime.</para>
		/// </summary>
		NEWARRAY_DYNAMIC,

		/// <summary>
		/// Pushes the value of the static field.
		/// <para>Includes 1 parameter :</para>
		/// <para>> FieldSymbol* pField - the static field description.</para>
		/// </summary>
		LOADSTATICFIELD,

		/// <summary>
		/// Pushes the value of the enum field as a strongly-typed enum instance.
		/// <para>Includes 1 parameter :</para>
		/// <para>> FieldSymbol* pField - the enum field description.</para>
		/// </summary>
		LOADENUMFIELD,

		/// <summary>
		/// Pops a value and stores it into the static field.
		/// <para>Includes 1 parameter :</para>
		/// <para>> FieldSymbol* pField - the static field description.</para>
		/// </summary>
		STORESTATICFIELD,

		/// <summary>
		/// Invokes MethodSymbol with creation of new CallStackFrame.
		/// If pending type arguments were loaded beforehand with LOAD_TYPEARGUMENT,
		/// they are consumed and bound to the new frame.
		/// <para>When the slot is DirectCallSlot the method is invoked as is; otherwise it
		/// names an interface member and the implementation is taken from the receiver
		/// object's vtable at that slot.</para>
		/// <para>Includes 2 parameters :</para>
		/// <para>> MethodSymbol* pValue - pointer to MethodSymbol to invoke.</para>
		/// <para>> std::uint32_t InterfaceSlot - vtable slot, or DirectCallSlot for static binding.</para>
		/// </summary>
		CALLMETHODSYMBOL,

		/// <summary>
		/// Invokes the target method stored in a delegate instance.
		/// Pops the delegate receiver and its arguments.
		/// <para>Includes 1 parameter :</para>
		/// <para>> DelegateTypeSymbol* pType - the delegate type of the invoked instance.</para>
		/// </summary>
		CALLDELEGATE,

		/// <summary>
		/// Loads a type argument into the current call frame's TypeArguments vector.
		/// <para>Includes 2 parameters :</para>
		/// <para>> std::uint16_t Index - zero-based index in TypeArguments.</para>
		/// <para>> TypeSymbol* pType - the concrete type to load.</para>
		/// </summary>
		LOAD_TYPEARGUMENT,

		/// <summary>
		/// Pops an object instance, checks whether it is compatible with the given
		/// type, and pushes a boolean result.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the type to test against.</para>
		/// </summary>
		ISINSTANCE,

		/// <summary>
		/// Pops an object instance, pushes it back if it is compatible with the given
		/// type, or null otherwise.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the type to cast to.</para>
		/// </summary>
		CAST_AS,

		/// <summary>
		/// Pops an object instance. Raises an exception if the instance is not
		/// compatible with the given type; otherwise pushes the same instance back.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the type to cast to.</para>
		/// </summary>
		CAST,

		/// <summary>
		/// Pops a primitive value, converts it to the given primitive type, and pushes
		/// the converted value.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the primitive type to cast to.</para>
		/// </summary>
		CASTPRIMITIVE,

		/// <summary>
		/// Marks the beginning of a try-protected region. If an exception is raised
		/// inside this region, execution continues at the given handler offset.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t HandlerOffset - absolute byte offset of the catch dispatch block.</para>
		/// </summary>
		ENTER_TRY,

		/// <summary>
		/// Marks the end of a try-protected region. Pops the active handler record.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LEAVE_TRY,

		/// <summary>
		/// Re-raises the exception currently being handled. Must only appear inside a catch block.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		RETHROW,

		/// <summary>
		/// Marks the end of a catch handler. Clears the current exception being handled.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		END_CATCH,

		/// <summary>
		/// Pushes the exception currently being handled by this frame onto the evaluation stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOAD_CURRENT_EXCEPTION,

		/// <summary>
		/// Pops an exception object and makes it the frame's current exception. Used to
		/// restore the exception context after resuming from an await inside a catch clause.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		STORE_CURRENT_EXCEPTION,

		/// <summary>
		/// Registers a deferred expression. Pushes the target instruction pointer onto
		/// the frame's DeferStack; the expression is executed later by DEFER_DRAIN.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Target - absolute byte offset of the deferred expression.</para>
		/// </summary>
		DEFER,

		/// <summary>
		/// Returns from a deferred expression back to the DEFER_DRAIN loop that invoked it.
		/// Pops a return address pushed by DEFER_DRAIN.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		DEFER_BREAK,

		/// <summary>
		/// Drains the top N registered defers from DeferStack and executes them LIFO.
		/// Each deferred expression must end with DEFER_BREAK.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Count - number of defers to drain.</para>
		/// </summary>
		DEFER_DRAIN,

		/// <summary>
		/// Creates a new range enumerable. Pops the inclusive flag, the upper bound and
		/// the lower bound, then pushes the populated instance.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the range type to instantiate.</para>
		/// </summary>
		CREATERANGE,

		/// <summary>
		/// Invokes MethodSymbol and writes the result straight into a frame slot of the
		/// caller, so nothing is pushed onto the evaluation stack. Used for tail calls.
		/// <para>Includes 2 parameters :</para>
		/// <para>> MethodSymbol* pValue - pointer to MethodSymbol to invoke.</para>
		/// <para>> std::uint16_t Slot - destination slot, or ReturnSlotSentinel to write into the caller's own return storage.</para>
		/// </summary>
		CALL_METHOD_INTO,

		/// <summary>
		/// Returns from a method whose result was already placed into the return entry
		/// by CALL_METHOD_INTO. Also settles the frame's outstanding children before
		/// signalling the return.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		RETURN_PLACED,

		/// <summary>
		/// Pops a Task, waits for it to settle and pushes its result. An async frame is
		/// suspended instead of blocking, and resumes at the same instruction; a raising
		/// task rethrows inside this frame.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		AWAIT,

		/// <summary>
		/// Pushes an empty nullable wrapper of the given type.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the nullable type to produce.</para>
		/// </summary>
		NULLABLE_DEFAULT,

		/// <summary>
		/// Pops a value and pushes it wrapped into a nullable of the given type. A null
		/// reference produces an empty wrapper.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the nullable type to produce.</para>
		/// </summary>
		NULLABLE_WRAP,

		/// <summary>
		/// Pops a nullable wrapper and pushes the wrapper's has-value flag.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		NULLABLE_HAS_VALUE,

		/// <summary>
		/// Pops a nullable wrapper and pushes the value it holds. Raises
		/// InvalidOperationException when the wrapper is empty.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the nullable type being unwrapped.</para>
		/// </summary>
		NULLABLE_UNWRAP,

		/// <summary>
		/// Pops two nullable wrappers of the given type and pushes whether they are
		/// equal: two empty wrappers are equal, an empty and a filled one are not.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - the nullable type being compared.</para>
		/// </summary>
		NULLABLE_EQUAL
	};
}
