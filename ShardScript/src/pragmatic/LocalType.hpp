#pragma once

#include <shard/semantic/symbols/ParameterSymbol.hpp>
#include <shard/semantic/symbols/VariableSymbol.hpp>

namespace shard
{
	inline StructSymbol* local_type(SyntaxSymbol* local)
	{
		if (local == nullptr)
			return nullptr;

		switch (local->get_kind())
		{
		case SyntaxKind::VariableStatement:
			return static_cast<VariableSymbol*>(local)->get_type();
		case SyntaxKind::Parameter:
			return static_cast<ParameterSymbol*>(local)->get_type();
		default:
			return nullptr;
		}
	}
}
