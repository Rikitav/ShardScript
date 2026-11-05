#include <shard/CompilationState.hpp>

using namespace shard;

CompilationState::CompilationState()
	: m_model(m_tree)
{ }

CompilationState::~CompilationState() = default;
