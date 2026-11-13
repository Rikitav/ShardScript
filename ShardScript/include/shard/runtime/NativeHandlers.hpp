#pragma once

namespace shard
{
	class FuncCallState;
	class AsyncFuncCallState;

	using FuncHandler = void (*)(FuncCallState& state);
	using AsyncFuncHandler = void (*)(AsyncFuncCallState& state);
}
