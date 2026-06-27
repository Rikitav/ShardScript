#include <ShardScript.hpp>
#include "include/raylib.h"
#include <string>

using namespace shard;
#define PACK_RGBA(r, g, b, a) static_cast<std::int64_t>( \
    ((static_cast<std::uint64_t>(r) & 0xFF) << 24) | \
    ((static_cast<std::uint64_t>(g) & 0xFF) << 16) | \
    ((static_cast<std::uint64_t>(b) & 0xFF) << 8)  | \
    (static_cast<std::uint64_t>(a) & 0xFF) \
)

static TypeSymbol* raylib_Window = nullptr;
static EnumSymbol* raylib_color = nullptr;

static ObjectInstance* shard_graphics_InitWindow(const CallState& context) noexcept
{
    int64_t width = context.Args[0]->AsInteger();
    int64_t height = context.Args[1]->AsInteger();
    const wchar_t* title = context.Args[2]->AsString();
    
    size_t length = wcslen(title) + 1;
    char* narrow_str = new char[length];

    size_t converted_chars = 0;
    wcstombs_s(&converted_chars, narrow_str, length, title, _TRUNCATE);

    InitWindow(static_cast<int>(width), static_cast<int>(height), narrow_str);
    SetTargetFPS(60);
    delete[] narrow_str;
    return nullptr;
}

static ObjectInstance* shard_graphics_WindowShouldClose(const CallState& context) noexcept
{
    bool shouldClose = WindowShouldClose();
    return context.Collector.FromValue(shouldClose);
}

static ObjectInstance* shard_graphics_BeginDrawing(const CallState& context) noexcept
{
    BeginDrawing();
    return nullptr;
}

static ObjectInstance* shard_graphics_EndDrawing(const CallState& context) noexcept
{
    EndDrawing();
    return nullptr;
}

static ObjectInstance* shard_graphics_CloseWindow(const CallState& context) noexcept
{
    CloseWindow();
    return nullptr;
}

static ObjectInstance* shard_graphics_ClearBackground(const CallState& context) noexcept
{
    int64_t color = *reinterpret_cast<std::int64_t*>(context.Args[0]->getMemory());
    ClearBackground(*reinterpret_cast<Color*>(&color));
    return nullptr;
}

static ObjectInstance* shard_graphics_DrawRectangle(const CallState& context) noexcept
{
    int64_t x = context.Args[0]->AsInteger();
    int64_t y = context.Args[1]->AsInteger();
    int64_t w = context.Args[2]->AsInteger();
    int64_t h = context.Args[3]->AsInteger();
    int64_t color = *reinterpret_cast<std::int64_t*>(context.Args[4]->getMemory());

    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), *reinterpret_cast<Color*>(&color));
    return nullptr;
}

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.raylib";
	lib.Description = L"raylib is a simple and easy-to-use library to enjoy videogames programming.";
	lib.Version = L"0.1.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> raylib(context, L"raylib");

    raylib_color = raylib.AddEnum(L"Colors")
        .AddValue(L"LightGray",  PACK_RGBA(200, 200, 200, 255))
        .AddValue(L"Gray",       PACK_RGBA(130, 130, 130, 255))
        .AddValue(L"DarkGray",   PACK_RGBA(80 , 80 , 80 , 255))
        .AddValue(L"Yellow",     PACK_RGBA(253, 249, 0  , 255))
        .AddValue(L"Gold",       PACK_RGBA(255, 203, 0  , 255))
        .AddValue(L"Orange",     PACK_RGBA(255, 161, 0  , 255))
        .AddValue(L"Pink",       PACK_RGBA(255, 109, 194, 255))
        .AddValue(L"Red",        PACK_RGBA(230, 41 , 55 , 255))
        .AddValue(L"Marron",     PACK_RGBA(190, 33 , 55 , 255))
        .AddValue(L"Green",      PACK_RGBA(0  , 228, 48 , 255))
        .AddValue(L"Lime",       PACK_RGBA(0  , 158, 47 , 255))
        .AddValue(L"DarkGreen",  PACK_RGBA(0  , 117, 44 , 255))
        .AddValue(L"SkyBlue",    PACK_RGBA(102, 191, 255, 255))
        .AddValue(L"Blue",       PACK_RGBA(0  , 121, 241, 255))
        .AddValue(L"DarkBlue",   PACK_RGBA(0  , 82 , 172, 255))
        .AddValue(L"Purple",     PACK_RGBA(200, 122, 255, 255))
        .AddValue(L"Violet",     PACK_RGBA(135, 60 , 190, 255))
        .AddValue(L"DarkPurple", PACK_RGBA(112, 31 , 126, 255))
        .AddValue(L"Beige",      PACK_RGBA(211, 176, 131, 255))
        .AddValue(L"Brown",      PACK_RGBA(127, 106, 79 , 255))
        .AddValue(L"DarkBrown",  PACK_RGBA(76 , 63 , 47 , 255))
        .AddValue(L"White",      PACK_RGBA(255, 255, 255, 255))
        .AddValue(L"Black",      PACK_RGBA(0  , 0  , 0  , 255))
        .AddValue(L"Blank",      PACK_RGBA(0  , 0  , 0  , 0  ))
        .AddValue(L"Magenta",    PACK_RGBA(255, 0  , 255, 255))
        .AddValue(L"RayWhite",   PACK_RGBA(245, 245, 245, 255));

    SymbolBuilder<ClassSymbol> raylibWindowBuilder = raylib.AddClass(L"Window");

    raylibWindowBuilder
        .AddMethod(L"Init", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"title", TYPE_STRING)
        .SetCallback(&shard_graphics_InitWindow);

    raylibWindowBuilder
        .AddMethod(L"ShouldClose", TYPE_BOOL, LINK_STATIC)
        .SetCallback(&shard_graphics_WindowShouldClose);

    raylibWindowBuilder
        .AddMethod(L"BeginDrawing", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_graphics_BeginDrawing);

    raylibWindowBuilder
        .AddMethod(L"EndDrawing", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_graphics_EndDrawing);

    raylibWindowBuilder
        .AddMethod(L"Close", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_graphics_CloseWindow);

    raylibWindowBuilder
        .AddMethod(L"ClearBackground", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_graphics_ClearBackground);

    raylibWindowBuilder
        .AddMethod(L"DrawRectangle", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_graphics_DrawRectangle);

    raylib_Window = raylibWindowBuilder;
}