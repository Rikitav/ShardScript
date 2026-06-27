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
static TypeSymbol* raylib_Input = nullptr;

static EnumSymbol* raylib_color = nullptr;
static EnumSymbol* raylib_configFlags = nullptr;
static EnumSymbol* raylib_keyboardKey = nullptr;
static EnumSymbol* raylib_mouseButton = nullptr;

static inline Color UnpackColor(const ObjectInstance* arg) noexcept
{
    int64_t raw_color = *reinterpret_cast<std::int64_t*>(arg->getMemory());
    Color c{ };
    c.r = static_cast<unsigned char>((raw_color >> 24) & 0xFF);
    c.g = static_cast<unsigned char>((raw_color >> 16) & 0xFF);
    c.b = static_cast<unsigned char>((raw_color >> 8) & 0xFF);
    c.a = static_cast<unsigned char>(raw_color & 0xFF);
    return c;
}

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
    //SetTargetFPS(60);
    delete[] narrow_str;
    return nullptr;
}

static ObjectInstance* shard_graphics_WindowShouldClose(const CallState& context) noexcept
{
    return context.Collector.FromValue(WindowShouldClose());
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
    ClearBackground(UnpackColor(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_input_IsKeyDown(const CallState& context) noexcept
{
    int64_t key = context.Args[0]->AsInteger();
    return context.Collector.FromValue(IsKeyDown(static_cast<int>(key)));
}

static ObjectInstance* shard_input_IsKeyPressed(const CallState& context) noexcept
{
    int64_t key = context.Args[0]->AsInteger();
    return context.Collector.FromValue(IsKeyPressed(static_cast<int>(key)));
}

static ObjectInstance* shard_input_GetMouseX(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<int64_t>(GetMouseX()));
}

static ObjectInstance* shard_input_GetMouseY(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<int64_t>(GetMouseY()));
}

static ObjectInstance* shard_input_IsMouseButtonPressed(const CallState& context) noexcept
{
    int64_t button = context.Args[0]->AsInteger();
    return context.Collector.FromValue(IsMouseButtonPressed(static_cast<int>(button)));
}

static ObjectInstance* shard_graphics_DrawRectangle(const CallState& context) noexcept
{
    int64_t x = context.Args[0]->AsInteger();
    int64_t y = context.Args[1]->AsInteger();
    int64_t w = context.Args[2]->AsInteger();
    int64_t h = context.Args[3]->AsInteger();
    Color color = UnpackColor(context.Args[4]);

    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), color);
    return nullptr;
}

static ObjectInstance* shard_graphics_DrawCircle(const CallState& context) noexcept
{
    int64_t centerX = context.Args[0]->AsInteger();
    int64_t centerY = context.Args[1]->AsInteger();
    double radius = context.Args[2]->AsDouble();
    Color color = UnpackColor(context.Args[3]);

    DrawCircle(static_cast<int>(centerX), static_cast<int>(centerY), static_cast<float>(radius), color);
    return nullptr;
}

static ObjectInstance* shard_graphics_DrawLine(const CallState& context) noexcept
{
    int64_t startX = context.Args[0]->AsInteger();
    int64_t startY = context.Args[1]->AsInteger();
    int64_t endX = context.Args[2]->AsInteger();
    int64_t endY = context.Args[3]->AsInteger();
    Color color = UnpackColor(context.Args[4]);

    DrawLine(static_cast<int>(startX), static_cast<int>(startY), static_cast<int>(endX), static_cast<int>(endY), color);
    return nullptr;
}

static ObjectInstance* shard_graphics_DrawText(const CallState& context) noexcept
{
    const wchar_t* text = context.Args[0]->AsString();
    int64_t x = context.Args[1]->AsInteger();
    int64_t y = context.Args[2]->AsInteger();
    int64_t fontSize = context.Args[3]->AsInteger();
    Color color = UnpackColor(context.Args[4]);

    size_t length = wcslen(text) + 1;
    char* narrow_text = new char[length];
    size_t converted = 0;
    wcstombs_s(&converted, narrow_text, length, text, _TRUNCATE);

    DrawText(narrow_text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(fontSize), color);

    delete[] narrow_text;
    return nullptr;
}

static ObjectInstance* shard_graphics_DrawFPS(const CallState& context) noexcept
{
    int64_t x = context.Args[0]->AsInteger();
    int64_t y = context.Args[1]->AsInteger();
    DrawFPS(static_cast<int>(x), static_cast<int>(y));
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

    raylib_configFlags = raylib.AddEnum(L"ConfigFlags", true)
        .AddValue(L"VsyncHint", static_cast<std::int64_t>(FLAG_VSYNC_HINT))
        .AddValue(L"FullscreenMode", static_cast<std::int64_t>(FLAG_FULLSCREEN_MODE))
        .AddValue(L"WindowResizable", static_cast<std::int64_t>(FLAG_WINDOW_RESIZABLE))
        .AddValue(L"WindowUndecorated", static_cast<std::int64_t>(FLAG_WINDOW_UNDECORATED))
        .AddValue(L"WindowHidden", static_cast<std::int64_t>(FLAG_WINDOW_HIDDEN))
        .AddValue(L"WindowMinimized", static_cast<std::int64_t>(FLAG_WINDOW_MINIMIZED))
        .AddValue(L"WindowMaximized", static_cast<std::int64_t>(FLAG_WINDOW_MAXIMIZED))
        .AddValue(L"WindowUnfocused", static_cast<std::int64_t>(FLAG_WINDOW_UNFOCUSED))
        .AddValue(L"WindowTopmost", static_cast<std::int64_t>(FLAG_WINDOW_TOPMOST))
        .AddValue(L"WindowAlwaysRun", static_cast<std::int64_t>(FLAG_WINDOW_ALWAYS_RUN))
        .AddValue(L"WindowTransparent", static_cast<std::int64_t>(FLAG_WINDOW_TRANSPARENT))
        .AddValue(L"WindowHighDPI", static_cast<std::int64_t>(FLAG_WINDOW_HIGHDPI))
        .AddValue(L"Msaa4xHint", static_cast<std::int64_t>(FLAG_MSAA_4X_HINT));

    raylib_keyboardKey = raylib.AddEnum(L"Keys")
        .AddValue(L"Null", static_cast<std::int64_t>(KEY_NULL))
        .AddValue(L"Apostrophe", static_cast<std::int64_t>(KEY_APOSTROPHE))
        .AddValue(L"Comma", static_cast<std::int64_t>(KEY_COMMA))
        .AddValue(L"Minus", static_cast<std::int64_t>(KEY_MINUS))
        .AddValue(L"Period", static_cast<std::int64_t>(KEY_PERIOD))
        .AddValue(L"Slash", static_cast<std::int64_t>(KEY_SLASH))
        .AddValue(L"Zero", static_cast<std::int64_t>(KEY_ZERO))
        .AddValue(L"One", static_cast<std::int64_t>(KEY_ONE))
        .AddValue(L"Two", static_cast<std::int64_t>(KEY_TWO))
        .AddValue(L"Three", static_cast<std::int64_t>(KEY_THREE))
        .AddValue(L"Four", static_cast<std::int64_t>(KEY_FOUR))
        .AddValue(L"Five", static_cast<std::int64_t>(KEY_FIVE))
        .AddValue(L"Six", static_cast<std::int64_t>(KEY_SIX))
        .AddValue(L"Seven", static_cast<std::int64_t>(KEY_SEVEN))
        .AddValue(L"Eight", static_cast<std::int64_t>(KEY_EIGHT))
        .AddValue(L"Nine", static_cast<std::int64_t>(KEY_NINE))
        .AddValue(L"Semicolon", static_cast<std::int64_t>(KEY_SEMICOLON))
        .AddValue(L"Equal", static_cast<std::int64_t>(KEY_EQUAL))
        .AddValue(L"A", static_cast<std::int64_t>(KEY_A))
        .AddValue(L"B", static_cast<std::int64_t>(KEY_B))
        .AddValue(L"C", static_cast<std::int64_t>(KEY_C))
        .AddValue(L"D", static_cast<std::int64_t>(KEY_D))
        .AddValue(L"E", static_cast<std::int64_t>(KEY_E))
        .AddValue(L"F", static_cast<std::int64_t>(KEY_F))
        .AddValue(L"G", static_cast<std::int64_t>(KEY_G))
        .AddValue(L"H", static_cast<std::int64_t>(KEY_H))
        .AddValue(L"I", static_cast<std::int64_t>(KEY_I))
        .AddValue(L"J", static_cast<std::int64_t>(KEY_J))
        .AddValue(L"K", static_cast<std::int64_t>(KEY_K))
        .AddValue(L"L", static_cast<std::int64_t>(KEY_L))
        .AddValue(L"M", static_cast<std::int64_t>(KEY_M))
        .AddValue(L"N", static_cast<std::int64_t>(KEY_N))
        .AddValue(L"O", static_cast<std::int64_t>(KEY_O))
        .AddValue(L"P", static_cast<std::int64_t>(KEY_P))
        .AddValue(L"Q", static_cast<std::int64_t>(KEY_Q))
        .AddValue(L"E", static_cast<std::int64_t>(KEY_R))
        .AddValue(L"S", static_cast<std::int64_t>(KEY_S))
        .AddValue(L"T", static_cast<std::int64_t>(KEY_T))
        .AddValue(L"U", static_cast<std::int64_t>(KEY_U))
        .AddValue(L"V", static_cast<std::int64_t>(KEY_V))
        .AddValue(L"W", static_cast<std::int64_t>(KEY_W))
        .AddValue(L"X", static_cast<std::int64_t>(KEY_X))
        .AddValue(L"Y", static_cast<std::int64_t>(KEY_Y))
        .AddValue(L"Z", static_cast<std::int64_t>(KEY_Z))
        .AddValue(L"LeftBracket", static_cast<std::int64_t>(KEY_LEFT_BRACKET))
        .AddValue(L"RightBracket", static_cast<std::int64_t>(KEY_RIGHT_BRACKET))
        .AddValue(L"BackSlash", static_cast<std::int64_t>(KEY_BACKSLASH))
        .AddValue(L"Grave", static_cast<std::int64_t>(KEY_GRAVE))
        .AddValue(L"Space", static_cast<std::int64_t>(KEY_SPACE))
        .AddValue(L"Escape", static_cast<std::int64_t>(KEY_ESCAPE))
        .AddValue(L"Enter", static_cast<std::int64_t>(KEY_ENTER))
        .AddValue(L"Tab", static_cast<std::int64_t>(KEY_TAB))
        .AddValue(L"Backspace", static_cast<std::int64_t>(KEY_BACKSPACE))
        .AddValue(L"Insert", static_cast<std::int64_t>(KEY_INSERT))
        .AddValue(L"Del", static_cast<std::int64_t>(KEY_DELETE))
        .AddValue(L"Right", static_cast<std::int64_t>(KEY_RIGHT))
        .AddValue(L"Left", static_cast<std::int64_t>(KEY_LEFT))
        .AddValue(L"Down", static_cast<std::int64_t>(KEY_DOWN))
        .AddValue(L"Up", static_cast<std::int64_t>(KEY_UP))
        .AddValue(L"PageUp", static_cast<std::int64_t>(KEY_PAGE_UP))
        .AddValue(L"PageDown", static_cast<std::int64_t>(KEY_PAGE_DOWN ))
        .AddValue(L"Home", static_cast<std::int64_t>(KEY_HOME))
        .AddValue(L"End", static_cast<std::int64_t>(KEY_END))
        .AddValue(L"CapsLock", static_cast<std::int64_t>(KEY_CAPS_LOCK ))
        .AddValue(L"ScrollLock", static_cast<std::int64_t>(KEY_SCROLL_LOCK))
        .AddValue(L"NumLock", static_cast<std::int64_t>(KEY_NUM_LOCK))
        .AddValue(L"PrintScreen", static_cast<std::int64_t>(KEY_PRINT_SCREEN))
        .AddValue(L"Pause", static_cast<std::int64_t>(KEY_PAUSE))
        .AddValue(L"F1", static_cast<std::int64_t>(KEY_F1))
        .AddValue(L"F2", static_cast<std::int64_t>(KEY_F2))
        .AddValue(L"F3", static_cast<std::int64_t>(KEY_F3))
        .AddValue(L"F4", static_cast<std::int64_t>(KEY_F4))
        .AddValue(L"F5", static_cast<std::int64_t>(KEY_F5))
        .AddValue(L"F6", static_cast<std::int64_t>(KEY_F6))
        .AddValue(L"F7", static_cast<std::int64_t>(KEY_F7))
        .AddValue(L"F8", static_cast<std::int64_t>(KEY_F8))
        .AddValue(L"F9", static_cast<std::int64_t>(KEY_F9))
        .AddValue(L"F10", static_cast<std::int64_t>(KEY_F10))
        .AddValue(L"F11", static_cast<std::int64_t>(KEY_F11))
        .AddValue(L"F12", static_cast<std::int64_t>(KEY_F12))
        .AddValue(L"LeftShist", static_cast<std::int64_t>(KEY_LEFT_SHIFT))
        .AddValue(L"LeftControl", static_cast<std::int64_t>(KEY_LEFT_CONTROL))
        .AddValue(L"LeftAlt", static_cast<std::int64_t>(KEY_LEFT_ALT))
        .AddValue(L"LeftSuper", static_cast<std::int64_t>(KEY_LEFT_SUPER))
        .AddValue(L"RightShift", static_cast<std::int64_t>(KEY_RIGHT_SHIFT))
        .AddValue(L"RightControl", static_cast<std::int64_t>(KEY_RIGHT_CONTROL))
        .AddValue(L"RightAlt", static_cast<std::int64_t>(KEY_RIGHT_ALT ))
        .AddValue(L"RightSuper", static_cast<std::int64_t>(KEY_RIGHT_SUPER))
        .AddValue(L"KbMenu", static_cast<std::int64_t>(KEY_KB_MENU))
        .AddValue(L"KeyPad0", static_cast<std::int64_t>(KEY_KP_0))
        .AddValue(L"KeyPad1", static_cast<std::int64_t>(KEY_KP_1))
        .AddValue(L"KeyPad2", static_cast<std::int64_t>(KEY_KP_2))
        .AddValue(L"KeyPad3", static_cast<std::int64_t>(KEY_KP_3))
        .AddValue(L"KeyPad4", static_cast<std::int64_t>(KEY_KP_4))
        .AddValue(L"KeyPad5", static_cast<std::int64_t>(KEY_KP_5))
        .AddValue(L"KeyPad6", static_cast<std::int64_t>(KEY_KP_6))
        .AddValue(L"KeyPad7", static_cast<std::int64_t>(KEY_KP_7))
        .AddValue(L"KeyPad8", static_cast<std::int64_t>(KEY_KP_8))
        .AddValue(L"KeyPad9", static_cast<std::int64_t>(KEY_KP_9))
        .AddValue(L"Decimal", static_cast<std::int64_t>(KEY_KP_DECIMAL))
        .AddValue(L"Divide", static_cast<std::int64_t>(KEY_KP_DIVIDE))
        .AddValue(L"Multiply", static_cast<std::int64_t>(KEY_KP_MULTIPLY))
        .AddValue(L"Substract", static_cast<std::int64_t>(KEY_KP_SUBTRACT))
        .AddValue(L"Add", static_cast<std::int64_t>(KEY_KP_ADD))
        .AddValue(L"Enter", static_cast<std::int64_t>(KEY_KP_ENTER))
        .AddValue(L"Equal", static_cast<std::int64_t>(KEY_KP_EQUAL))
        .AddValue(L"Back", static_cast<std::int64_t>(KEY_BACK))
        .AddValue(L"Menu", static_cast<std::int64_t>(KEY_MENU))
        .AddValue(L"VolumeUp", static_cast<std::int64_t>(KEY_VOLUME_UP))
        .AddValue(L"VolumeDown", static_cast<std::int64_t>(KEY_VOLUME_DOWN));

    raylib_mouseButton = raylib.AddEnum(L"Mouse")
        .AddValue(L"Left", static_cast<std::int64_t>(MOUSE_BUTTON_LEFT))
        .AddValue(L"Right", static_cast<std::int64_t>(MOUSE_BUTTON_RIGHT))
        .AddValue(L"Middle", static_cast<std::int64_t>(MOUSE_BUTTON_MIDDLE));

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

    raylibWindowBuilder
        .AddMethod(L"DrawCircle", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"centerX", TYPE_INT)
        .AddParameter(L"centerY", TYPE_INT)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_graphics_DrawCircle);

    raylibWindowBuilder
        .AddMethod(L"DrawLine", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"startX", TYPE_INT)
        .AddParameter(L"startY", TYPE_INT)
        .AddParameter(L"endX", TYPE_INT)
        .AddParameter(L"endY", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_graphics_DrawLine);

    raylibWindowBuilder
        .AddMethod(L"DrawText", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"text", TYPE_STRING)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .AddParameter(L"fontSize", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_graphics_DrawText);

    raylibWindowBuilder
        .AddMethod(L"DrawFPS", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .SetCallback(&shard_graphics_DrawFPS);

    // --- class Input ---
    SymbolBuilder<ClassSymbol> raylibInputBuilder = raylib.AddClass(L"Input");
    raylibInputBuilder
        .AddMethod(L"IsKeyDown", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_input_IsKeyDown);

    raylibInputBuilder
        .AddMethod(L"IsKeyPressed", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_input_IsKeyPressed);

    raylibInputBuilder
        .AddMethod(L"IsMouseButtonPressed", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"button", raylib_mouseButton)
        .SetCallback(&shard_input_IsMouseButtonPressed);

    raylibInputBuilder
        .AddMethod(L"GetMouseX", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_input_GetMouseX);

    raylibInputBuilder
        .AddMethod(L"GetMouseY", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_input_GetMouseY);

    raylib_Input = raylibInputBuilder;
    raylib_Window = raylibWindowBuilder;
}