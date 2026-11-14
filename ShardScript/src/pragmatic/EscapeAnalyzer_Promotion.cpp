#include <shard/pragmatic/EscapeAnalyzer.hpp>

using namespace shard;

void EscapeAnalyzer::record_cold_point(int id, const ExpressionSyntax* expression)
{
	m_coldPoints.emplace_back(id, expression);
}
