#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>
#include <future>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <variant>
#include <vector>

#include <lsp/connection.h>
#include <lsp/io/socket.h>
#include <lsp/io/standardio.h>
#include <lsp/messagehandler.h>
#include <lsp/messages.h>
#include <lsp/types.h>
#include <lsp/fileuri.h>

#include <ShardScript.hpp>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

using namespace shard;
using namespace shard::strings;

static void LogMessage(const std::string& message)
{
    std::cout << message;
}

static void LogException(const char* context, const std::exception& e)
{
    LogMessage(std::string(context) + ": " + e.what());
}

thread_local bool g_running = false;
std::recursive_mutex g_compiler_mutex;
std::unordered_map<std::string, std::wstring> g_open_documents;
std::vector<std::filesystem::path> g_library_paths;

#ifdef _WIN32
static std::filesystem::path GetExecutableDirectory()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
}
#endif

static void scanDirectory(const std::filesystem::path& dir)
{
    if (!std::filesystem::exists(dir))
        return;

    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() != ".dll")
            continue;

        HMODULE hModule = LoadLibraryW(entry.path().c_str());
        if (!hModule)
            continue;

        bool hasEntryPoint = GetProcAddress(hModule, "ShardLib_EntryPoint") != nullptr;
        FreeLibrary(hModule);

        if (!hasEntryPoint)
            continue;

        g_library_paths.push_back(entry.path());
    }
}

static void InitializeLibraries()
{
    if (!g_library_paths.empty())
        return;

    const auto exeDir = GetExecutableDirectory();
    scanDirectory(exeDir / L"system");
    scanDirectory(exeDir / L"third-party");

    LogMessage("Discovered " + std::to_string(g_library_paths.size()) + " ShardScript libraries");
}

static void LoadLibrariesIntoContext(shard::CompilationContext* context)
{
    context->AddLibraries(g_library_paths);
}

static std::wstring ApplyContentChange(const std::wstring& text, const lsp::TextDocumentContentChangeEvent_Range_Text& change)
{
    auto offsetFromPosition = [&text](const lsp::Position& position)
    {
        std::size_t offset = 0;
        std::size_t line = 0;

        while (offset < text.size() && line < position.line)
        {
            if (text[offset] == L'\n')
                ++line;
            ++offset;
        }

        std::size_t character = 0;
        while (offset < text.size() && character < position.character && text[offset] != L'\n')
        {
            ++character;
            ++offset;
        }

        return offset;
    };

    const std::size_t start = offsetFromPosition(change.range.start);
    const std::size_t end = offsetFromPosition(change.range.end);

    return text.substr(0, start) + Utf8ToWide(change.text) + text.substr(end);
}

static lsp::DiagnosticSeverity ToLspSeverity(shard::DiagnosticSeverity severity)
{
    switch (severity)
    {
        case shard::DiagnosticSeverity::Warning:  return lsp::DiagnosticSeverity::Warning;
            case shard::DiagnosticSeverity::Info: return lsp::DiagnosticSeverity::Information;
        case shard::DiagnosticSeverity::Error:
        default:                                  return lsp::DiagnosticSeverity::Error;
    }
}

void RunDiagnostics(lsp::MessageHandler& messageHandler)
{
    std::lock_guard<std::recursive_mutex> lock(g_compiler_mutex);

    auto context = std::make_unique<CompilationContext>();
    LoadLibrariesIntoContext(context.get());

    for (const auto& file : g_open_documents)
    {
        try
        {
            std::wstring wideUri = Utf8ToWide(file.first);
            StringStreamReader reader(wideUri, file.second);
            LexicalAnalyzer lexer(reader);
            context->EnrichTree(lexer, CompilationUnitOrigin::SourceFile);
        }
        catch (...)
        {

        }
    }

    try
    {
        context->AnalyzeTree();
    }
    catch (...)
    {

    }

    const DiagnosticsContext& diagnostics = context->GetDiagnosticsContext();

    std::unordered_map<std::string, std::vector<const Diagnostic*>> diagnosticsByUri;
    for (const auto& err : diagnostics.Diagnostics)
    {
        std::string uri = WideToUtf8(err.Location.FileName.c_str());
        diagnosticsByUri[std::move(uri)].push_back(&err);
    }

    for (const auto& file : g_open_documents)
    {
        const std::string& uri = file.first;
        auto params = lsp::notifications::TextDocument_PublishDiagnostics::Params{};
        params.uri = lsp::Uri::parse(uri);

        auto it = diagnosticsByUri.find(uri);
        if (it != diagnosticsByUri.end())
        {
            for (const Diagnostic* err : it->second)
            {
                lsp::Diagnostic lspErr;
                uint32_t line = err->Location.Line > 0 ? static_cast<uint32_t>(err->Location.Line - 1) : 0u;
                uint32_t col = err->Location.Offset > 0 ? static_cast<uint32_t>(err->Location.Offset - 1) : 0u;
                uint32_t length = err->Location.Length > 0
                    ? static_cast<uint32_t>(err->Location.Length)
                    : static_cast<uint32_t>(err->Token.Word.size());

                lspErr.range = lsp::Range
                {
                    .start = { .line = line, .character = col },
                    .end = { .line = line, .character = col + length }
                };

                lspErr.severity = ToLspSeverity(err->Severity);
                lspErr.source = "ShardScript";
                lspErr.message = WideToUtf8(err->Description.c_str());

                params.diagnostics.push_back(std::move(lspErr));
            }
        }

        messageHandler.sendNotification<lsp::notifications::TextDocument_PublishDiagnostics>(std::move(params));
    }
}

void registerCallbacks(lsp::MessageHandler& messageHandler) {

    messageHandler.add<lsp::requests::Initialize>([](lsp::requests::Initialize::Params&& params)
    {
        (void)params;
        return lsp::requests::Initialize::Result
        {
            .capabilities = lsp::ServerCapabilities
            {
                .positionEncoding = lsp::PositionEncodingKind::UTF16,
                .textDocumentSync = lsp::TextDocumentSyncOptions { .openClose = true, .change = lsp::TextDocumentSyncKind::Full, .save = true },
                .hoverProvider = true, // Включаем всплывающие подсказки
            },

            .serverInfo = lsp::InitializeResultServerInfo
            {
                .name    = "ShardScript LSP Server",
                .version = "1.0.0"
            },
        };
    });

    messageHandler.add<lsp::notifications::TextDocument_DidOpen>([&](lsp::notifications::TextDocument_DidOpen::Params&& params)
    {
        std::lock_guard<std::recursive_mutex> lock(g_compiler_mutex);
        g_open_documents[params.textDocument.uri.toString()] = Utf8ToWide(params.textDocument.text);
        RunDiagnostics(messageHandler);
    });

    messageHandler.add<lsp::notifications::TextDocument_DidChange>([&](lsp::notifications::TextDocument_DidChange::Params&& params) -> void
    {
        if (params.contentChanges.empty())
            return;

        std::lock_guard<std::recursive_mutex> lock(g_compiler_mutex);
        const std::string uri = params.textDocument.uri.toString();
        for (const auto& change : params.contentChanges)
        {
            if (std::holds_alternative<lsp::TextDocumentContentChangeEvent_Text>(change))
            {
                const auto& textChange = std::get<lsp::TextDocumentContentChangeEvent_Text>(change);
                g_open_documents[uri] = Utf8ToWide(textChange.text);
            }
            else if (std::holds_alternative<lsp::TextDocumentContentChangeEvent_Range_Text>(change))
            {
                const auto& rangeChange = std::get<lsp::TextDocumentContentChangeEvent_Range_Text>(change);
                auto it = g_open_documents.find(uri);
                if (it != g_open_documents.end())
                    it->second = ApplyContentChange(it->second, rangeChange);
            }
        }

        RunDiagnostics(messageHandler);
    });

    messageHandler.add<lsp::requests::TextDocument_Hover>([](lsp::requests::TextDocument_Hover::Params&& params) -> std::future<lsp::requests::TextDocument_Hover::Result>
    {
        return std::async(std::launch::deferred, [params = std::move(params)]() -> lsp::requests::TextDocument_Hover::Result
        {
            std::lock_guard<std::recursive_mutex> lock(g_compiler_mutex);
            auto it = g_open_documents.find(params.textDocument.uri.toString());

            if (it == g_open_documents.end())
                return lsp::requests::TextDocument_Hover::Result();

            std::string foundTypeInfo = "Type: processing semantic node...";
            auto hover = lsp::Hover
            {
                .contents = foundTypeInfo
            };

            return lsp::requests::TextDocument_Hover::Result(std::move(hover));
        });
    });

    messageHandler
        .add<lsp::requests::Shutdown>([]() { return lsp::requests::Shutdown::Result(); })
        .add<lsp::notifications::Exit>([]() { g_running = false; });
}

void runLanguageServer(lsp::io::Stream& io)
{
    LogMessage("LSP server started");
    try
    {
        auto connection = lsp::Connection(io);
        auto messageHandler = lsp::MessageHandler(connection);

        registerCallbacks(messageHandler);

        g_running = true;
        while(g_running)
            messageHandler.processIncomingMessages();

    }
    catch(const std::exception& e)
    {
        LogException("LSP Loop Error", e);
        std::cerr << "LSP Loop Error: " << e.what() << std::endl;
    }
}

void runSocketServer(unsigned short port)
{
    InitializeLibraries();
    auto socketListener = lsp::io::SocketListener(port);
    while(socketListener.isReady())
    {
        auto socket = socketListener.listen();
        if(!socket.isOpen())
            break;

        auto thread = std::thread([socket = std::move(socket)]() mutable
        {
            runLanguageServer(socket);
        });

        thread.detach();
    }
}

void runStdioServer()
{
    InitializeLibraries();
    runLanguageServer(lsp::io::standardIO());
}

std::optional<unsigned short> parsePortArg(int argc, char** argv)
{
    constexpr auto PortArg = std::string_view("--port=");
    for(int i = 1; i < argc; ++i)
    {
        const auto arg = std::string_view(argv[i]);
        if(arg.starts_with(PortArg))
        {
            unsigned short port;
            const auto portStr = arg.substr(PortArg.size());
            auto [ptr, ec] = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);

            if(ec == std::errc{})
                return port;
        }
    }

    return std::nullopt;
}

int main(int argc, char** argv)
{
    try
    {
        const auto port = parsePortArg(argc, argv);
        if(!port.has_value())
        {
            runStdioServer();
        }
        else
        {
            runSocketServer(*port);
        }
    }
    catch(const std::exception& e)
    {
        LogException("Fatal Error", e);
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
