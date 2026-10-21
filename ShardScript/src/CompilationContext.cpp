#include <shard/CompilationContext.hpp>

#include <shard/ApplicationDomain.hpp>
#include <shard/LibraryMetadata.hpp>

#include <shard/compilation/ByteCodeEmitter.hpp>
#include <shard/compilation/VirtualProgramImage.hpp>

#include <shard/lexical/FileReader.hpp>
#include <shard/lexical/LexicalAnalyzer.hpp>
#include <shard/lexical/StringStreamReader.hpp>

#include <shard/parsing/SourceParser.hpp>
#include <shard/parsing/SyntaxToken.hpp>

#include <shard/pragmatic/PragmaticAnalyzer.hpp>
#include <shard/semantic/SemanticAnalyzer.hpp>

#include <shard/parsing/SemanticVersion.hpp>

#include <gmt/Concat.hpp>
#include <gmt/Strings.hpp>

#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>

using namespace shard;

using GetMetadataFunction = void (*)(ShardLibMetadata& lib);
using EntryPointFunction = void (*)(CompilationContext& context);

namespace
{
	struct ResolvedLibrary
	{
		std::filesystem::path path;
		std::wstring name;
		std::wstring description;
		std::wstring version;
		std::vector<std::pair<std::wstring, std::wstring>> dependencies;
	};

	struct LibraryDependency
	{
		const ResolvedLibrary* source = nullptr;
		std::wstring name;
		std::wstring versionExpression;
		const ResolvedLibrary* target = nullptr;
	};

	std::wstring copy_or_empty(const wchar_t* value)
	{
		return value != nullptr ? std::wstring(value) : std::wstring();
	}

	bool try_read_library_metadata(const std::filesystem::path& path, ResolvedLibrary& outLibrary)
	{
		gmt::SharedLibrary library;
		try
		{
			library.load(path);
		}
		catch (...)
		{
			return false;
		}

		GetMetadataFunction getMetadata = library.get_function<GetMetadataFunction>(SHARD_STRINGIFY(SHARDLIB_GETMETADATA_FUNCNAME));
		if (getMetadata == nullptr)
			return false;

		ShardLibMetadata metadata;
		getMetadata(metadata);

		outLibrary.path = path;
		outLibrary.name = copy_or_empty(metadata.Name);
		outLibrary.description = copy_or_empty(metadata.Description);
		outLibrary.version = copy_or_empty(metadata.Version);

		if (metadata.Dependencies != nullptr)
		{
			for (std::size_t i = 0; i < metadata.DependenciesLength; ++i)
			{
				const ShardLibDependencyInfo& dep = metadata.Dependencies[i];
				if (dep.Name == nullptr)
					continue;

				outLibrary.dependencies.emplace_back(copy_or_empty(dep.Name), copy_or_empty(dep.VersionExpression));
			}
		}

		return true;
	}

	std::wstring get_library_display_name(const ResolvedLibrary* lib)
	{
		if (lib == nullptr)
			return L"<unknown>";

		if (lib->name.empty())
			return lib->path.wstring();

		if (lib->version.empty())
			return lib->name;

		return gmt::concat(lib->name, L"@", lib->version);
	}

	bool resolve_library_dependencies(
		const std::vector<ResolvedLibrary>& libraries,
		DiagnosticsContext& diagnostics,
		std::vector<const ResolvedLibrary*>& outLoadOrder)
	{
		// name -> libraries multimap, so multiple versions can coexist
		std::unordered_multimap<std::wstring, const ResolvedLibrary*> librariesByName;
		for (const ResolvedLibrary& lib : libraries)
		{
			if (!lib.name.empty())
				librariesByName.emplace(lib.name, &lib);
		}

		std::vector<LibraryDependency> resolved;
		resolved.reserve(libraries.size() * 2);

		for (const ResolvedLibrary& lib : libraries)
		{
			for (const auto& dep : lib.dependencies)
			{
				const std::wstring& depName = dep.first;
				std::wstring versionExpr = !dep.second.empty() ? dep.second : L"*";

				auto range = librariesByName.equal_range(depName);
				if (range.first == range.second)
				{
					diagnostics.report_error(
						SyntaxToken(),
						gmt::concat(L"Library '", get_library_display_name(&lib), L"' depends on '", depName, L"' which was not found"));

					return false;
				}

				std::optional<semver::VersionExpression> expr;
				if (versionExpr != L"*")
				{
					std::optional<semver::VersionExpression> parsed = semver::VersionExpression::parse(versionExpr);
					if (!parsed.has_value())
					{
						diagnostics.report_error(
							SyntaxToken(),
							gmt::concat(L"Library '", get_library_display_name(&lib), L"' has invalid version expression '", versionExpr, L"' for dependency '", depName, L"'"));

						return false;
					}

					expr = parsed.value();
				}

				const ResolvedLibrary* bestMatch = nullptr;
				for (auto it = range.first; it != range.second; ++it)
				{
					const ResolvedLibrary* candidate = it->second;
					if (expr.has_value())
					{
						const std::wstring& candidateText = !candidate->version.empty() ? candidate->version : L"0.0.0";
						std::optional<semver::SemanticVersion> candidateVersion = semver::parse_version(candidateText);
						if (!candidateVersion.has_value())
							continue;

						if (!expr.value().satisfies(candidateVersion.value()))
							continue;
					}

					if (bestMatch == nullptr)
					{
						bestMatch = candidate;
						continue;
					}

					const std::wstring& bestText = !bestMatch->version.empty() ? bestMatch->version : L"0.0.0";
					const std::wstring& candidateText = !candidate->version.empty() ? candidate->version : L"0.0.0";
					std::optional<semver::SemanticVersion> bestVersion = semver::parse_version(bestText);
					std::optional<semver::SemanticVersion> candidateVersion = semver::parse_version(candidateText);

					if (bestVersion.has_value() && candidateVersion.has_value() && candidateVersion.value() > bestVersion.value())
						bestMatch = candidate;
				}

				if (bestMatch == nullptr)
				{
					diagnostics.report_error(
						SyntaxToken(),
						gmt::concat(L"Library '", get_library_display_name(&lib), L"' requires '", depName, L" ", versionExpr, L"' but no matching version was found"));

					return false;
				}

				resolved.push_back(LibraryDependency{ &lib, depName, versionExpr, bestMatch });
			}
		}

		// topological sort (Kahn's algorithm) over the resolved dependency graph
		std::unordered_map<const ResolvedLibrary*, std::vector<const ResolvedLibrary*>> edges;
		std::unordered_map<const ResolvedLibrary*, std::size_t> inDegree;

		for (const ResolvedLibrary& lib : libraries)
			inDegree[&lib] = 0;

		for (const LibraryDependency& dep : resolved)
		{
			edges[dep.target].push_back(dep.source);
			++inDegree[dep.source];
		}

		std::queue<const ResolvedLibrary*> queue;
		for (const ResolvedLibrary& lib : libraries)
		{
			if (inDegree[&lib] == 0)
				queue.push(&lib);
		}

		std::vector<const ResolvedLibrary*> loadOrder;
		loadOrder.reserve(libraries.size());

		while (!queue.empty())
		{
			const ResolvedLibrary* current = queue.front();
			queue.pop();
			loadOrder.push_back(current);

			auto it = edges.find(current);
			if (it == edges.end())
				continue;

			for (const ResolvedLibrary* dependent : it->second)
			{
				if (--inDegree[dependent] == 0)
					queue.push(dependent);
			}
		}

		if (loadOrder.size() != libraries.size())
		{
			const ResolvedLibrary* cycleNode = nullptr;
			for (const ResolvedLibrary& lib : libraries)
			{
				if (inDegree[&lib] > 0)
				{
					cycleNode = &lib;
					break;
				}
			}

			std::wstring cycleNodeName = get_library_display_name(cycleNode);
			diagnostics.report_error(
				SyntaxToken(),
				gmt::concat(L"Circular library dependency detected involving '", cycleNodeName, L"'"));

			return false;
		}

		outLoadOrder = std::move(loadOrder);
		return true;
	}
}

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

void CompilationContext::add_lib(const std::filesystem::path& path)
{
	add_lib(gmt::SharedLibrary(path));
}

void CompilationContext::add_lib(LibraryHandle handle)
{
	if (handle == nullptr)
		throw std::runtime_error("library handle is null");

	add_lib(gmt::SharedLibrary(handle));
}

void CompilationContext::add_lib(gmt::SharedLibrary library)
{
	if (!library.is_loaded())
		throw std::runtime_error("library is not loaded");

	EntryPointFunction entryPoint = library.get_function<EntryPointFunction>(SHARD_STRINGIFY(SHARDLIB_ENTRYPOINT_FUNCNAME));
	if (entryPoint == nullptr)
	{
		const std::filesystem::path& path = library.get_path();
		if (path.empty())
			throw std::runtime_error("library \"<UNKNOWN>\" does not export ShardLib_EntryPoint");

		throw std::runtime_error("library \"" + gmt::wide_to_utf8(path.wstring()) + "\" does not export ShardLib_EntryPoint");
	}

	// the entry point builds the library's symbols into the table and may
	// inject source through parse_source; library symbols stay owned by this
	// context's semantic model
	entryPoint(*this);
	m_libraries.push_back(std::move(library));
}

void CompilationContext::add_libraries(const std::vector<std::filesystem::path>& paths)
{
	std::vector<ResolvedLibrary> libraries;
	libraries.reserve(paths.size());

	for (const std::filesystem::path& path : paths)
	{
		ResolvedLibrary lib;
		if (!try_read_library_metadata(path, lib))
			continue;

		libraries.push_back(std::move(lib));
	}

	std::vector<const ResolvedLibrary*> loadOrder;
	if (!resolve_library_dependencies(libraries, m_diagnostics, loadOrder))
		return;

	// deduplicate by (name, version) while preserving topological order
	std::set<std::pair<std::wstring, std::wstring>> loaded;
	for (const ResolvedLibrary* lib : loadOrder)
	{
		std::wstring name = !lib->name.empty() ? lib->name : lib->path.wstring();
		std::pair<std::wstring, std::wstring> key = std::make_pair(name, lib->version);
		if (loaded.contains(key))
			continue;

		loaded.insert(key);
		add_lib(lib->path);
	}
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
