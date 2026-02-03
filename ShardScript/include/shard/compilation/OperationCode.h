#pragma once
#include <stdint.h>

namespace shard
{
	enum class OpCode : uint16_t
	{
		/// <summary>
		/// Upon receiving this byte, VM does nothing.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Nop = 1,
		
		/// <summary>
		/// VM immidietly stops execution upon receiving this byte.
		/// <para>Includes 1 parameter :</para>
		/// <para>> int exit_code - exit code of virtual program image
		/// </summary>
		Halt,

		/// <summary>
		/// Pops current top value of stack.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		PopStack,

		/// <summary>
		/// Pops N values from stack. Used for stack cleanup
		/// <para>Includes 1 parameter :</para>
		/// <para>> uin8_t Value - How mush values to pop.</para>
		/// </summary>
		PopStack_N,

		/// <summary>
		/// Loads `boolean` and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> bool Value - bool stored to load.</para>
		/// </summary>
		LoadConst_Boolean,

		/// <summary>
		/// Loads `signed 64-bit integer number` and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> long Value - long stored to load.</para>
		/// </summary>
		LoadConst_Integer64,

		/// <summary>
		/// Loads `signed 64-bit floating point integer`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> double Value - double stored to load.</para>
		/// </summary>
		LoadConst_Rational64,
		
		/// <summary>
		/// Loads `UTF16 character`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> wchar_t Value - UTF16 character stored to load.</para>
		/// </summary>
		LoadConst_Char,
		
		/// <summary>
		/// Loads NULL-terminated UTF16 string from data section, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> const wchar_t* Value - pointer to first symbol of NULL-Terminated UTF16 string stored in Data section to load.</para>
		/// </summary>
		LoadConst_String,

		/// <summary>
		/// Pops ObjectInstance* from stack and stores it to variable slot at given index.
		/// <para>Includes 1 parameter :</para>
		/// <para>> short Value - Zero-based index of variable, where instance will be written to.</para>
		/// </summary>
		StoreVariable,
		
		/// <summary>
		/// Pushes copy of ObjectInstance* from variable slot on given index to stack top.
		/// <para>Includes 1 parameter :</para>
		/// <para>> short Value - Zero-based index of variable, where instance will be readed from.</para>
		/// </summary>
		LoadVariable,

		/// <summary>
		/// Unconditional jump to a relative offset.
		/// <para>Includes 1 parameter :</para>
		/// <para>> size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		Jump,

		/// <summary>
		/// Conditional jump to a relative offset if stack top contains True.
		/// <para>Includes 1 parameter :</para>
		/// <para>> size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		Jump_True,

		/// <summary>
		/// Conditional jump to a relative offset if stack top contains False.
		/// <para>Includes 1 parameter :</para>
		/// <para>> size_t Offset - Relative offset in bytes from current instruction pointer.</para>
		/// </summary>
		Jump_False,

		/// <summary>
		/// Returns from the current method call, destroying the current CallStackFrame.
		/// If the method is non-void, the top of the stack is treated as the return value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Return,

		Math_Addition,
		Math_Substraction,
		Math_Multiplication,
		Math_Division,
		Math_Modification,
		Math_Power,

		/// <summary>
		/// Pops two values, compares them for equality, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Equal,

		/// <summary>
		/// Pops two values, compares them for inequality, pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_NotEqual,

		/// <summary>
		/// Pops two values, checks if value B is greater than value A (B > A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Greater,
			
		/// <summary>
		/// Pops two values, checks if value B is greater or equal to value A (B >= A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_GreaterOrEqual,

		/// <summary>
		/// Pops two values, checks if value B is less than value A (B < A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Less,

		/// <summary>
		/// Pops two values, checks if value B is less or equal to value A (B <= A), pushes boolean result.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_LessOrEqual,

		/// <summary>
		/// Logical NOT operation. Pops boolean/integer, pushes inverted value.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Compare_Not,

		/// <summary>
        /// Creates a new instance of a class/struct defined by TypeSymbol.
        /// <para>Includes 1 parameter :</para>
        /// <para>> TypeSymbol* pType - The type to instantiate.</para>
        /// </summary>
        NewObject,

        /// <summary>
        /// Pops an object instance from the stack, loads a field value from it and pushes gotten instance to stack.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description to resolve offset/name.</para>
        /// </summary>
        LoadField,

        /// <summary>
        /// Pops a value and an object instance, then stores the value into the object's field.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description.</para>
        /// </summary>
        StoreField,

		/// <summary>
        /// Creates a new array of specified type and size. 
        /// Pops size (int) from stack.
        /// <para>Includes 1 parameter :</para>
        /// <para>> TypeSymbol* pElementType - Type of array elements.</para>
        /// </summary>
        NewArray,

        /// <summary>
        /// Loads an element from an array. Pops index and array reference. Pushes element.
        /// <para>Includes no additional parameters.</para>
        /// </summary>
        LoadArrayElement,

        /// <summary>
        /// Stores a value into an array. Pops value, index, and array reference.
        /// <para>Includes no additional parameters.</para>
        /// </summary>
        StoreArrayElement,
	
        /// <summary>
        /// Loads a static field value.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description to resolve offset/name.</para>
        /// </summary>
        LoadStaticField,

        /// <summary>
        /// Pops a value, then stores the value into the static field.
        /// <para>Includes 1 parameter :</para>
        /// <para>> FieldSymbol* pField - The field description.</para>
        /// </summary>
        StoreStaticField,

		/// <summary>
		/// Invokes MethodSymbol with creation of new CallStackFrame.
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbol* pValue - pointer to MethodSymbol to invoke.</para>
		/// </summary>
		CallMethodSymbol,

		/// <summary>
		/// Invokes C-Function from pointer. Function signature should match 'MethodSymbolDelegate' typedef
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbolDelegate* pValue - pointer to C-Function to invoke.</para>
		/// </summary>
		CallFunction,
	};
}