#include <shard/ShardScriptLIB.hpp>
#include <shard/CompilationContext.hpp>
#include <shard/semantic/SymbolBuilder.hpp>
#include <shard/semantic/SymbolTable.hpp>
#include <shard/runtime/MethodCallState.hpp>
#include <shard/runtime/ObjectInstance.hpp>

#include "include/sqlite3.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
    #include <Windows.h>
#endif

using namespace shard;

TypeSymbol* shard_SqliteConnection = nullptr;
FieldSymbol* shard_SqliteConnection_HandleField = nullptr;

TypeSymbol* shard_SqliteCommand = nullptr;
FieldSymbol* shard_SqliteCommand_ConnField = nullptr;
FieldSymbol* shard_SqliteCommand_TextField = nullptr;

namespace
{
    sqlite3* GetNativeHandle(ObjectInstance* connInstance)
    {
        if (!connInstance)
            return nullptr;

        ObjectInstance* handleVal = connInstance->GetField(shard_SqliteConnection_HandleField);
        if (!handleVal)
            return nullptr;

        return reinterpret_cast<sqlite3*>(handleVal->AsInteger());
    }

    sqlite3* GetOpenDatabase(ObjectInstance* connInstance)
    {
        sqlite3* db = GetNativeHandle(connInstance);
        if (!db)
            throw std::runtime_error("Database connection is closed.");
        return db;
    }

    // RAII wrapper for sqlite3_stmt to guarantee finalization on every exit path.
    class StatementGuard
    {
    public:
        explicit StatementGuard(sqlite3_stmt* stmt) noexcept
            : _stmt(stmt)
        {
        }

        ~StatementGuard()
        {
            if (_stmt)
                sqlite3_finalize(_stmt);
        }

        StatementGuard(const StatementGuard&) = delete;
        StatementGuard& operator=(const StatementGuard&) = delete;

        StatementGuard(StatementGuard&& other) noexcept
            : _stmt(other._stmt)
        {
            other._stmt = nullptr;
        }

        StatementGuard& operator=(StatementGuard&& other) noexcept
        {
            if (this != &other)
            {
                reset(other._stmt);
                other._stmt = nullptr;
            }
            return *this;
        }

        sqlite3_stmt* get() const noexcept { return _stmt; }

        void reset(sqlite3_stmt* stmt = nullptr) noexcept
        {
            if (_stmt)
                sqlite3_finalize(_stmt);
            _stmt = stmt;
        }

    private:
        sqlite3_stmt* _stmt;
    };

    int64_t ExecuteNonQuerySql(sqlite3* db, const std::wstring& sql)
    {
        sqlite3_stmt* rawStmt = nullptr;
        int rc = sqlite3_prepare16_v2(db, sql.c_str(), -1, &rawStmt, nullptr);
        if (rc != SQLITE_OK)
            throw std::runtime_error(std::string("SQLite Prepare Error: ") + sqlite3_errmsg(db));

        StatementGuard stmt(rawStmt);

        rc = sqlite3_step(stmt.get());
        if (rc != SQLITE_DONE && rc != SQLITE_ROW)
            throw std::runtime_error(std::string("SQLite Step Error: ") + sqlite3_errmsg(db));

        return static_cast<int64_t>(sqlite3_changes(db));
    }

    ObjectInstance* ExecuteScalarSql(sqlite3* db, const std::wstring& sql, GarbageCollector& collector)
    {
        sqlite3_stmt* rawStmt = nullptr;
        int rc = sqlite3_prepare16_v2(db, sql.c_str(), -1, &rawStmt, nullptr);
        if (rc != SQLITE_OK)
            throw std::runtime_error(std::string("SQLite Prepare Error: ") + sqlite3_errmsg(db));

        StatementGuard stmt(rawStmt);

        rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_ROW)
        {
            switch (sqlite3_column_type(stmt.get(), 0))
            {
                case SQLITE_INTEGER:
                    return collector.FromValue(static_cast<int64_t>(sqlite3_column_int64(stmt.get(), 0)));

                case SQLITE_FLOAT:
                    return collector.FromValue(sqlite3_column_double(stmt.get(), 0));

                case SQLITE_TEXT:
                {
                    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
                    if (text == nullptr)
                        return collector.NullInstance;

                    std::wstring wide;
#ifdef _WIN32
                    int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
                    if (required > 0)
                    {
                        wide.resize(static_cast<std::size_t>(required - 1));
                        MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), required);
                    }
#else
                    std::size_t len = std::mbstowcs(nullptr, text, 0);
                    if (len != static_cast<std::size_t>(-1))
                    {
                        wide.resize(len);
                        std::mbstowcs(wide.data(), text, len + 1);
                    }
#endif
                    return collector.FromValue(wide);
                }

                case SQLITE_NULL:
                default:
                    return collector.NullInstance;
            }
        }

        if (rc != SQLITE_DONE)
            throw std::runtime_error(std::string("SQLite Step Error: ") + sqlite3_errmsg(db));

        return collector.NullInstance;
    }
}

// ============================================================================
// class SqliteConnection
// ============================================================================

static ObjectInstance* shard_sqlite_Connection_Init(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    std::wstring connectionString = context.Args[1]->AsString();

    sqlite3* db = nullptr;
    int rc = sqlite3_open16(connectionString.c_str(), &db);

    if (rc != SQLITE_OK)
    {
        std::string errDoc = db
            ? sqlite3_errmsg(db)
            : "Failed to open database";

        if (db)
            sqlite3_close_v2(db);

        throw std::runtime_error("SQLite Open Error: " + errDoc);
    }

    // Be tolerant of short-lived locks from other threads/processes.
    sqlite3_busy_timeout(db, 5000);

    instance->SetField(shard_SqliteConnection_HandleField, context.Collector.FromValue(reinterpret_cast<int64_t>(db)));
    return instance;
}

static ObjectInstance* shard_sqlite_Connection_Close(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    sqlite3* db = GetNativeHandle(instance);

    if (db != nullptr)
    {
        sqlite3_close_v2(db);
        instance->SetField(shard_SqliteConnection_HandleField, context.Collector.FromValue(static_cast<int64_t>(0)));
    }

    return nullptr;
}

static ObjectInstance* shard_sqlite_Connection_Dispose(const CallState& context) noexcept
{
    // Conceptually equals to Close.
    return shard_sqlite_Connection_Close(context);
}

static ObjectInstance* shard_sqlite_Connection_ExecuteNonQuery(const CallState& context) noexcept(false)
{
    sqlite3* db = GetOpenDatabase(context.Args[0]);
    std::wstring sql = context.Args[1]->AsString();
    return context.Collector.FromValue(ExecuteNonQuerySql(db, sql));
}

static ObjectInstance* shard_sqlite_Connection_ExecuteScalar(const CallState& context) noexcept(false)
{
    sqlite3* db = GetOpenDatabase(context.Args[0]);
    std::wstring sql = context.Args[1]->AsString();
    return ExecuteScalarSql(db, sql, context.Collector);
}

static ObjectInstance* shard_sqlite_Connection_LastInsertRowId_get(const CallState& context) noexcept(false)
{
    sqlite3* db = GetOpenDatabase(context.Args[0]);
    return context.Collector.FromValue(static_cast<int64_t>(sqlite3_last_insert_rowid(db)));
}

static ObjectInstance* shard_sqlite_Connection_Changes_get(const CallState& context) noexcept(false)
{
    sqlite3* db = GetOpenDatabase(context.Args[0]);
    return context.Collector.FromValue(static_cast<int64_t>(sqlite3_changes(db)));
}

static ObjectInstance* shard_sqlite_Connection_IsOpen_get(const CallState& context) noexcept
{
    sqlite3* db = GetNativeHandle(context.Args[0]);
    return context.Collector.FromValue(db != nullptr);
}

static ObjectInstance* shard_sqlite_Connection_Version_get(const CallState& context) noexcept
{
    const char* ver = sqlite3_libversion();
    std::wstring version(ver, ver + std::strlen(ver));
    return context.Collector.FromValue(version);
}

// ============================================================================
// class SqliteCommand
// ============================================================================

static ObjectInstance* shard_sqlite_Command_Init(const CallState& context) noexcept
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* conn = context.Args[1];
    ObjectInstance* sqlText = context.Args[2];

    instance->SetField(shard_SqliteCommand_ConnField, conn);
    instance->SetField(shard_SqliteCommand_TextField, sqlText);
    return instance;
}

static ObjectInstance* shard_sqlite_Command_ExecuteNonQuery(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* connInstance = instance->GetField(shard_SqliteCommand_ConnField);
    std::wstring sql = instance->GetField(shard_SqliteCommand_TextField)->AsString();

    sqlite3* db = GetOpenDatabase(connInstance);
    return context.Collector.FromValue(ExecuteNonQuerySql(db, sql));
}

static ObjectInstance* shard_sqlite_Command_ExecuteScalar(const CallState& context) noexcept(false)
{
    ObjectInstance* instance = context.Args[0];
    ObjectInstance* connInstance = instance->GetField(shard_SqliteCommand_ConnField);
    std::wstring sql = instance->GetField(shard_SqliteCommand_TextField)->AsString();

    sqlite3* db = GetOpenDatabase(connInstance);
    return ExecuteScalarSql(db, sql, context.Collector);
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.database";
    lib.Description = L"Embedded SQLite3 database provider";
    lib.Version = L"1.1.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> dbNamespace(context, L"Database");

    // --- class SqliteConnection ---
    SymbolBuilder<ClassSymbol> connClass = dbNamespace.AddClass(L"SqliteConnection");
    connClass.Implements(TRAIT_DISPOSABLE);
    shard_SqliteConnection = connClass;

    shard_SqliteConnection_HandleField = connClass
        .AddField(L"_handle", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE);

    connClass.AddInit()
        .AddParameter(L"connectionString", TYPE_STRING)
        .SetCallback(&shard_sqlite_Connection_Init);

    connClass.AddProperty(L"IsOpen", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter().SetCallback(&shard_sqlite_Connection_IsOpen_get);

    connClass.AddProperty(L"LastInsertRowId", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter().SetCallback(&shard_sqlite_Connection_LastInsertRowId_get);

    connClass.AddProperty(L"Changes", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
        .AddGetter().SetCallback(&shard_sqlite_Connection_Changes_get);

    connClass.AddProperty(L"Version", TYPE_STRING, LINK_STATIC, ACS_PUBLIC)
        .AddGetter().SetCallback(&shard_sqlite_Connection_Version_get);

    connClass.AddMethod(L"ExecuteNonQuery", TYPE_INT, LINK_INSTANCE)
        .AddParameter(L"sql", TYPE_STRING)
        .SetCallback(&shard_sqlite_Connection_ExecuteNonQuery);

    connClass.AddMethod(L"ExecuteScalar", SymbolTable::Primitives::Any, LINK_INSTANCE)
        .AddParameter(L"sql", TYPE_STRING)
        .SetCallback(&shard_sqlite_Connection_ExecuteScalar);

    connClass.AddMethod(L"Close", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_sqlite_Connection_Close);

    connClass.AddMethod(L"Dispose", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_sqlite_Connection_Dispose)
        .IsImplementationOf(TRAIT_DISPOSABLE_Dispose);

    // --- class SqliteCommand ---
    SymbolBuilder<ClassSymbol> cmdClass = dbNamespace.AddClass(L"SqliteCommand");
    shard_SqliteCommand = cmdClass;

    shard_SqliteCommand_ConnField = cmdClass
        .AddField(L"_connection", shard_SqliteConnection, LINK_INSTANCE, ACS_PRIVATE);

    shard_SqliteCommand_TextField = cmdClass
        .AddField(L"_commandText", TYPE_STRING, LINK_INSTANCE, ACS_PRIVATE);

    cmdClass.AddInit()
        .AddParameter(L"connection", shard_SqliteConnection)
        .AddParameter(L"commandText", TYPE_STRING)
        .SetCallback(&shard_sqlite_Command_Init);

    cmdClass.AddMethod(L"ExecuteNonQuery", SymbolTable::Primitives::Integer, LINK_INSTANCE)
        .SetCallback(&shard_sqlite_Command_ExecuteNonQuery);

    cmdClass.AddMethod(L"ExecuteScalar", SymbolTable::Primitives::Any, LINK_INSTANCE)
        .SetCallback(&shard_sqlite_Command_ExecuteScalar);
}
