#include <shard/CompilationContext.hpp>

#include <shard/ApplicationDomain.hpp>

#include <shard/compilation/ByteCodeEmitter.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/lexical/FileReader.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/StringStreamReader.hpp>

#include <shard/parsing/SourceParser.hpp>

#include <shard/pragmatic/PragmaticAnalyzer.hpp>
#include <shard/semantic/SemanticAnalyzer.hpp>

#include <utility>

using namespace shard;

CompilationContext::CompilationContext() :
	m_tree(),
	m_diagnostics(),
	m_model(m_tree),
	m_pragmatic()
{
}

CompilationContext::~CompilationContext() = default;

void CompilationContext::enrich_tree(SourceProvider& provider)
{
	SourceParser parser(m_tree, m_diagnostics);
	parser.FromSourceProvider(provider);
}

void CompilationContext::parse_file(const std::filesystem::path path)
{
	FileReader reader(path);
	LexicalAnalyzer lexer(reader);
	enrich_tree(lexer);
}

void CompilationContext::parse_source(const std::wstring& name, const std::wstring& source)
{
	StringStreamReader reader(name, source);
	LexicalAnalyzer lexer(reader);
	enrich_tree(lexer);
}

bool CompilationContext::analyze_tree()
{
	SemanticAnalyzer semanticAnalyzer(m_diagnostics);
	semanticAnalyzer.analyze(m_tree, m_model);

	PragmaticAnalyzer pragmaticAnalyzer(m_diagnostics);
	pragmaticAnalyzer.analyze(m_tree, m_model, m_pragmatic);
	return !m_diagnostics.has_errors();
}

bool CompilationContext::emit_program()
{
	m_image = std::make_unique<VirtualProgramImage>();

	ByteCodeEmitter emitter(*m_image, m_model, m_pragmatic, m_diagnostics);
	emitter.emit(m_tree);
	return !m_diagnostics.has_errors();
}

std::unique_ptr<ApplicationDomain> CompilationContext::compile()
{
	return std::make_unique<ApplicationDomain>(std::move(m_image), m_model, m_pragmatic);
}

SyntaxTree& CompilationContext::get_syntax_tree()
{
	return m_tree;
}

SemanticModel& CompilationContext::get_semantic_model()
{
	return m_model;
}

PragmaticModel& CompilationContext::get_pragmatic_model()
{
	return m_pragmatic;
}

DiagnosticsContext& CompilationContext::get_diagnostics()
{
	return m_diagnostics;
}

VirtualProgramImage& CompilationContext::get_program()
{
	return *m_image;
}
