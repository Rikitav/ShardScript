#pragma once
#include <cstdint>

namespace shard
{
	enum class OpCode : std::uint16_t
	{
		/// <summary>
		/// Upon receiving this byte, VM does nothing.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		NOP = 1,
		
		/// <summary>
		/// VM immidietly stops execution upon receiving this byte.
		/// <para>Includes 1 parameter :</para>
		/// <para>> int32_t exit_code - exit code of virtual program image
		/// </summary>
		HALT,

		/// <summary>
		/// Pops current top value of stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		POPSTACK,

		/// <summary>
		/// Pops N values from stack. Used for stack cleanup
		/// <para>Includes 1 parameter :</para>
		/// <para>> uin8_t Value - How mush values to pop.</para>
		/// </summary>
		POPSTACK_N,

		/// <summary>
		/// Loads `null` and pushes its ObjectInstance to stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOADCONST_NULL,

		/// <summary>
		/// Loads `boolean` and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> bool Value - bool stored to load.</para>
		/// </summary>
		LOADCONST_BOOLEAN,

		/// <summary>
		/// Loads `signed 64-bit integer number` and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> long Value - long stored to load.</para>
		/// </summary>
		LOADCONST_INTEGER64,

		/// <summary>
		/// Loads `signed 64-bit floating point integer`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> double Value - double stored to load.</para>
		/// </summary>
		LOADCONST_RATIONAL64,
		
		/// <summary>
		/// Loads `UTF16 character`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> wchar_t Value - UTF16 character stored to load.</para>
		/// </summary>
		LOADCONST_CHAR,
		
		/// <summary>
		/// Loads NULL-terminated UTF16 string from data section, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Value - offset inside Data section of program, to first symbol of NULL-Terminated UTF16 string to load.</para>
		/// </summary>
		LOADCONST_STRING,

		/// <summary>
		/// Pops 1 instance from stack and creates duplicate of it.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		CREATEDUPLICATE,

		/// <summary>
		/// Pops ObjectInstance* from stack and stores it to variable slot at given index.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint16_t Value - Zero-based index of variable, where instance will be written to.</para>
		/// </summary>
		STOREVARIABLE,
		
		/// <summary>
		/// Pushes copy of ObjectInstance* from variable slot on given index to stack top.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::uint16_t Value - Zero-based index of variable, where instance will be readed from.</para>
		/// </summary>
		LOADVARIABLE,

		/// <summary>
		/// Unconditional jump to a relative offset.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		JUMP,

		/// <summary>
		/// Conditional jump to a relative offset if stack top contains True.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		JUMP_TRUE,

		/// <summary>
		/// Conditional jump to a relative offset if stack top contains False.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		JUMP_FALSE,

		/// <summary>
		/// Returns from the current method call, destroying the current CallStackFrame.
		/// If the method is non-void, the top of the stack is treated as the return value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		RETURN,
			
		/// <summary>
		/// Interrupts current method flow with an exception.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		THROW,

		MATH_ADDITION,
		MATH_SUBSTRACTION,
		MATH_MULTIPLICATION,
		MATH_DIVISION,
		MATH_MODULE,
		MATH_POWER,
		MATH_NEGATIVE,
		MATH_POSITIVE,
		MATH_LEFTSHIFT,
		MATH_RIGHTSHIFT,

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
		/// Pops two values, checks if value B is greater than value A (B > A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_GREATER,
			
		/// <summary>
		/// Pops two values, checks if value B is greater or equal to value A (B >= A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_GREATEROREQUAL,

		/// <summary>
		/// Pops two values, checks if value B is less than value A (B < A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_LESS,

		/// <summary>
		/// Pops two values, checks if value B is less or equal to value A (B <= A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		COMPARE_LESSOREQUAL,

		/// <summary>
		/// Logical NOT operation. Pops boolean/integer, pushes inverted value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOGICAL_NOT,
			
		/// <summary>
		/// Logical OR operation. Pops boolean/integer, pushes inverted value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOGICAL_OR,

		/// <summary>
		/// Logical AND operation. Pops boolean/integer, pushes inverted value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LOGICAL_AND,

		/// <summary>
        /// Creates a new instance of a class/struct defined by TypeSymbol.
        /// <para>Includes 2 parameters :</para>
        /// <para>> TypeSymbol* pType - The type to instantiate.</para>
        /// <para>> ConstructorSymbol* pCtor - The Constructor to invoke.</para>
        /// </summary>
        NEWOBJECT,
			
		/// <summary>
        /// Creates a new instance of a delegate defined by DelegateTypeSymbol.
        /// <para>Includes 2 parameters :</para>
        /// <para>> DelegateTypeSymbol* pType - The delegate type.</para>
        /// </summary>
        NEWDELEGATE,

        /// <summary>
        /// Pops an object instance from the stack, loads a field value from it and pushes gotten instance to stack.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description to resolve offset/name.</para>
        /// </summary>
        LOADFIELD,

        /// <summary>
        /// Pops an object instance and a value, then stores the value into the object's field.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description.</para>
        /// </summary>
        STOREFIELD,

		/// <summary>
        /// Creates a new array of specified type and size. 
        /// Pops size (int) from stack.
        /// <para>Includes 1 parameter :</para>
        /// <para>> TypeSymbol* pElementType - Type of array elements.</para>
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
        /// Creates a new array of specified element type and dynamic size.
        /// Pops size (int) from stack.
        /// <para>Includes 1 parameter :</para>
        /// <para>> TypeSymbol* pElementType - Type of array elements.</para>
        /// </summary>
        NEWDYNAMICARRAY,
	
        /// <summary>
        /// Loads a static field value.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description to resolve offset/name.</para>
        /// </summary>
        LOADSTATICFIELD,

        /// <summary>
        /// Loads an enum field value as a strongly-typed enum instance.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The enum field description.</para>
        /// </summary>
        LOADENUMFIELD,

        /// <summary>
        /// Pops a value, then stores the value into the static field.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description.</para>
        /// </summary>
        STORESTATICFIELD,

		/// <summary>
		/// Invokes MethodSymbol with creation of new CallStackFrame.
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbol* pValue - pointer to MethodSymbol to invoke.</para>
		/// </summary>
		CALLMETHODSYMBOL,

		/// <summary>
		/// Invokes a generic MethodSymbol with creation of new CallStackFrame.
		/// Type arguments must be loaded beforehand with LOAD_TYPEARGUMENT.
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbol* pValue - pointer to MethodSymbol to invoke.</para>
		/// </summary>
		CALLGENERICMETHOD,

		/// <summary>
		/// Invokes the target method stored in a delegate instance.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		CALLDELEGATE,

		/// <summary>
		/// Loads a type argument into the current call frame's TypeArguments vector.
		/// <para>Includes 2 parameters :</para>
		/// <para>> std::uint16_t Index - Zero-based index in TypeArguments.</para>
		/// <para>> TypeSymbol* pType - The concrete type to load.</para>
		/// </summary>
		LOAD_TYPEARGUMENT,

		/// <summary>
		/// Invokes an interface method on the object currently on the evaluation stack.
		/// The concrete implementing method is resolved at runtime from the object's type.
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbol* pInterfaceMethod - pointer to the interface MethodSymbol to invoke.</para>
		/// </summary>
		CALLINTERFACE,

		/// <summary>
		/// Pops an object instance and a target type, checks whether the instance is
		/// compatible with the type, and pushes a boolean result.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - The type to test against.</para>
		/// </summary>
		ISINSTANCE,

		/// <summary>
		/// Pops an object instance and a target type, returns the instance if it is
		/// compatible with the type, or null otherwise.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - The type to cast to.</para>
		/// </summary>
		CASTINTERFACE,

		/// <summary>
		/// Pops an object instance and a target type. Throws an exception if the
		/// instance is not compatible with the target type; otherwise pushes the
		/// same instance.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - The type to cast to.</para>
		/// </summary>
		CAST,

		/// <summary>
		/// Pops an object instance and a target primitive type, converts the value
		/// to the target primitive type, and pushes the new instance.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pType - The primitive type to cast to.</para>
		/// </summary>
		CASTPRIMITIVE,

		/// <summary>
		/// Marks the beginning of a try-protected region. If an exception is raised
		/// inside this region, execution continues at the given handler offset.
		/// <para>Includes 1 parameter :</para>
		/// <para>> std::size_t HandlerOffset - absolute bytecode offset of catch dispatch block.</para>
		/// </summary>
		ENTER_TRY,

		/// <summary>
		/// Marks the end of a try-protected region. Pops the active handler record.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		LEAVE_TRY,

		/// <summary>
		/// Re-throws the exception currently being handled. Must only appear inside a catch block.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		RETHROW,

		/// <summary>
		/// Marks the end of a catch handler. Clears the current exception being handled.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		END_CATCH,

		/// <summary>
		/// Creates a new integer array from a range. Pops upper bound, lower bound and
		/// an inclusive flag, then pushes the populated array.
		/// <para>Includes 1 parameter :</para>
		/// <para>> TypeSymbol* pElementType - Type of array elements.</para>
		/// </summary>
		CREATERANGE
	};
}