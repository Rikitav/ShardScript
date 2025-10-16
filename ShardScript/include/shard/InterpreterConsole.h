#pragma once
#include <shard/parsing/SyntaxTreeParser.h>
#include <shard/parsing/LexicalAnalyzer.h>
#include <shard/syntax/structures/SyntaxTree.h>
#include <memory>

void RunInterpreter(std::shared_ptr<shard::syntax::structures::SyntaxTree> tree, shard::syntax::analysis::DiagnosticsContext& diagnostics, shard::parsing::LexicalAnalyzer& lexer, shard::parsing::SyntaxTreeParser& parser);
