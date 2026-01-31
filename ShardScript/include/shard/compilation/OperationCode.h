#pragma once

namespace shard
{
	enum class OpCode : char
	{
		/// <summary>
		/// Upon receiving this byte, VM does nothing.
		/// <para>Includes no additional parameters.</para>
		/// </summary>
		Nop,
		
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
		/// Loads `signed 64-bit integer number` and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> long Value - long stored to load. Recommended to reference value from Data section.</para>
		/// </summary>
		LoadConst_Integer64,

		/// <summary>
		/// Loads `signed 64-bit floating point integer`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> double Value - double stored to load.</para>
		/// </summary>
		LoadConst_Rational64,
		
		/// <summary>
		/// Loads `UTF16 wide character`, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> wchar_t Value - UTF16 wide character stored to load.</para>
		/// </summary>
		LoadConst_Char,
		
		/// <summary>
		/// Loads NULL-terminated UTF16 wide character string from data section, and pushes its ObjectInstance to stack.
		/// <para>Includes 1 parameter :</para>
		/// <para>> const wchar_t* Value - pointer to first symbol of NULL-Terminated UTF16 wide character string stored in Data section to load.</para>
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
		/// Pushes copy of ObjectInstance* from arguments slot on given index to stack top.
		/// <para>Includes 1 parameter :</para>
		/// <para>> short Value - Zero-based index of argument, where instance will be readed from.</para>
		/// </summary>
		LoadArgument,

		/// <summary>
		/// Invokes MethodSymbol with creation of new CallStackFrame.
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbol* pValue - pointer to MethodSymbol to invoke.</para>
		/// </summary>
		CallSymbol,

		/// <summary>
		/// Invokes C-Function from pointer. Function signature should match 'MethodSymbolDelegate' typedef
		/// <para>Includes 1 parameter :</para>
		/// <para>> MethodSymbolDelegate* pValue - pointer to C-Function to invoke.</para>
		/// </summary>
		CallFunction,

		JumpBackward_True,
		JumpBackward_False,
		JumpForward_True,
		JumpForward_False,

		Math_Addition,
		Math_Substraction,
		Math_Multiplication,
		Math_Division,
		Math_Modification,
		Math_Power,


	};
}