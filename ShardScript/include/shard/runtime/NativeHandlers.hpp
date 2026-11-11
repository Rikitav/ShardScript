#pragma once

namespace shard
{
	class MethodCallState;
	class AsyncFuncCallState;

	using FuncCallState = MethodCallState;
	using FuncHandler = void (*)(FuncCallState& state);
	using AsyncFuncHandler = void (*)(AsyncFuncCallState& state);
	using MethodSymbolDelegate = FuncHandler;
}
