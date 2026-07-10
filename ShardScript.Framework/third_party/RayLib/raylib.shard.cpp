#include <ShardScript.hpp>
#include "include/raylib.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace shard;

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

#define PACK_RGBA(r, g, b, a) static_cast<std::int64_t>( \
    ((static_cast<std::uint64_t>(r) & 0xFF) << 24) | \
    ((static_cast<std::uint64_t>(g) & 0xFF) << 16) | \
    ((static_cast<std::uint64_t>(b) & 0xFF) << 8)  | \
    (static_cast<std::uint64_t>(a) & 0xFF) \
)

static inline std::int32_t ArgInt(ObjectInstance* arg) noexcept
{
    return static_cast<std::int32_t>(arg->AsInteger());
}

static inline std::int64_t ArgLong(ObjectInstance* arg) noexcept
{
    return arg->AsInteger();
}

static inline float ArgFloat(ObjectInstance* arg) noexcept
{
    return static_cast<float>(arg->AsDouble());
}

static inline double ArgDouble(ObjectInstance* arg) noexcept
{
    return arg->AsDouble();
}

static inline std::string ArgString(ObjectInstance* arg)
{
    return strings::WideToUtf8(arg->AsString());
}

static inline bool ArgBool(ObjectInstance* arg) noexcept
{
    return arg->AsBoolean();
}

static inline Color UnpackColor(const ObjectInstance* arg) noexcept
{
    std::int64_t raw_color = *reinterpret_cast<std::int64_t*>(arg->getMemory());
    Color c{ };
    c.r = static_cast<unsigned char>((raw_color >> 24) & 0xFF);
    c.g = static_cast<unsigned char>((raw_color >> 16) & 0xFF);
    c.b = static_cast<unsigned char>((raw_color >> 8) & 0xFF);
    c.a = static_cast<unsigned char>(raw_color & 0xFF);
    return c;
}

static inline ObjectInstance* PackColor(GarbageCollector& gc, Color c)
{
    return gc.FromValue(PACK_RGBA(c.r, c.g, c.b, c.a));
}

static inline ObjectInstance* ReturnString(GarbageCollector& gc, const char* text)
{
    if (text == nullptr)
        return gc.FromValue(std::wstring());

    return gc.FromValue(strings::Utf8ToWide(text));
}

// ----------------------------------------------------------------------------
// Type / field symbols
// ----------------------------------------------------------------------------

static TypeSymbol* raylib_Vector2 = nullptr;
static TypeSymbol* raylib_Vector3 = nullptr;
static TypeSymbol* raylib_Rectangle = nullptr;
static TypeSymbol* raylib_Texture = nullptr;
static TypeSymbol* raylib_Image = nullptr;
static TypeSymbol* raylib_Window = nullptr;
static TypeSymbol* raylib_Keyboard = nullptr;
static TypeSymbol* raylib_Mouse = nullptr;
static TypeSymbol* raylib_Gamepad = nullptr;
static TypeSymbol* raylib_Touch = nullptr;
static TypeSymbol* raylib_Gesture = nullptr;
static TypeSymbol* raylib_Cursor = nullptr;
static TypeSymbol* raylib_Time = nullptr;
static TypeSymbol* raylib_Random = nullptr;
static TypeSymbol* raylib_Shapes = nullptr;
static TypeSymbol* raylib_Text = nullptr;
static TypeSymbol* raylib_Sound = nullptr;
static TypeSymbol* raylib_Music = nullptr;
static TypeSymbol* raylib_Math = nullptr;
static TypeSymbol* raylib_Camera3D = nullptr;
static TypeSymbol* raylib_D3 = nullptr;

static EnumSymbol* raylib_color = nullptr;
static EnumSymbol* raylib_configFlags = nullptr;
static EnumSymbol* raylib_keyboardKey = nullptr;
static EnumSymbol* raylib_mouseButton = nullptr;
static EnumSymbol* raylib_mouseCursor = nullptr;
static EnumSymbol* raylib_gamepadButton = nullptr;
static EnumSymbol* raylib_gamepadAxis = nullptr;
static EnumSymbol* raylib_gesture = nullptr;
static EnumSymbol* raylib_blendMode = nullptr;
static EnumSymbol* raylib_traceLogLevel = nullptr;
static EnumSymbol* raylib_cameraMode = nullptr;
static EnumSymbol* raylib_cameraProjection = nullptr;

static FieldSymbol* vector2_X = nullptr;
static FieldSymbol* vector2_Y = nullptr;
static FieldSymbol* vector3_X = nullptr;
static FieldSymbol* vector3_Y = nullptr;
static FieldSymbol* vector3_Z = nullptr;
static FieldSymbol* rectangle_X = nullptr;
static FieldSymbol* rectangle_Y = nullptr;
static FieldSymbol* rectangle_Width = nullptr;
static FieldSymbol* rectangle_Height = nullptr;
static FieldSymbol* texture_Id = nullptr;
static FieldSymbol* texture_Width = nullptr;
static FieldSymbol* texture_Height = nullptr;
static FieldSymbol* texture_Mipmaps = nullptr;
static FieldSymbol* texture_Format = nullptr;
static FieldSymbol* image_Data = nullptr;
static FieldSymbol* image_Width = nullptr;
static FieldSymbol* image_Height = nullptr;
static FieldSymbol* image_Mipmaps = nullptr;
static FieldSymbol* image_Format = nullptr;
static FieldSymbol* sound_Handle = nullptr;
static FieldSymbol* music_Handle = nullptr;
static FieldSymbol* camera3D_Position = nullptr;
static FieldSymbol* camera3D_Target = nullptr;
static FieldSymbol* camera3D_Up = nullptr;
static FieldSymbol* camera3D_Fovy = nullptr;
static FieldSymbol* camera3D_Projection = nullptr;

// ----------------------------------------------------------------------------
// Struct read / write helpers
// ----------------------------------------------------------------------------

static inline Vector2 ReadVector2(ObjectInstance* arg) noexcept
{
    return Vector2{
        ArgFloat(arg->GetField(vector2_X->SlotIndex)),
        ArgFloat(arg->GetField(vector2_Y->SlotIndex))
    };
}

static inline ObjectInstance* WriteVector2(GarbageCollector& gc, Vector2 v)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Vector2);
    obj->SetField(vector2_X->SlotIndex, gc.FromValue(static_cast<double>(v.x)));
    obj->SetField(vector2_Y->SlotIndex, gc.FromValue(static_cast<double>(v.y)));
    return obj;
}

static inline Vector3 ReadVector3(ObjectInstance* arg) noexcept
{
    return Vector3{
        ArgFloat(arg->GetField(vector3_X->SlotIndex)),
        ArgFloat(arg->GetField(vector3_Y->SlotIndex)),
        ArgFloat(arg->GetField(vector3_Z->SlotIndex))
    };
}

static inline ObjectInstance* WriteVector3(GarbageCollector& gc, Vector3 v)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Vector3);
    obj->SetField(vector3_X->SlotIndex, gc.FromValue(static_cast<double>(v.x)));
    obj->SetField(vector3_Y->SlotIndex, gc.FromValue(static_cast<double>(v.y)));
    obj->SetField(vector3_Z->SlotIndex, gc.FromValue(static_cast<double>(v.z)));
    return obj;
}

static inline Rectangle ReadRectangle(ObjectInstance* arg) noexcept
{
    return Rectangle{
        ArgFloat(arg->GetField(rectangle_X->SlotIndex)),
        ArgFloat(arg->GetField(rectangle_Y->SlotIndex)),
        ArgFloat(arg->GetField(rectangle_Width->SlotIndex)),
        ArgFloat(arg->GetField(rectangle_Height->SlotIndex))
    };
}

static inline ObjectInstance* WriteRectangle(GarbageCollector& gc, Rectangle r)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Rectangle);
    obj->SetField(rectangle_X->SlotIndex, gc.FromValue(static_cast<double>(r.x)));
    obj->SetField(rectangle_Y->SlotIndex, gc.FromValue(static_cast<double>(r.y)));
    obj->SetField(rectangle_Width->SlotIndex, gc.FromValue(static_cast<double>(r.width)));
    obj->SetField(rectangle_Height->SlotIndex, gc.FromValue(static_cast<double>(r.height)));
    return obj;
}

static inline Texture2D ReadTexture(ObjectInstance* arg) noexcept
{
    Texture2D tex{};
    tex.id = static_cast<unsigned int>(ArgInt(arg->GetField(texture_Id->SlotIndex)));
    tex.width = ArgInt(arg->GetField(texture_Width->SlotIndex));
    tex.height = ArgInt(arg->GetField(texture_Height->SlotIndex));
    tex.mipmaps = ArgInt(arg->GetField(texture_Mipmaps->SlotIndex));
    tex.format = ArgInt(arg->GetField(texture_Format->SlotIndex));
    return tex;
}

static inline ObjectInstance* WriteTexture(GarbageCollector& gc, Texture2D tex)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Texture);
    obj->SetField(texture_Id->SlotIndex, gc.FromValue(static_cast<std::int64_t>(tex.id)));
    obj->SetField(texture_Width->SlotIndex, gc.FromValue(static_cast<std::int64_t>(tex.width)));
    obj->SetField(texture_Height->SlotIndex, gc.FromValue(static_cast<std::int64_t>(tex.height)));
    obj->SetField(texture_Mipmaps->SlotIndex, gc.FromValue(static_cast<std::int64_t>(tex.mipmaps)));
    obj->SetField(texture_Format->SlotIndex, gc.FromValue(static_cast<std::int64_t>(tex.format)));
    return obj;
}

static inline Image ReadImage(ObjectInstance* arg) noexcept
{
    Image img{};
    img.data = reinterpret_cast<void*>(static_cast<std::uintptr_t>(ArgLong(arg->GetField(image_Data->SlotIndex))));
    img.width = ArgInt(arg->GetField(image_Width->SlotIndex));
    img.height = ArgInt(arg->GetField(image_Height->SlotIndex));
    img.mipmaps = ArgInt(arg->GetField(image_Mipmaps->SlotIndex));
    img.format = ArgInt(arg->GetField(image_Format->SlotIndex));
    return img;
}

static inline ObjectInstance* WriteImage(GarbageCollector& gc, Image img)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Image);
    obj->SetField(image_Data->SlotIndex, gc.FromValue(static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(img.data))));
    obj->SetField(image_Width->SlotIndex, gc.FromValue(static_cast<std::int64_t>(img.width)));
    obj->SetField(image_Height->SlotIndex, gc.FromValue(static_cast<std::int64_t>(img.height)));
    obj->SetField(image_Mipmaps->SlotIndex, gc.FromValue(static_cast<std::int64_t>(img.mipmaps)));
    obj->SetField(image_Format->SlotIndex, gc.FromValue(static_cast<std::int64_t>(img.format)));
    return obj;
}

static inline ::Sound* ReadSound(ObjectInstance* arg) noexcept
{
    return reinterpret_cast<::Sound*>(static_cast<std::uintptr_t>(ArgLong(arg->GetField(sound_Handle->SlotIndex))));
}

static inline ObjectInstance* WriteSound(GarbageCollector& gc, ::Sound sound)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Sound);
    ::Sound* ptr = new ::Sound(sound);
    obj->SetField(sound_Handle->SlotIndex, gc.FromValue(static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(ptr))));
    return obj;
}

static inline ::Music* ReadMusic(ObjectInstance* arg) noexcept
{
    return reinterpret_cast<::Music*>(static_cast<std::uintptr_t>(ArgLong(arg->GetField(music_Handle->SlotIndex))));
}

static inline ObjectInstance* WriteMusic(GarbageCollector& gc, ::Music music)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Music);
    ::Music* ptr = new ::Music(music);
    obj->SetField(music_Handle->SlotIndex, gc.FromValue(static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(ptr))));
    return obj;
}

static inline Camera3D ReadCamera3D(ObjectInstance* arg) noexcept
{
    Camera3D cam{};
    cam.position = ReadVector3(arg->GetField(camera3D_Position->SlotIndex));
    cam.target = ReadVector3(arg->GetField(camera3D_Target->SlotIndex));
    cam.up = ReadVector3(arg->GetField(camera3D_Up->SlotIndex));
    cam.fovy = ArgFloat(arg->GetField(camera3D_Fovy->SlotIndex));
    cam.projection = ArgInt(arg->GetField(camera3D_Projection->SlotIndex));
    return cam;
}

static inline ObjectInstance* WriteCamera3D(GarbageCollector& gc, Camera3D cam)
{
    ObjectInstance* obj = gc.AllocateInstance(raylib_Camera3D);
    obj->SetField(camera3D_Position->SlotIndex, WriteVector3(gc, cam.position));
    obj->SetField(camera3D_Target->SlotIndex, WriteVector3(gc, cam.target));
    obj->SetField(camera3D_Up->SlotIndex, WriteVector3(gc, cam.up));
    obj->SetField(camera3D_Fovy->SlotIndex, gc.FromValue(static_cast<double>(cam.fovy)));
    obj->SetField(camera3D_Projection->SlotIndex, gc.FromValue(static_cast<std::int64_t>(cam.projection)));
    return obj;
}

static ObjectInstance* shard_camera3D_Init(const CallState& context) noexcept
{
    ObjectInstance* self = context.Args[0];

    self->SetField(camera3D_Position->SlotIndex, WriteVector3(context.Collector, ReadVector3(context.Args[1])));
    self->SetField(camera3D_Target->SlotIndex, WriteVector3(context.Collector, ReadVector3(context.Args[2])));
    self->SetField(camera3D_Up->SlotIndex, WriteVector3(context.Collector, ReadVector3(context.Args[3])));
    self->SetField(camera3D_Fovy->SlotIndex, context.Collector.FromValue(ArgDouble(context.Args[4])));
    self->SetField(camera3D_Projection->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(ArgInt(context.Args[5]))));

    return nullptr;
}

// ----------------------------------------------------------------------------
// Struct factories (callbacks registered in the entry point)
// ----------------------------------------------------------------------------

static ObjectInstance* shard_vector2_Create(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, Vector2{ ArgFloat(context.Args[0]), ArgFloat(context.Args[1]) });
}

static ObjectInstance* shard_vector3_Create(const CallState& context) noexcept
{
    return WriteVector3(context.Collector, Vector3{ ArgFloat(context.Args[0]), ArgFloat(context.Args[1]), ArgFloat(context.Args[2]) });
}

static ObjectInstance* shard_rectangle_Create(const CallState& context) noexcept
{
    return WriteRectangle(context.Collector, Rectangle{
        ArgFloat(context.Args[0]),
        ArgFloat(context.Args[1]),
        ArgFloat(context.Args[2]),
        ArgFloat(context.Args[3])
    });
}

// ----------------------------------------------------------------------------
// Window
// ----------------------------------------------------------------------------

static ObjectInstance* shard_window_InitWindow(const CallState& context) noexcept
{
    std::int32_t width = ArgInt(context.Args[0]);
    std::int32_t height = ArgInt(context.Args[1]);
    std::string narrow_str = ArgString(context.Args[2]);

    InitWindow(width, height, narrow_str.c_str());
    return nullptr;
}

static ObjectInstance* shard_window_WindowShouldClose(const CallState& context) noexcept
{
    return context.Collector.FromValue(WindowShouldClose());
}

static ObjectInstance* shard_window_BeginDrawing(const CallState& context) noexcept
{
    BeginDrawing();
    return nullptr;
}

static ObjectInstance* shard_window_EndDrawing(const CallState& context) noexcept
{
    EndDrawing();
    return nullptr;
}

static ObjectInstance* shard_window_CloseWindow(const CallState& context) noexcept
{
    CloseWindow();
    return nullptr;
}

static ObjectInstance* shard_window_ClearBackground(const CallState& context) noexcept
{
    ClearBackground(UnpackColor(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_window_DrawFPS(const CallState& context) noexcept
{
    DrawFPS(ArgInt(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_window_IsWindowReady(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsWindowReady());
}

static ObjectInstance* shard_window_IsWindowFullscreen(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsWindowFullscreen());
}

static ObjectInstance* shard_window_SetWindowState(const CallState& context) noexcept
{
    SetWindowState(static_cast<unsigned int>(ArgLong(context.Args[0])));
    return nullptr;
}

static ObjectInstance* shard_window_ClearWindowState(const CallState& context) noexcept
{
    ClearWindowState(static_cast<unsigned int>(ArgLong(context.Args[0])));
    return nullptr;
}

static ObjectInstance* shard_window_ToggleFullscreen(const CallState& context) noexcept
{
    ToggleFullscreen();
    return nullptr;
}

static ObjectInstance* shard_window_MaximizeWindow(const CallState& context) noexcept
{
    MaximizeWindow();
    return nullptr;
}

static ObjectInstance* shard_window_MinimizeWindow(const CallState& context) noexcept
{
    MinimizeWindow();
    return nullptr;
}

static ObjectInstance* shard_window_RestoreWindow(const CallState& context) noexcept
{
    RestoreWindow();
    return nullptr;
}

static ObjectInstance* shard_window_SetWindowTitle(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    SetWindowTitle(narrow_str.c_str());
    return nullptr;
}

static ObjectInstance* shard_window_SetWindowPosition(const CallState& context) noexcept
{
    SetWindowPosition(ArgInt(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_window_SetWindowMonitor(const CallState& context) noexcept
{
    SetWindowMonitor(ArgInt(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_window_SetWindowMinSize(const CallState& context) noexcept
{
    SetWindowMinSize(ArgInt(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_window_SetWindowMaxSize(const CallState& context) noexcept
{
    SetWindowMaxSize(ArgInt(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_window_SetWindowSize(const CallState& context) noexcept
{
    SetWindowSize(ArgInt(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_window_SetWindowOpacity(const CallState& context) noexcept
{
    SetWindowOpacity(ArgFloat(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_window_GetScreenWidth(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetScreenWidth()));
}

static ObjectInstance* shard_window_GetScreenHeight(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetScreenHeight()));
}

static ObjectInstance* shard_window_GetRenderWidth(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetRenderWidth()));
}

static ObjectInstance* shard_window_GetRenderHeight(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetRenderHeight()));
}

static ObjectInstance* shard_window_GetMonitorCount(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetMonitorCount()));
}

static ObjectInstance* shard_window_GetCurrentMonitor(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetCurrentMonitor()));
}

static ObjectInstance* shard_window_GetMonitorWidth(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetMonitorWidth(ArgInt(context.Args[0]))));
}

static ObjectInstance* shard_window_GetMonitorHeight(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetMonitorHeight(ArgInt(context.Args[0]))));
}

static ObjectInstance* shard_window_GetMonitorRefreshRate(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetMonitorRefreshRate(ArgInt(context.Args[0]))));
}

static ObjectInstance* shard_window_GetWindowPosition(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetWindowPosition());
}

static ObjectInstance* shard_window_GetWindowScaleDPI(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetWindowScaleDPI());
}

static ObjectInstance* shard_window_GetMonitorName(const CallState& context) noexcept
{
    return ReturnString(context.Collector, GetMonitorName(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_window_SetClipboardText(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    SetClipboardText(narrow_str.c_str());
    return nullptr;
}

static ObjectInstance* shard_window_GetClipboardText(const CallState& context) noexcept
{
    return ReturnString(context.Collector, GetClipboardText());
}

static ObjectInstance* shard_window_EnableEventWaiting(const CallState& context) noexcept
{
    EnableEventWaiting();
    return nullptr;
}

static ObjectInstance* shard_window_DisableEventWaiting(const CallState& context) noexcept
{
    DisableEventWaiting();
    return nullptr;
}

// ----------------------------------------------------------------------------
// Cursor
// ----------------------------------------------------------------------------

static ObjectInstance* shard_cursor_ShowCursor(const CallState& context) noexcept
{
    ShowCursor();
    return nullptr;
}

static ObjectInstance* shard_cursor_HideCursor(const CallState& context) noexcept
{
    HideCursor();
    return nullptr;
}

static ObjectInstance* shard_cursor_IsCursorHidden(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsCursorHidden());
}

static ObjectInstance* shard_cursor_EnableCursor(const CallState& context) noexcept
{
    EnableCursor();
    return nullptr;
}

static ObjectInstance* shard_cursor_DisableCursor(const CallState& context) noexcept
{
    DisableCursor();
    return nullptr;
}

static ObjectInstance* shard_cursor_IsCursorOnScreen(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsCursorOnScreen());
}

// ----------------------------------------------------------------------------
// Time
// ----------------------------------------------------------------------------

static ObjectInstance* shard_time_SetTargetFPS(const CallState& context) noexcept
{
    SetTargetFPS(ArgInt(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_time_GetFPS(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetFPS()));
}

static ObjectInstance* shard_time_GetFrameTime(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<double>(GetFrameTime()));
}

static ObjectInstance* shard_time_GetTime(const CallState& context) noexcept
{
    return context.Collector.FromValue(GetTime());
}

// ----------------------------------------------------------------------------
// Random
// ----------------------------------------------------------------------------

static ObjectInstance* shard_random_SetRandomSeed(const CallState& context) noexcept
{
    SetRandomSeed(static_cast<unsigned int>(ArgLong(context.Args[0])));
    return nullptr;
}

static ObjectInstance* shard_random_GetRandomValue(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetRandomValue(ArgInt(context.Args[0]), ArgInt(context.Args[1]))));
}

// ----------------------------------------------------------------------------
// Keyboard
// ----------------------------------------------------------------------------

static ObjectInstance* shard_keyboard_IsKeyDown(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsKeyDown(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_keyboard_IsKeyPressed(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsKeyPressed(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_keyboard_IsKeyPressedRepeat(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsKeyPressedRepeat(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_keyboard_IsKeyReleased(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsKeyReleased(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_keyboard_IsKeyUp(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsKeyUp(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_keyboard_GetKeyPressed(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetKeyPressed()));
}

static ObjectInstance* shard_keyboard_GetCharPressed(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetCharPressed()));
}

static ObjectInstance* shard_keyboard_SetExitKey(const CallState& context) noexcept
{
    SetExitKey(ArgInt(context.Args[0]));
    return nullptr;
}

// ----------------------------------------------------------------------------
// Mouse
// ----------------------------------------------------------------------------

static ObjectInstance* shard_mouse_IsMouseButtonPressed(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsMouseButtonPressed(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_mouse_IsMouseButtonDown(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsMouseButtonDown(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_mouse_IsMouseButtonReleased(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsMouseButtonReleased(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_mouse_IsMouseButtonUp(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsMouseButtonUp(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_mouse_GetMouseX(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetMouseX()));
}

static ObjectInstance* shard_mouse_GetMouseY(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetMouseY()));
}

static ObjectInstance* shard_mouse_GetMousePosition(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetMousePosition());
}

static ObjectInstance* shard_mouse_GetMouseDelta(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetMouseDelta());
}

static ObjectInstance* shard_mouse_SetMousePosition(const CallState& context) noexcept
{
    SetMousePosition(ArgInt(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_mouse_SetMouseOffset(const CallState& context) noexcept
{
    SetMouseOffset(ArgInt(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_mouse_SetMouseScale(const CallState& context) noexcept
{
    SetMouseScale(ArgFloat(context.Args[0]), ArgFloat(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_mouse_GetMouseWheelMove(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<double>(GetMouseWheelMove()));
}

static ObjectInstance* shard_mouse_GetMouseWheelMoveV(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetMouseWheelMoveV());
}

static ObjectInstance* shard_mouse_SetMouseCursor(const CallState& context) noexcept
{
    SetMouseCursor(ArgInt(context.Args[0]));
    return nullptr;
}

// ----------------------------------------------------------------------------
// Gamepad
// ----------------------------------------------------------------------------

static ObjectInstance* shard_gamepad_IsGamepadAvailable(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsGamepadAvailable(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_gamepad_GetGamepadName(const CallState& context) noexcept
{
    return ReturnString(context.Collector, GetGamepadName(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_gamepad_IsGamepadButtonPressed(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsGamepadButtonPressed(ArgInt(context.Args[0]), ArgInt(context.Args[1])));
}

static ObjectInstance* shard_gamepad_IsGamepadButtonDown(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsGamepadButtonDown(ArgInt(context.Args[0]), ArgInt(context.Args[1])));
}

static ObjectInstance* shard_gamepad_IsGamepadButtonReleased(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsGamepadButtonReleased(ArgInt(context.Args[0]), ArgInt(context.Args[1])));
}

static ObjectInstance* shard_gamepad_IsGamepadButtonUp(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsGamepadButtonUp(ArgInt(context.Args[0]), ArgInt(context.Args[1])));
}

static ObjectInstance* shard_gamepad_GetGamepadButtonPressed(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetGamepadButtonPressed()));
}

static ObjectInstance* shard_gamepad_GetGamepadAxisCount(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetGamepadAxisCount(ArgInt(context.Args[0]))));
}

static ObjectInstance* shard_gamepad_GetGamepadAxisMovement(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<double>(GetGamepadAxisMovement(ArgInt(context.Args[0]), ArgInt(context.Args[1]))));
}

// ----------------------------------------------------------------------------
// Touch
// ----------------------------------------------------------------------------

static ObjectInstance* shard_touch_GetTouchX(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetTouchX()));
}

static ObjectInstance* shard_touch_GetTouchY(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetTouchY()));
}

static ObjectInstance* shard_touch_GetTouchPosition(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetTouchPosition(ArgInt(context.Args[0])));
}

static ObjectInstance* shard_touch_GetTouchPointId(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetTouchPointId(ArgInt(context.Args[0]))));
}

static ObjectInstance* shard_touch_GetTouchPointCount(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetTouchPointCount()));
}

// ----------------------------------------------------------------------------
// Gesture
// ----------------------------------------------------------------------------

static ObjectInstance* shard_gesture_SetGesturesEnabled(const CallState& context) noexcept
{
    SetGesturesEnabled(static_cast<unsigned int>(ArgLong(context.Args[0])));
    return nullptr;
}

static ObjectInstance* shard_gesture_IsGestureDetected(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsGestureDetected(static_cast<unsigned int>(ArgLong(context.Args[0]))));
}

static ObjectInstance* shard_gesture_GetGestureDetected(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<std::int64_t>(GetGestureDetected()));
}

static ObjectInstance* shard_gesture_GetGestureHoldDuration(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<double>(GetGestureHoldDuration()));
}

static ObjectInstance* shard_gesture_GetGestureDragVector(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetGestureDragVector());
}

static ObjectInstance* shard_gesture_GetGestureDragAngle(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<double>(GetGestureDragAngle()));
}

static ObjectInstance* shard_gesture_GetGesturePinchVector(const CallState& context) noexcept
{
    return WriteVector2(context.Collector, GetGesturePinchVector());
}

static ObjectInstance* shard_gesture_GetGesturePinchAngle(const CallState& context) noexcept
{
    return context.Collector.FromValue(static_cast<double>(GetGesturePinchAngle()));
}

// ----------------------------------------------------------------------------
// Shapes
// ----------------------------------------------------------------------------

static ObjectInstance* shard_shapes_DrawPixel(const CallState& context) noexcept
{
    DrawPixel(ArgInt(context.Args[0]), ArgInt(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawPixelV(const CallState& context) noexcept
{
    DrawPixelV(ReadVector2(context.Args[0]), UnpackColor(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawLine(const CallState& context) noexcept
{
    DrawLine(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgInt(context.Args[2]), ArgInt(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawLineV(const CallState& context) noexcept
{
    DrawLineV(ReadVector2(context.Args[0]), ReadVector2(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawLineEx(const CallState& context) noexcept
{
    DrawLineEx(ReadVector2(context.Args[0]), ReadVector2(context.Args[1]), ArgFloat(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawLineBezier(const CallState& context) noexcept
{
    DrawLineBezier(ReadVector2(context.Args[0]), ReadVector2(context.Args[1]), ArgFloat(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawCircle(const CallState& context) noexcept
{
    DrawCircle(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgFloat(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawCircleV(const CallState& context) noexcept
{
    DrawCircleV(ReadVector2(context.Args[0]), ArgFloat(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawCircleGradient(const CallState& context) noexcept
{
    DrawCircleGradient(ReadVector2(context.Args[0]), ArgFloat(context.Args[1]), UnpackColor(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawCircleSector(const CallState& context) noexcept
{
    DrawCircleSector(ReadVector2(context.Args[0]), ArgFloat(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), ArgInt(context.Args[4]), UnpackColor(context.Args[5]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawCircleSectorLines(const CallState& context) noexcept
{
    DrawCircleSectorLines(ReadVector2(context.Args[0]), ArgFloat(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), ArgInt(context.Args[4]), UnpackColor(context.Args[5]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawCircleLines(const CallState& context) noexcept
{
    DrawCircleLines(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgFloat(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawCircleLinesV(const CallState& context) noexcept
{
    DrawCircleLinesV(ReadVector2(context.Args[0]), ArgFloat(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawEllipse(const CallState& context) noexcept
{
    DrawEllipse(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawEllipseLines(const CallState& context) noexcept
{
    DrawEllipseLines(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRing(const CallState& context) noexcept
{
    DrawRing(ReadVector2(context.Args[0]), ArgFloat(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), ArgFloat(context.Args[4]), ArgInt(context.Args[5]), UnpackColor(context.Args[6]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRingLines(const CallState& context) noexcept
{
    DrawRingLines(ReadVector2(context.Args[0]), ArgFloat(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), ArgFloat(context.Args[4]), ArgInt(context.Args[5]), UnpackColor(context.Args[6]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangle(const CallState& context) noexcept
{
    DrawRectangle(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgInt(context.Args[2]), ArgInt(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleV(const CallState& context) noexcept
{
    DrawRectangleV(ReadVector2(context.Args[0]), ReadVector2(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleRec(const CallState& context) noexcept
{
    DrawRectangleRec(ReadRectangle(context.Args[0]), UnpackColor(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectanglePro(const CallState& context) noexcept
{
    DrawRectanglePro(ReadRectangle(context.Args[0]), ReadVector2(context.Args[1]), ArgFloat(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleGradientV(const CallState& context) noexcept
{
    DrawRectangleGradientV(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgInt(context.Args[2]), ArgInt(context.Args[3]), UnpackColor(context.Args[4]), UnpackColor(context.Args[5]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleGradientH(const CallState& context) noexcept
{
    DrawRectangleGradientH(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgInt(context.Args[2]), ArgInt(context.Args[3]), UnpackColor(context.Args[4]), UnpackColor(context.Args[5]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleGradientEx(const CallState& context) noexcept
{
    DrawRectangleGradientEx(ReadRectangle(context.Args[0]), UnpackColor(context.Args[1]), UnpackColor(context.Args[2]), UnpackColor(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleLines(const CallState& context) noexcept
{
    DrawRectangleLines(ArgInt(context.Args[0]), ArgInt(context.Args[1]), ArgInt(context.Args[2]), ArgInt(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleLinesEx(const CallState& context) noexcept
{
    DrawRectangleLinesEx(ReadRectangle(context.Args[0]), ArgFloat(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleRounded(const CallState& context) noexcept
{
    DrawRectangleRounded(ReadRectangle(context.Args[0]), ArgFloat(context.Args[1]), ArgInt(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawRectangleRoundedLines(const CallState& context) noexcept
{
    DrawRectangleRoundedLines(ReadRectangle(context.Args[0]), ArgFloat(context.Args[1]), ArgInt(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawTriangle(const CallState& context) noexcept
{
    DrawTriangle(ReadVector2(context.Args[0]), ReadVector2(context.Args[1]), ReadVector2(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawTriangleLines(const CallState& context) noexcept
{
    DrawTriangleLines(ReadVector2(context.Args[0]), ReadVector2(context.Args[1]), ReadVector2(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawPoly(const CallState& context) noexcept
{
    DrawPoly(ReadVector2(context.Args[0]), ArgInt(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawPolyLines(const CallState& context) noexcept
{
    DrawPolyLines(ReadVector2(context.Args[0]), ArgInt(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawPolyLinesEx(const CallState& context) noexcept
{
    DrawPolyLinesEx(ReadVector2(context.Args[0]), ArgInt(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), ArgFloat(context.Args[4]), UnpackColor(context.Args[5]));
    return nullptr;
}

static ObjectInstance* shard_shapes_DrawText(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    DrawText(narrow_str.c_str(), ArgInt(context.Args[1]), ArgInt(context.Args[2]), ArgInt(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

// ----------------------------------------------------------------------------
// Text
// ----------------------------------------------------------------------------

static ObjectInstance* shard_text_MeasureText(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    return context.Collector.FromValue(static_cast<std::int64_t>(MeasureText(narrow_str.c_str(), ArgInt(context.Args[1]))));
}

static ObjectInstance* shard_text_SetTextLineSpacing(const CallState& context) noexcept
{
    SetTextLineSpacing(ArgInt(context.Args[0]));
    return nullptr;
}

// ----------------------------------------------------------------------------
// Texture
// ----------------------------------------------------------------------------

static ObjectInstance* shard_texture_LoadTexture(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    Texture2D tex = LoadTexture(narrow_str.c_str());
    return WriteTexture(context.Collector, tex);
}

static ObjectInstance* shard_texture_LoadTextureFromImage(const CallState& context) noexcept
{
    Texture2D tex = LoadTextureFromImage(ReadImage(context.Args[0]));
    return WriteTexture(context.Collector, tex);
}

static ObjectInstance* shard_texture_UnloadTexture(const CallState& context) noexcept
{
    UnloadTexture(ReadTexture(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_texture_DrawTexture(const CallState& context) noexcept
{
    DrawTexture(ReadTexture(context.Args[0]), ArgInt(context.Args[1]), ArgInt(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_texture_DrawTextureV(const CallState& context) noexcept
{
    DrawTextureV(ReadTexture(context.Args[0]), ReadVector2(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_texture_DrawTextureEx(const CallState& context) noexcept
{
    DrawTextureEx(ReadTexture(context.Args[0]), ReadVector2(context.Args[1]), ArgFloat(context.Args[2]), ArgFloat(context.Args[3]), UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_texture_DrawTextureRec(const CallState& context) noexcept
{
    DrawTextureRec(ReadTexture(context.Args[0]), ReadRectangle(context.Args[1]), ReadVector2(context.Args[2]), UnpackColor(context.Args[3]));
    return nullptr;
}

static ObjectInstance* shard_texture_DrawTexturePro(const CallState& context) noexcept
{
    DrawTexturePro(ReadTexture(context.Args[0]), ReadRectangle(context.Args[1]), ReadRectangle(context.Args[2]), ReadVector2(context.Args[3]), ArgFloat(context.Args[4]), UnpackColor(context.Args[5]));
    return nullptr;
}

static ObjectInstance* shard_texture_IsTextureValid(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsTextureValid(ReadTexture(context.Args[0])));
}

static ObjectInstance* shard_texture_SetTextureFilter(const CallState& context) noexcept
{
    SetTextureFilter(ReadTexture(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_texture_SetTextureWrap(const CallState& context) noexcept
{
    SetTextureWrap(ReadTexture(context.Args[0]), ArgInt(context.Args[1]));
    return nullptr;
}

// ----------------------------------------------------------------------------
// Image
// ----------------------------------------------------------------------------

static ObjectInstance* shard_image_LoadImage(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    Image img = LoadImage(narrow_str.c_str());
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_LoadImageFromTexture(const CallState& context) noexcept
{
    Image img = LoadImageFromTexture(ReadTexture(context.Args[0]));
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_UnloadImage(const CallState& context) noexcept
{
    UnloadImage(ReadImage(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_image_ExportImage(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[1]);
    return context.Collector.FromValue(ExportImage(ReadImage(context.Args[0]), narrow_str.c_str()));
}

static ObjectInstance* shard_image_GenImageColor(const CallState& context) noexcept
{
    Image img = GenImageColor(ArgInt(context.Args[0]), ArgInt(context.Args[1]), UnpackColor(context.Args[2]));
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_ImageCopy(const CallState& context) noexcept
{
    Image img = ImageCopy(ReadImage(context.Args[0]));
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_ImageResize(const CallState& context) noexcept
{
    Image img = ReadImage(context.Args[0]);
    ImageResize(&img, ArgInt(context.Args[1]), ArgInt(context.Args[2]));
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_ImageFlipVertical(const CallState& context) noexcept
{
    Image img = ReadImage(context.Args[0]);
    ImageFlipVertical(&img);
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_ImageFlipHorizontal(const CallState& context) noexcept
{
    Image img = ReadImage(context.Args[0]);
    ImageFlipHorizontal(&img);
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_ImageColorTint(const CallState& context) noexcept
{
    Image img = ReadImage(context.Args[0]);
    ImageColorTint(&img, UnpackColor(context.Args[1]));
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_ImageColorInvert(const CallState& context) noexcept
{
    Image img = ReadImage(context.Args[0]);
    ImageColorInvert(&img);
    return WriteImage(context.Collector, img);
}

static ObjectInstance* shard_image_ImageColorGrayscale(const CallState& context) noexcept
{
    Image img = ReadImage(context.Args[0]);
    ImageColorGrayscale(&img);
    return WriteImage(context.Collector, img);
}

// ----------------------------------------------------------------------------
// Audio device
// ----------------------------------------------------------------------------

static ObjectInstance* shard_audio_InitAudioDevice(const CallState& context) noexcept
{
    InitAudioDevice();
    return nullptr;
}

static ObjectInstance* shard_audio_CloseAudioDevice(const CallState& context) noexcept
{
    CloseAudioDevice();
    return nullptr;
}

static ObjectInstance* shard_audio_IsAudioDeviceReady(const CallState& context) noexcept
{
    return context.Collector.FromValue(IsAudioDeviceReady());
}

static ObjectInstance* shard_audio_SetMasterVolume(const CallState& context) noexcept
{
    SetMasterVolume(ArgFloat(context.Args[0]));
    return nullptr;
}

// ----------------------------------------------------------------------------
// Sound
// ----------------------------------------------------------------------------

static ObjectInstance* shard_sound_LoadSound(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    ::Sound sound = LoadSound(narrow_str.c_str());
    return WriteSound(context.Collector, sound);
}

static ObjectInstance* shard_sound_UnloadSound(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
    {
        UnloadSound(*ptr);
        delete ptr;

        context.Args[0]->SetField(sound_Handle->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)));
    }
    return nullptr;
}

static ObjectInstance* shard_sound_PlaySound(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
        PlaySound(*ptr);
    return nullptr;
}

static ObjectInstance* shard_sound_StopSound(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
        StopSound(*ptr);
    return nullptr;
}

static ObjectInstance* shard_sound_PauseSound(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
        PauseSound(*ptr);
    return nullptr;
}

static ObjectInstance* shard_sound_ResumeSound(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
        ResumeSound(*ptr);
    return nullptr;
}

static ObjectInstance* shard_sound_IsSoundPlaying(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr == nullptr)
        return context.Collector.FromValue(false);
    return context.Collector.FromValue(IsSoundPlaying(*ptr));
}

static ObjectInstance* shard_sound_SetSoundVolume(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
        SetSoundVolume(*ptr, ArgFloat(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_sound_SetSoundPitch(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
        SetSoundPitch(*ptr, ArgFloat(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_sound_SetSoundPan(const CallState& context) noexcept
{
    ::Sound* ptr = ReadSound(context.Args[0]);
    if (ptr != nullptr)
        SetSoundPan(*ptr, ArgFloat(context.Args[1]));
    return nullptr;
}

// ----------------------------------------------------------------------------
// Music
// ----------------------------------------------------------------------------

static ObjectInstance* shard_music_LoadMusicStream(const CallState& context) noexcept
{
    std::string narrow_str = ArgString(context.Args[0]);
    ::Music music = LoadMusicStream(narrow_str.c_str());
    return WriteMusic(context.Collector, music);
}

static ObjectInstance* shard_music_UnloadMusicStream(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
    {
        UnloadMusicStream(*ptr);
        delete ptr;

        context.Args[0]->SetField(music_Handle->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(0)));
    }
    return nullptr;
}

static ObjectInstance* shard_music_PlayMusicStream(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        PlayMusicStream(*ptr);
    return nullptr;
}

static ObjectInstance* shard_music_StopMusicStream(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        StopMusicStream(*ptr);
    return nullptr;
}

static ObjectInstance* shard_music_PauseMusicStream(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        PauseMusicStream(*ptr);
    return nullptr;
}

static ObjectInstance* shard_music_ResumeMusicStream(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        ResumeMusicStream(*ptr);
    return nullptr;
}

static ObjectInstance* shard_music_IsMusicStreamPlaying(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr == nullptr)
        return context.Collector.FromValue(false);
    return context.Collector.FromValue(IsMusicStreamPlaying(*ptr));
}

static ObjectInstance* shard_music_UpdateMusicStream(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        UpdateMusicStream(*ptr);
    return nullptr;
}

static ObjectInstance* shard_music_SetMusicVolume(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        SetMusicVolume(*ptr, ArgFloat(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_music_SetMusicPitch(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        SetMusicPitch(*ptr, ArgFloat(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_music_SetMusicPan(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr != nullptr)
        SetMusicPan(*ptr, ArgFloat(context.Args[1]));
    return nullptr;
}

static ObjectInstance* shard_music_GetMusicTimeLength(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr == nullptr)
        return context.Collector.FromValue(0.0);
    return context.Collector.FromValue(static_cast<double>(GetMusicTimeLength(*ptr)));
}

static ObjectInstance* shard_music_GetMusicTimePlayed(const CallState& context) noexcept
{
    ::Music* ptr = ReadMusic(context.Args[0]);
    if (ptr == nullptr)
        return context.Collector.FromValue(0.0);
    return context.Collector.FromValue(static_cast<double>(GetMusicTimePlayed(*ptr)));
}

// ----------------------------------------------------------------------------
// 3D
// ----------------------------------------------------------------------------

static ObjectInstance* shard_3d_BeginMode3D(const CallState& context) noexcept
{
    BeginMode3D(ReadCamera3D(context.Args[0]));
    return nullptr;
}

static ObjectInstance* shard_3d_EndMode3D(const CallState& context) noexcept
{
    EndMode3D();
    return nullptr;
}

static ObjectInstance* shard_3d_UpdateCamera(const CallState& context) noexcept
{
    Camera3D cam = ReadCamera3D(context.Args[0]);
    UpdateCamera(&cam, static_cast<int>(ArgInt(context.Args[1])));

    ObjectInstance* result = WriteCamera3D(context.Collector, cam);
    context.Args[0]->SetField(camera3D_Position->SlotIndex, result->GetField(camera3D_Position->SlotIndex));
    context.Args[0]->SetField(camera3D_Target->SlotIndex, result->GetField(camera3D_Target->SlotIndex));
    context.Args[0]->SetField(camera3D_Up->SlotIndex, result->GetField(camera3D_Up->SlotIndex));
    context.Args[0]->SetField(camera3D_Fovy->SlotIndex, result->GetField(camera3D_Fovy->SlotIndex));
    context.Args[0]->SetField(camera3D_Projection->SlotIndex, result->GetField(camera3D_Projection->SlotIndex));
    return nullptr;
}

static ObjectInstance* shard_3d_DrawCube(const CallState& context) noexcept
{
    DrawCube(
        ReadVector3(context.Args[0]),
        ArgFloat(context.Args[1]),
        ArgFloat(context.Args[2]),
        ArgFloat(context.Args[3]),
        UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_3d_DrawCubeRaw(const CallState& context) noexcept
{
    DrawCube(
        Vector3{ ArgFloat(context.Args[0]), ArgFloat(context.Args[1]), ArgFloat(context.Args[2]) },
        ArgFloat(context.Args[3]),
        ArgFloat(context.Args[4]),
        ArgFloat(context.Args[5]),
        UnpackColor(context.Args[6]));
    return nullptr;
}

static ObjectInstance* shard_3d_DrawCubeV(const CallState& context) noexcept
{
    DrawCubeV(ReadVector3(context.Args[0]), ReadVector3(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_3d_DrawCubeWires(const CallState& context) noexcept
{
    DrawCubeWires(
        ReadVector3(context.Args[0]),
        ArgFloat(context.Args[1]),
        ArgFloat(context.Args[2]),
        ArgFloat(context.Args[3]),
        UnpackColor(context.Args[4]));
    return nullptr;
}

static ObjectInstance* shard_3d_DrawCubeWiresV(const CallState& context) noexcept
{
    DrawCubeWiresV(ReadVector3(context.Args[0]), ReadVector3(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_3d_DrawWalls(const CallState& context) noexcept
{
    ObjectInstance* map = context.Args[0];
    std::int32_t width = ArgInt(context.Args[1]);
    std::int32_t height = ArgInt(context.Args[2]);
    Color color = UnpackColor(context.Args[3]);

    const ArrayTypeSymbol* arrayType = static_cast<const ArrayTypeSymbol*>(map->getInfo());
    TypeSymbol* elementType = arrayType->UnderlayingType;
    void* baseMemory = map->getMemory();
    std::size_t headerSize = SymbolTable::Primitives::Array->MemoryBytesSize;
    std::size_t elementSize = elementType->GetInlineSize();

    for (std::int32_t z = 0; z < height; ++z)
    {
        for (std::int32_t x = 0; x < width; ++x)
        {
            std::size_t offset = headerSize + elementSize * static_cast<std::size_t>(z * width + x);
            std::int64_t value = *reinterpret_cast<std::int64_t*>(static_cast<std::uint8_t*>(baseMemory) + offset);
            if (value == 1)
            {
                DrawCube(
                    Vector3{ static_cast<float>(x), 0.5f, static_cast<float>(z) },
                    1.0f, 1.0f, 1.0f,
                    color);
            }
        }
    }

    return nullptr;
}

static ObjectInstance* shard_3d_DrawLine3D(const CallState& context) noexcept
{
    DrawLine3D(ReadVector3(context.Args[0]), ReadVector3(context.Args[1]), UnpackColor(context.Args[2]));
    return nullptr;
}

static ObjectInstance* shard_3d_DrawGrid(const CallState& context) noexcept
{
    DrawGrid(ArgInt(context.Args[0]), ArgFloat(context.Args[1]));
    return nullptr;
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.raylib";
    lib.Description = L"raylib is a simple and easy-to-use library to enjoy videogames programming.";
    lib.Version = L"0.2.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> raylib(context, L"raylib");

    // ------------------------------------------------------------------------
    // Enums
    // ------------------------------------------------------------------------

    raylib_color = raylib.AddEnum(L"Colors")
        .AddValue(L"LightGray",  PACK_RGBA(200, 200, 200, 255))
        .AddValue(L"Gray",       PACK_RGBA(130, 130, 130, 255))
        .AddValue(L"DarkGray",   PACK_RGBA(80 , 80 , 80 , 255))
        .AddValue(L"Yellow",     PACK_RGBA(253, 249, 0  , 255))
        .AddValue(L"Gold",       PACK_RGBA(255, 203, 0  , 255))
        .AddValue(L"Orange",     PACK_RGBA(255, 161, 0  , 255))
        .AddValue(L"Pink",       PACK_RGBA(255, 109, 194, 255))
        .AddValue(L"Red",        PACK_RGBA(230, 41 , 55 , 255))
        .AddValue(L"Maroon",     PACK_RGBA(190, 33 , 55 , 255))
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
        .AddValue(L"VsyncHint",              static_cast<std::int64_t>(FLAG_VSYNC_HINT))
        .AddValue(L"FullscreenMode",         static_cast<std::int64_t>(FLAG_FULLSCREEN_MODE))
        .AddValue(L"WindowResizable",        static_cast<std::int64_t>(FLAG_WINDOW_RESIZABLE))
        .AddValue(L"WindowUndecorated",      static_cast<std::int64_t>(FLAG_WINDOW_UNDECORATED))
        .AddValue(L"WindowHidden",           static_cast<std::int64_t>(FLAG_WINDOW_HIDDEN))
        .AddValue(L"WindowMinimized",        static_cast<std::int64_t>(FLAG_WINDOW_MINIMIZED))
        .AddValue(L"WindowMaximized",        static_cast<std::int64_t>(FLAG_WINDOW_MAXIMIZED))
        .AddValue(L"WindowUnfocused",        static_cast<std::int64_t>(FLAG_WINDOW_UNFOCUSED))
        .AddValue(L"WindowTopmost",          static_cast<std::int64_t>(FLAG_WINDOW_TOPMOST))
        .AddValue(L"WindowAlwaysRun",        static_cast<std::int64_t>(FLAG_WINDOW_ALWAYS_RUN))
        .AddValue(L"WindowTransparent",      static_cast<std::int64_t>(FLAG_WINDOW_TRANSPARENT))
        .AddValue(L"WindowHighDPI",          static_cast<std::int64_t>(FLAG_WINDOW_HIGHDPI))
        .AddValue(L"Msaa4xHint",             static_cast<std::int64_t>(FLAG_MSAA_4X_HINT))
        .AddValue(L"WindowMousePassthrough", static_cast<std::int64_t>(FLAG_WINDOW_MOUSE_PASSTHROUGH))
        .AddValue(L"BorderlessWindowed",     static_cast<std::int64_t>(FLAG_BORDERLESS_WINDOWED_MODE))
        .AddValue(L"InterlacedHint",         static_cast<std::int64_t>(FLAG_INTERLACED_HINT));

    raylib_traceLogLevel = raylib.AddEnum(L"TraceLogLevel")
        .AddValue(L"All",     static_cast<std::int64_t>(LOG_ALL))
        .AddValue(L"Trace",   static_cast<std::int64_t>(LOG_TRACE))
        .AddValue(L"Debug",   static_cast<std::int64_t>(LOG_DEBUG))
        .AddValue(L"Info",    static_cast<std::int64_t>(LOG_INFO))
        .AddValue(L"Warning", static_cast<std::int64_t>(LOG_WARNING))
        .AddValue(L"Error",   static_cast<std::int64_t>(LOG_ERROR))
        .AddValue(L"Fatal",   static_cast<std::int64_t>(LOG_FATAL))
        .AddValue(L"None",    static_cast<std::int64_t>(LOG_NONE));

    raylib_blendMode = raylib.AddEnum(L"BlendMode")
        .AddValue(L"Alpha",             static_cast<std::int64_t>(BLEND_ALPHA))
        .AddValue(L"Additive",          static_cast<std::int64_t>(BLEND_ADDITIVE))
        .AddValue(L"Multiplied",        static_cast<std::int64_t>(BLEND_MULTIPLIED))
        .AddValue(L"AddColors",         static_cast<std::int64_t>(BLEND_ADD_COLORS))
        .AddValue(L"SubtractColors",    static_cast<std::int64_t>(BLEND_SUBTRACT_COLORS))
        .AddValue(L"AlphaPremultiply",  static_cast<std::int64_t>(BLEND_ALPHA_PREMULTIPLY))
        .AddValue(L"Custom",            static_cast<std::int64_t>(BLEND_CUSTOM))
        .AddValue(L"CustomSeparate",    static_cast<std::int64_t>(BLEND_CUSTOM_SEPARATE));

    raylib_keyboardKey = raylib.AddEnum(L"Keys")
        .AddValue(L"Null",          static_cast<std::int64_t>(KEY_NULL))
        .AddValue(L"Apostrophe",    static_cast<std::int64_t>(KEY_APOSTROPHE))
        .AddValue(L"Comma",         static_cast<std::int64_t>(KEY_COMMA))
        .AddValue(L"Minus",         static_cast<std::int64_t>(KEY_MINUS))
        .AddValue(L"Period",        static_cast<std::int64_t>(KEY_PERIOD))
        .AddValue(L"Slash",         static_cast<std::int64_t>(KEY_SLASH))
        .AddValue(L"Zero",          static_cast<std::int64_t>(KEY_ZERO))
        .AddValue(L"One",           static_cast<std::int64_t>(KEY_ONE))
        .AddValue(L"Two",           static_cast<std::int64_t>(KEY_TWO))
        .AddValue(L"Three",         static_cast<std::int64_t>(KEY_THREE))
        .AddValue(L"Four",          static_cast<std::int64_t>(KEY_FOUR))
        .AddValue(L"Five",          static_cast<std::int64_t>(KEY_FIVE))
        .AddValue(L"Six",           static_cast<std::int64_t>(KEY_SIX))
        .AddValue(L"Seven",         static_cast<std::int64_t>(KEY_SEVEN))
        .AddValue(L"Eight",         static_cast<std::int64_t>(KEY_EIGHT))
        .AddValue(L"Nine",          static_cast<std::int64_t>(KEY_NINE))
        .AddValue(L"Semicolon",     static_cast<std::int64_t>(KEY_SEMICOLON))
        .AddValue(L"Equal",         static_cast<std::int64_t>(KEY_EQUAL))
        .AddValue(L"A",             static_cast<std::int64_t>(KEY_A))
        .AddValue(L"B",             static_cast<std::int64_t>(KEY_B))
        .AddValue(L"C",             static_cast<std::int64_t>(KEY_C))
        .AddValue(L"D",             static_cast<std::int64_t>(KEY_D))
        .AddValue(L"E",             static_cast<std::int64_t>(KEY_E))
        .AddValue(L"F",             static_cast<std::int64_t>(KEY_F))
        .AddValue(L"G",             static_cast<std::int64_t>(KEY_G))
        .AddValue(L"H",             static_cast<std::int64_t>(KEY_H))
        .AddValue(L"I",             static_cast<std::int64_t>(KEY_I))
        .AddValue(L"J",             static_cast<std::int64_t>(KEY_J))
        .AddValue(L"K",             static_cast<std::int64_t>(KEY_K))
        .AddValue(L"L",             static_cast<std::int64_t>(KEY_L))
        .AddValue(L"M",             static_cast<std::int64_t>(KEY_M))
        .AddValue(L"N",             static_cast<std::int64_t>(KEY_N))
        .AddValue(L"O",             static_cast<std::int64_t>(KEY_O))
        .AddValue(L"P",             static_cast<std::int64_t>(KEY_P))
        .AddValue(L"Q",             static_cast<std::int64_t>(KEY_Q))
        .AddValue(L"R",             static_cast<std::int64_t>(KEY_R))
        .AddValue(L"S",             static_cast<std::int64_t>(KEY_S))
        .AddValue(L"T",             static_cast<std::int64_t>(KEY_T))
        .AddValue(L"U",             static_cast<std::int64_t>(KEY_U))
        .AddValue(L"V",             static_cast<std::int64_t>(KEY_V))
        .AddValue(L"W",             static_cast<std::int64_t>(KEY_W))
        .AddValue(L"X",             static_cast<std::int64_t>(KEY_X))
        .AddValue(L"Y",             static_cast<std::int64_t>(KEY_Y))
        .AddValue(L"Z",             static_cast<std::int64_t>(KEY_Z))
        .AddValue(L"LeftBracket",   static_cast<std::int64_t>(KEY_LEFT_BRACKET))
        .AddValue(L"RightBracket",  static_cast<std::int64_t>(KEY_RIGHT_BRACKET))
        .AddValue(L"Backslash",     static_cast<std::int64_t>(KEY_BACKSLASH))
        .AddValue(L"Grave",         static_cast<std::int64_t>(KEY_GRAVE))
        .AddValue(L"Space",         static_cast<std::int64_t>(KEY_SPACE))
        .AddValue(L"Escape",        static_cast<std::int64_t>(KEY_ESCAPE))
        .AddValue(L"Enter",         static_cast<std::int64_t>(KEY_ENTER))
        .AddValue(L"Tab",           static_cast<std::int64_t>(KEY_TAB))
        .AddValue(L"Backspace",     static_cast<std::int64_t>(KEY_BACKSPACE))
        .AddValue(L"Insert",        static_cast<std::int64_t>(KEY_INSERT))
        .AddValue(L"Delete",        static_cast<std::int64_t>(KEY_DELETE))
        .AddValue(L"Right",         static_cast<std::int64_t>(KEY_RIGHT))
        .AddValue(L"Left",          static_cast<std::int64_t>(KEY_LEFT))
        .AddValue(L"Down",          static_cast<std::int64_t>(KEY_DOWN))
        .AddValue(L"Up",            static_cast<std::int64_t>(KEY_UP))
        .AddValue(L"PageUp",        static_cast<std::int64_t>(KEY_PAGE_UP))
        .AddValue(L"PageDown",      static_cast<std::int64_t>(KEY_PAGE_DOWN))
        .AddValue(L"Home",          static_cast<std::int64_t>(KEY_HOME))
        .AddValue(L"End",           static_cast<std::int64_t>(KEY_END))
        .AddValue(L"CapsLock",      static_cast<std::int64_t>(KEY_CAPS_LOCK))
        .AddValue(L"ScrollLock",    static_cast<std::int64_t>(KEY_SCROLL_LOCK))
        .AddValue(L"NumLock",       static_cast<std::int64_t>(KEY_NUM_LOCK))
        .AddValue(L"PrintScreen",   static_cast<std::int64_t>(KEY_PRINT_SCREEN))
        .AddValue(L"Pause",         static_cast<std::int64_t>(KEY_PAUSE))
        .AddValue(L"F1",            static_cast<std::int64_t>(KEY_F1))
        .AddValue(L"F2",            static_cast<std::int64_t>(KEY_F2))
        .AddValue(L"F3",            static_cast<std::int64_t>(KEY_F3))
        .AddValue(L"F4",            static_cast<std::int64_t>(KEY_F4))
        .AddValue(L"F5",            static_cast<std::int64_t>(KEY_F5))
        .AddValue(L"F6",            static_cast<std::int64_t>(KEY_F6))
        .AddValue(L"F7",            static_cast<std::int64_t>(KEY_F7))
        .AddValue(L"F8",            static_cast<std::int64_t>(KEY_F8))
        .AddValue(L"F9",            static_cast<std::int64_t>(KEY_F9))
        .AddValue(L"F10",           static_cast<std::int64_t>(KEY_F10))
        .AddValue(L"F11",           static_cast<std::int64_t>(KEY_F11))
        .AddValue(L"F12",           static_cast<std::int64_t>(KEY_F12))
        .AddValue(L"LeftShift",     static_cast<std::int64_t>(KEY_LEFT_SHIFT))
        .AddValue(L"LeftControl",   static_cast<std::int64_t>(KEY_LEFT_CONTROL))
        .AddValue(L"LeftAlt",       static_cast<std::int64_t>(KEY_LEFT_ALT))
        .AddValue(L"LeftSuper",     static_cast<std::int64_t>(KEY_LEFT_SUPER))
        .AddValue(L"RightShift",    static_cast<std::int64_t>(KEY_RIGHT_SHIFT))
        .AddValue(L"RightControl",  static_cast<std::int64_t>(KEY_RIGHT_CONTROL))
        .AddValue(L"RightAlt",      static_cast<std::int64_t>(KEY_RIGHT_ALT))
        .AddValue(L"RightSuper",    static_cast<std::int64_t>(KEY_RIGHT_SUPER))
        .AddValue(L"KbMenu",        static_cast<std::int64_t>(KEY_KB_MENU))
        .AddValue(L"KeyPad0",       static_cast<std::int64_t>(KEY_KP_0))
        .AddValue(L"KeyPad1",       static_cast<std::int64_t>(KEY_KP_1))
        .AddValue(L"KeyPad2",       static_cast<std::int64_t>(KEY_KP_2))
        .AddValue(L"KeyPad3",       static_cast<std::int64_t>(KEY_KP_3))
        .AddValue(L"KeyPad4",       static_cast<std::int64_t>(KEY_KP_4))
        .AddValue(L"KeyPad5",       static_cast<std::int64_t>(KEY_KP_5))
        .AddValue(L"KeyPad6",       static_cast<std::int64_t>(KEY_KP_6))
        .AddValue(L"KeyPad7",       static_cast<std::int64_t>(KEY_KP_7))
        .AddValue(L"KeyPad8",       static_cast<std::int64_t>(KEY_KP_8))
        .AddValue(L"KeyPad9",       static_cast<std::int64_t>(KEY_KP_9))
        .AddValue(L"KeyPadDecimal", static_cast<std::int64_t>(KEY_KP_DECIMAL))
        .AddValue(L"KeyPadDivide",  static_cast<std::int64_t>(KEY_KP_DIVIDE))
        .AddValue(L"KeyPadMultiply",static_cast<std::int64_t>(KEY_KP_MULTIPLY))
        .AddValue(L"KeyPadSubtract",static_cast<std::int64_t>(KEY_KP_SUBTRACT))
        .AddValue(L"KeyPadAdd",     static_cast<std::int64_t>(KEY_KP_ADD))
        .AddValue(L"KeyPadEnter",   static_cast<std::int64_t>(KEY_KP_ENTER))
        .AddValue(L"KeyPadEqual",   static_cast<std::int64_t>(KEY_KP_EQUAL))
        .AddValue(L"Back",          static_cast<std::int64_t>(KEY_BACK))
        .AddValue(L"Menu",          static_cast<std::int64_t>(KEY_MENU))
        .AddValue(L"VolumeUp",      static_cast<std::int64_t>(KEY_VOLUME_UP))
        .AddValue(L"VolumeDown",    static_cast<std::int64_t>(KEY_VOLUME_DOWN));

    raylib_mouseButton = raylib.AddEnum(L"MouseButton")
        .AddValue(L"Left",    static_cast<std::int64_t>(MOUSE_BUTTON_LEFT))
        .AddValue(L"Right",   static_cast<std::int64_t>(MOUSE_BUTTON_RIGHT))
        .AddValue(L"Middle",  static_cast<std::int64_t>(MOUSE_BUTTON_MIDDLE))
        .AddValue(L"Side",    static_cast<std::int64_t>(MOUSE_BUTTON_SIDE))
        .AddValue(L"Extra",   static_cast<std::int64_t>(MOUSE_BUTTON_EXTRA))
        .AddValue(L"Forward", static_cast<std::int64_t>(MOUSE_BUTTON_FORWARD))
        .AddValue(L"Back",    static_cast<std::int64_t>(MOUSE_BUTTON_BACK));

    raylib_mouseCursor = raylib.AddEnum(L"MouseCursor")
        .AddValue(L"Default",      static_cast<std::int64_t>(MOUSE_CURSOR_DEFAULT))
        .AddValue(L"Arrow",        static_cast<std::int64_t>(MOUSE_CURSOR_ARROW))
        .AddValue(L"IBeam",        static_cast<std::int64_t>(MOUSE_CURSOR_IBEAM))
        .AddValue(L"Crosshair",    static_cast<std::int64_t>(MOUSE_CURSOR_CROSSHAIR))
        .AddValue(L"PointingHand", static_cast<std::int64_t>(MOUSE_CURSOR_POINTING_HAND))
        .AddValue(L"ResizeEW",     static_cast<std::int64_t>(MOUSE_CURSOR_RESIZE_EW))
        .AddValue(L"ResizeNS",     static_cast<std::int64_t>(MOUSE_CURSOR_RESIZE_NS))
        .AddValue(L"ResizeNWSE",   static_cast<std::int64_t>(MOUSE_CURSOR_RESIZE_NWSE))
        .AddValue(L"ResizeNESW",   static_cast<std::int64_t>(MOUSE_CURSOR_RESIZE_NESW))
        .AddValue(L"ResizeAll",    static_cast<std::int64_t>(MOUSE_CURSOR_RESIZE_ALL))
        .AddValue(L"NotAllowed",   static_cast<std::int64_t>(MOUSE_CURSOR_NOT_ALLOWED));

    raylib_gamepadButton = raylib.AddEnum(L"GamepadButton")
        .AddValue(L"Unknown",            static_cast<std::int64_t>(GAMEPAD_BUTTON_UNKNOWN))
        .AddValue(L"LeftFaceUp",         static_cast<std::int64_t>(GAMEPAD_BUTTON_LEFT_FACE_UP))
        .AddValue(L"LeftFaceRight",      static_cast<std::int64_t>(GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
        .AddValue(L"LeftFaceDown",       static_cast<std::int64_t>(GAMEPAD_BUTTON_LEFT_FACE_DOWN))
        .AddValue(L"LeftFaceLeft",       static_cast<std::int64_t>(GAMEPAD_BUTTON_LEFT_FACE_LEFT))
        .AddValue(L"RightFaceUp",        static_cast<std::int64_t>(GAMEPAD_BUTTON_RIGHT_FACE_UP))
        .AddValue(L"RightFaceRight",     static_cast<std::int64_t>(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))
        .AddValue(L"RightFaceDown",      static_cast<std::int64_t>(GAMEPAD_BUTTON_RIGHT_FACE_DOWN))
        .AddValue(L"RightFaceLeft",      static_cast<std::int64_t>(GAMEPAD_BUTTON_RIGHT_FACE_LEFT))
        .AddValue(L"LeftTrigger1",       static_cast<std::int64_t>(GAMEPAD_BUTTON_LEFT_TRIGGER_1))
        .AddValue(L"LeftTrigger2",       static_cast<std::int64_t>(GAMEPAD_BUTTON_LEFT_TRIGGER_2))
        .AddValue(L"RightTrigger1",      static_cast<std::int64_t>(GAMEPAD_BUTTON_RIGHT_TRIGGER_1))
        .AddValue(L"RightTrigger2",      static_cast<std::int64_t>(GAMEPAD_BUTTON_RIGHT_TRIGGER_2))
        .AddValue(L"MiddleLeft",         static_cast<std::int64_t>(GAMEPAD_BUTTON_MIDDLE_LEFT))
        .AddValue(L"Middle",             static_cast<std::int64_t>(GAMEPAD_BUTTON_MIDDLE))
        .AddValue(L"MiddleRight",        static_cast<std::int64_t>(GAMEPAD_BUTTON_MIDDLE_RIGHT))
        .AddValue(L"LeftThumb",          static_cast<std::int64_t>(GAMEPAD_BUTTON_LEFT_THUMB))
        .AddValue(L"RightThumb",         static_cast<std::int64_t>(GAMEPAD_BUTTON_RIGHT_THUMB));

    raylib_gamepadAxis = raylib.AddEnum(L"GamepadAxis")
        .AddValue(L"LeftX",         static_cast<std::int64_t>(GAMEPAD_AXIS_LEFT_X))
        .AddValue(L"LeftY",         static_cast<std::int64_t>(GAMEPAD_AXIS_LEFT_Y))
        .AddValue(L"RightX",        static_cast<std::int64_t>(GAMEPAD_AXIS_RIGHT_X))
        .AddValue(L"RightY",        static_cast<std::int64_t>(GAMEPAD_AXIS_RIGHT_Y))
        .AddValue(L"LeftTrigger",   static_cast<std::int64_t>(GAMEPAD_AXIS_LEFT_TRIGGER))
        .AddValue(L"RightTrigger",  static_cast<std::int64_t>(GAMEPAD_AXIS_RIGHT_TRIGGER));

    raylib_gesture = raylib.AddEnum(L"Gesture", true)
        .AddValue(L"None",        static_cast<std::int64_t>(GESTURE_NONE))
        .AddValue(L"Tap",         static_cast<std::int64_t>(GESTURE_TAP))
        .AddValue(L"DoubleTap",   static_cast<std::int64_t>(GESTURE_DOUBLETAP))
        .AddValue(L"Hold",        static_cast<std::int64_t>(GESTURE_HOLD))
        .AddValue(L"Drag",        static_cast<std::int64_t>(GESTURE_DRAG))
        .AddValue(L"SwipeRight",  static_cast<std::int64_t>(GESTURE_SWIPE_RIGHT))
        .AddValue(L"SwipeLeft",   static_cast<std::int64_t>(GESTURE_SWIPE_LEFT))
        .AddValue(L"SwipeUp",     static_cast<std::int64_t>(GESTURE_SWIPE_UP))
        .AddValue(L"SwipeDown",   static_cast<std::int64_t>(GESTURE_SWIPE_DOWN))
        .AddValue(L"PinchIn",     static_cast<std::int64_t>(GESTURE_PINCH_IN))
        .AddValue(L"PinchOut",    static_cast<std::int64_t>(GESTURE_PINCH_OUT));

    raylib_cameraMode = raylib.AddEnum(L"CameraMode")
        .AddValue(L"Custom",       static_cast<std::int64_t>(CAMERA_CUSTOM))
        .AddValue(L"Free",         static_cast<std::int64_t>(CAMERA_FREE))
        .AddValue(L"Orbital",      static_cast<std::int64_t>(CAMERA_ORBITAL))
        .AddValue(L"FirstPerson",  static_cast<std::int64_t>(CAMERA_FIRST_PERSON))
        .AddValue(L"ThirdPerson",  static_cast<std::int64_t>(CAMERA_THIRD_PERSON));

    raylib_cameraProjection = raylib.AddEnum(L"CameraProjection")
        .AddValue(L"Perspective",  static_cast<std::int64_t>(CAMERA_PERSPECTIVE))
        .AddValue(L"Orthographic", static_cast<std::int64_t>(CAMERA_ORTHOGRAPHIC));

    // ------------------------------------------------------------------------
    // Structs
    // ------------------------------------------------------------------------

    SymbolBuilder<StructSymbol> vector2Builder = raylib.AddStruct(L"Vector2");
    vector2_X = vector2Builder.AddField(L"X", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    vector2_Y = vector2Builder.AddField(L"Y", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    raylib_Vector2 = vector2Builder;

    SymbolBuilder<StructSymbol> vector3Builder = raylib.AddStruct(L"Vector3");
    vector3_X = vector3Builder.AddField(L"X", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    vector3_Y = vector3Builder.AddField(L"Y", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    vector3_Z = vector3Builder.AddField(L"Z", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    raylib_Vector3 = vector3Builder;

    SymbolBuilder<StructSymbol> rectangleBuilder = raylib.AddStruct(L"Rectangle");
    rectangle_X = rectangleBuilder.AddField(L"X", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    rectangle_Y = rectangleBuilder.AddField(L"Y", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    rectangle_Width = rectangleBuilder.AddField(L"Width", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    rectangle_Height = rectangleBuilder.AddField(L"Height", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    raylib_Rectangle = rectangleBuilder;

    SymbolBuilder<ClassSymbol> textureBuilder = raylib.AddClass(L"Texture");
    texture_Id = textureBuilder.AddField(L"Id", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    texture_Width = textureBuilder.AddField(L"Width", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    texture_Height = textureBuilder.AddField(L"Height", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    texture_Mipmaps = textureBuilder.AddField(L"Mipmaps", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    texture_Format = textureBuilder.AddField(L"Format", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    raylib_Texture = textureBuilder;

    SymbolBuilder<ClassSymbol> imageBuilder = raylib.AddClass(L"Image");
    image_Data = imageBuilder.AddField(L"Data", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    image_Width = imageBuilder.AddField(L"Width", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    image_Height = imageBuilder.AddField(L"Height", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    image_Mipmaps = imageBuilder.AddField(L"Mipmaps", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    image_Format = imageBuilder.AddField(L"Format", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    raylib_Image = imageBuilder;

    SymbolBuilder<ClassSymbol> soundBuilder = raylib.AddClass(L"Sound");
    sound_Handle = soundBuilder.AddField(L"Handle", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    raylib_Sound = soundBuilder;

    SymbolBuilder<ClassSymbol> musicBuilder = raylib.AddClass(L"Music");
    music_Handle = musicBuilder.AddField(L"Handle", TYPE_INT, LINK_INSTANCE, SymbolAccesibility::Private).Get();
    raylib_Music = musicBuilder;

    SymbolBuilder<StructSymbol> camera3DBuilder = raylib.AddStruct(L"Camera3D");
    camera3D_Position = camera3DBuilder.AddField(L"Position", raylib_Vector3, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    camera3D_Target = camera3DBuilder.AddField(L"Target", raylib_Vector3, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    camera3D_Up = camera3DBuilder.AddField(L"Up", raylib_Vector3, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    camera3D_Fovy = camera3DBuilder.AddField(L"Fovy", TYPE_DOUBLE, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    camera3D_Projection = camera3DBuilder.AddField(L"Projection", raylib_cameraProjection, LINK_INSTANCE, SymbolAccesibility::Public).Get();
    raylib_Camera3D = camera3DBuilder;

    // ------------------------------------------------------------------------
    // Struct / handle factories
    // ------------------------------------------------------------------------

    vector2Builder
        .AddMethod(L"Create", raylib_Vector2, LINK_STATIC)
        .AddParameter(L"x", TYPE_DOUBLE)
        .AddParameter(L"y", TYPE_DOUBLE)
        .SetCallback(&shard_vector2_Create);

    vector3Builder
        .AddMethod(L"Create", raylib_Vector3, LINK_STATIC)
        .AddParameter(L"x", TYPE_DOUBLE)
        .AddParameter(L"y", TYPE_DOUBLE)
        .AddParameter(L"z", TYPE_DOUBLE)
        .SetCallback(&shard_vector3_Create);

    rectangleBuilder
        .AddMethod(L"Create", raylib_Rectangle, LINK_STATIC)
        .AddParameter(L"x", TYPE_DOUBLE)
        .AddParameter(L"y", TYPE_DOUBLE)
        .AddParameter(L"width", TYPE_DOUBLE)
        .AddParameter(L"height", TYPE_DOUBLE)
        .SetCallback(&shard_rectangle_Create);

    camera3DBuilder
        .AddInit()
        .AddParameter(L"position", raylib_Vector3)
        .AddParameter(L"target", raylib_Vector3)
        .AddParameter(L"up", raylib_Vector3)
        .AddParameter(L"fovy", TYPE_DOUBLE)
        .AddParameter(L"projection", raylib_cameraProjection)
        .SetCallback(&shard_camera3D_Init);

    textureBuilder
        .AddMethod(L"Load", raylib_Texture, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_texture_LoadTexture);

    textureBuilder
        .AddMethod(L"LoadFromImage", raylib_Texture, LINK_STATIC)
        .AddParameter(L"image", raylib_Image)
        .SetCallback(&shard_texture_LoadTextureFromImage);

    imageBuilder
        .AddMethod(L"Load", raylib_Image, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_image_LoadImage);

    imageBuilder
        .AddMethod(L"LoadFromTexture", raylib_Image, LINK_STATIC)
        .AddParameter(L"texture", raylib_Texture)
        .SetCallback(&shard_image_LoadImageFromTexture);

    imageBuilder
        .AddMethod(L"GenColor", raylib_Image, LINK_STATIC)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_image_GenImageColor);

    soundBuilder
        .AddMethod(L"Load", raylib_Sound, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_sound_LoadSound);

    musicBuilder
        .AddMethod(L"Load", raylib_Music, LINK_STATIC)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_music_LoadMusicStream);

    // ------------------------------------------------------------------------
    // Window
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibWindowBuilder = raylib.AddClass(L"Window");

    raylibWindowBuilder
        .AddMethod(L"Init", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"title", TYPE_STRING)
        .SetCallback(&shard_window_InitWindow);

    raylibWindowBuilder
        .AddMethod(L"ShouldClose", TYPE_BOOL, LINK_STATIC)
        .SetCallback(&shard_window_WindowShouldClose);

    raylibWindowBuilder
        .AddMethod(L"BeginDrawing", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_BeginDrawing);

    raylibWindowBuilder
        .AddMethod(L"EndDrawing", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_EndDrawing);

    raylibWindowBuilder
        .AddMethod(L"Close", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_CloseWindow);

    raylibWindowBuilder
        .AddMethod(L"ClearBackground", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_window_ClearBackground);

    raylibWindowBuilder
        .AddMethod(L"DrawFPS", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .SetCallback(&shard_window_DrawFPS);

    raylibWindowBuilder
        .AddMethod(L"IsReady", TYPE_BOOL, LINK_STATIC)
        .SetCallback(&shard_window_IsWindowReady);

    raylibWindowBuilder
        .AddMethod(L"IsFullscreen", TYPE_BOOL, LINK_STATIC)
        .SetCallback(&shard_window_IsWindowFullscreen);

    raylibWindowBuilder
        .AddMethod(L"SetState", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"flags", raylib_configFlags)
        .SetCallback(&shard_window_SetWindowState);

    raylibWindowBuilder
        .AddMethod(L"ClearState", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"flags", raylib_configFlags)
        .SetCallback(&shard_window_ClearWindowState);

    raylibWindowBuilder
        .AddMethod(L"ToggleFullscreen", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_ToggleFullscreen);

    raylibWindowBuilder
        .AddMethod(L"Maximize", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_MaximizeWindow);

    raylibWindowBuilder
        .AddMethod(L"Minimize", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_MinimizeWindow);

    raylibWindowBuilder
        .AddMethod(L"Restore", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_RestoreWindow);

    raylibWindowBuilder
        .AddMethod(L"SetTitle", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"title", TYPE_STRING)
        .SetCallback(&shard_window_SetWindowTitle);

    raylibWindowBuilder
        .AddMethod(L"SetPosition", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .SetCallback(&shard_window_SetWindowPosition);

    raylibWindowBuilder
        .AddMethod(L"SetMonitor", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"monitor", TYPE_INT)
        .SetCallback(&shard_window_SetWindowMonitor);

    raylibWindowBuilder
        .AddMethod(L"SetMinSize", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .SetCallback(&shard_window_SetWindowMinSize);

    raylibWindowBuilder
        .AddMethod(L"SetMaxSize", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .SetCallback(&shard_window_SetWindowMaxSize);

    raylibWindowBuilder
        .AddMethod(L"SetSize", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .SetCallback(&shard_window_SetWindowSize);

    raylibWindowBuilder
        .AddMethod(L"SetOpacity", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"opacity", TYPE_DOUBLE)
        .SetCallback(&shard_window_SetWindowOpacity);

    raylibWindowBuilder
        .AddMethod(L"GetScreenWidth", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_window_GetScreenWidth);

    raylibWindowBuilder
        .AddMethod(L"GetScreenHeight", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_window_GetScreenHeight);

    raylibWindowBuilder
        .AddMethod(L"GetRenderWidth", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_window_GetRenderWidth);

    raylibWindowBuilder
        .AddMethod(L"GetRenderHeight", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_window_GetRenderHeight);

    raylibWindowBuilder
        .AddMethod(L"GetMonitorCount", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_window_GetMonitorCount);

    raylibWindowBuilder
        .AddMethod(L"GetCurrentMonitor", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_window_GetCurrentMonitor);

    raylibWindowBuilder
        .AddMethod(L"GetMonitorWidth", TYPE_INT, LINK_STATIC)
        .AddParameter(L"monitor", TYPE_INT)
        .SetCallback(&shard_window_GetMonitorWidth);

    raylibWindowBuilder
        .AddMethod(L"GetMonitorHeight", TYPE_INT, LINK_STATIC)
        .AddParameter(L"monitor", TYPE_INT)
        .SetCallback(&shard_window_GetMonitorHeight);

    raylibWindowBuilder
        .AddMethod(L"GetMonitorRefreshRate", TYPE_INT, LINK_STATIC)
        .AddParameter(L"monitor", TYPE_INT)
        .SetCallback(&shard_window_GetMonitorRefreshRate);

    raylibWindowBuilder
        .AddMethod(L"GetPosition", raylib_Vector2, LINK_STATIC)
        .SetCallback(&shard_window_GetWindowPosition);

    raylibWindowBuilder
        .AddMethod(L"GetScaleDPI", raylib_Vector2, LINK_STATIC)
        .SetCallback(&shard_window_GetWindowScaleDPI);

    raylibWindowBuilder
        .AddMethod(L"GetMonitorName", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"monitor", TYPE_INT)
        .SetCallback(&shard_window_GetMonitorName);

    raylibWindowBuilder
        .AddMethod(L"SetClipboardText", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"text", TYPE_STRING)
        .SetCallback(&shard_window_SetClipboardText);

    raylibWindowBuilder
        .AddMethod(L"GetClipboardText", TYPE_STRING, LINK_STATIC)
        .SetCallback(&shard_window_GetClipboardText);

    raylibWindowBuilder
        .AddMethod(L"EnableEventWaiting", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_EnableEventWaiting);

    raylibWindowBuilder
        .AddMethod(L"DisableEventWaiting", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_window_DisableEventWaiting);

    // ------------------------------------------------------------------------
    // Cursor
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibCursorBuilder = raylib.AddClass(L"Cursor");

    raylibCursorBuilder
        .AddMethod(L"Show", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_cursor_ShowCursor);

    raylibCursorBuilder
        .AddMethod(L"Hide", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_cursor_HideCursor);

    raylibCursorBuilder
        .AddMethod(L"IsHidden", TYPE_BOOL, LINK_STATIC)
        .SetCallback(&shard_cursor_IsCursorHidden);

    raylibCursorBuilder
        .AddMethod(L"Enable", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_cursor_EnableCursor);

    raylibCursorBuilder
        .AddMethod(L"Disable", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_cursor_DisableCursor);

    raylibCursorBuilder
        .AddMethod(L"IsOnScreen", TYPE_BOOL, LINK_STATIC)
        .SetCallback(&shard_cursor_IsCursorOnScreen);

    // ------------------------------------------------------------------------
    // Time
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibTimeBuilder = raylib.AddClass(L"Time");

    raylibTimeBuilder
        .AddMethod(L"SetTargetFPS", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"fps", TYPE_INT)
        .SetCallback(&shard_time_SetTargetFPS);

    raylibTimeBuilder
        .AddMethod(L"GetFPS", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_time_GetFPS);

    raylibTimeBuilder
        .AddMethod(L"GetFrameTime", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback(&shard_time_GetFrameTime);

    raylibTimeBuilder
        .AddMethod(L"GetTime", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback(&shard_time_GetTime);

    // ------------------------------------------------------------------------
    // Random
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibRandomBuilder = raylib.AddClass(L"Random");

    raylibRandomBuilder
        .AddMethod(L"SetSeed", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"seed", TYPE_INT)
        .SetCallback(&shard_random_SetRandomSeed);

    raylibRandomBuilder
        .AddMethod(L"GetValue", TYPE_INT, LINK_STATIC)
        .AddParameter(L"min", TYPE_INT)
        .AddParameter(L"max", TYPE_INT)
        .SetCallback(&shard_random_GetRandomValue);

    // ------------------------------------------------------------------------
    // Keyboard
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibKeyboardBuilder = raylib.AddClass(L"Keyboard");

    raylibKeyboardBuilder
        .AddMethod(L"IsKeyDown", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_keyboard_IsKeyDown);

    raylibKeyboardBuilder
        .AddMethod(L"IsKeyPressed", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_keyboard_IsKeyPressed);

    raylibKeyboardBuilder
        .AddMethod(L"IsKeyPressedRepeat", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_keyboard_IsKeyPressedRepeat);

    raylibKeyboardBuilder
        .AddMethod(L"IsKeyReleased", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_keyboard_IsKeyReleased);

    raylibKeyboardBuilder
        .AddMethod(L"IsKeyUp", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_keyboard_IsKeyUp);

    raylibKeyboardBuilder
        .AddMethod(L"GetKeyPressed", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_keyboard_GetKeyPressed);

    raylibKeyboardBuilder
        .AddMethod(L"GetCharPressed", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_keyboard_GetCharPressed);

    raylibKeyboardBuilder
        .AddMethod(L"SetExitKey", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"key", raylib_keyboardKey)
        .SetCallback(&shard_keyboard_SetExitKey);

    // ------------------------------------------------------------------------
    // Mouse
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibMouseBuilder = raylib.AddClass(L"Mouse");

    raylibMouseBuilder
        .AddMethod(L"IsButtonPressed", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"button", raylib_mouseButton)
        .SetCallback(&shard_mouse_IsMouseButtonPressed);

    raylibMouseBuilder
        .AddMethod(L"IsButtonDown", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"button", raylib_mouseButton)
        .SetCallback(&shard_mouse_IsMouseButtonDown);

    raylibMouseBuilder
        .AddMethod(L"IsButtonReleased", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"button", raylib_mouseButton)
        .SetCallback(&shard_mouse_IsMouseButtonReleased);

    raylibMouseBuilder
        .AddMethod(L"IsButtonUp", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"button", raylib_mouseButton)
        .SetCallback(&shard_mouse_IsMouseButtonUp);

    raylibMouseBuilder
        .AddMethod(L"GetX", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_mouse_GetMouseX);

    raylibMouseBuilder
        .AddMethod(L"GetY", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_mouse_GetMouseY);

    raylibMouseBuilder
        .AddMethod(L"GetPosition", raylib_Vector2, LINK_STATIC)
        .SetCallback(&shard_mouse_GetMousePosition);

    raylibMouseBuilder
        .AddMethod(L"GetDelta", raylib_Vector2, LINK_STATIC)
        .SetCallback(&shard_mouse_GetMouseDelta);

    raylibMouseBuilder
        .AddMethod(L"SetPosition", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .SetCallback(&shard_mouse_SetMousePosition);

    raylibMouseBuilder
        .AddMethod(L"SetOffset", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .SetCallback(&shard_mouse_SetMouseOffset);

    raylibMouseBuilder
        .AddMethod(L"SetScale", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_DOUBLE)
        .AddParameter(L"y", TYPE_DOUBLE)
        .SetCallback(&shard_mouse_SetMouseScale);

    raylibMouseBuilder
        .AddMethod(L"GetWheelMove", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback(&shard_mouse_GetMouseWheelMove);

    raylibMouseBuilder
        .AddMethod(L"GetWheelMoveV", raylib_Vector2, LINK_STATIC)
        .SetCallback(&shard_mouse_GetMouseWheelMoveV);

    raylibMouseBuilder
        .AddMethod(L"SetCursor", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"cursor", raylib_mouseCursor)
        .SetCallback(&shard_mouse_SetMouseCursor);

    // ------------------------------------------------------------------------
    // Gamepad
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibGamepadBuilder = raylib.AddClass(L"Gamepad");

    raylibGamepadBuilder
        .AddMethod(L"IsAvailable", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .SetCallback(&shard_gamepad_IsGamepadAvailable);

    raylibGamepadBuilder
        .AddMethod(L"GetName", TYPE_STRING, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .SetCallback(&shard_gamepad_GetGamepadName);

    raylibGamepadBuilder
        .AddMethod(L"IsButtonPressed", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .AddParameter(L"button", raylib_gamepadButton)
        .SetCallback(&shard_gamepad_IsGamepadButtonPressed);

    raylibGamepadBuilder
        .AddMethod(L"IsButtonDown", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .AddParameter(L"button", raylib_gamepadButton)
        .SetCallback(&shard_gamepad_IsGamepadButtonDown);

    raylibGamepadBuilder
        .AddMethod(L"IsButtonReleased", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .AddParameter(L"button", raylib_gamepadButton)
        .SetCallback(&shard_gamepad_IsGamepadButtonReleased);

    raylibGamepadBuilder
        .AddMethod(L"IsButtonUp", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .AddParameter(L"button", raylib_gamepadButton)
        .SetCallback(&shard_gamepad_IsGamepadButtonUp);

    raylibGamepadBuilder
        .AddMethod(L"GetButtonPressed", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_gamepad_GetGamepadButtonPressed);

    raylibGamepadBuilder
        .AddMethod(L"GetAxisCount", TYPE_INT, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .SetCallback(&shard_gamepad_GetGamepadAxisCount);

    raylibGamepadBuilder
        .AddMethod(L"GetAxisMovement", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"gamepad", TYPE_INT)
        .AddParameter(L"axis", raylib_gamepadAxis)
        .SetCallback(&shard_gamepad_GetGamepadAxisMovement);

    // ------------------------------------------------------------------------
    // Touch
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibTouchBuilder = raylib.AddClass(L"Touch");

    raylibTouchBuilder
        .AddMethod(L"GetX", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_touch_GetTouchX);

    raylibTouchBuilder
        .AddMethod(L"GetY", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_touch_GetTouchY);

    raylibTouchBuilder
        .AddMethod(L"GetPosition", raylib_Vector2, LINK_STATIC)
        .AddParameter(L"index", TYPE_INT)
        .SetCallback(&shard_touch_GetTouchPosition);

    raylibTouchBuilder
        .AddMethod(L"GetPointId", TYPE_INT, LINK_STATIC)
        .AddParameter(L"index", TYPE_INT)
        .SetCallback(&shard_touch_GetTouchPointId);

    raylibTouchBuilder
        .AddMethod(L"GetPointCount", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_touch_GetTouchPointCount);

    // ------------------------------------------------------------------------
    // Gesture
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibGestureBuilder = raylib.AddClass(L"Gesture");

    raylibGestureBuilder
        .AddMethod(L"SetEnabled", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"flags", raylib_gesture)
        .SetCallback(&shard_gesture_SetGesturesEnabled);

    raylibGestureBuilder
        .AddMethod(L"IsDetected", TYPE_BOOL, LINK_STATIC)
        .AddParameter(L"gesture", raylib_gesture)
        .SetCallback(&shard_gesture_IsGestureDetected);

    raylibGestureBuilder
        .AddMethod(L"GetDetected", TYPE_INT, LINK_STATIC)
        .SetCallback(&shard_gesture_GetGestureDetected);

    raylibGestureBuilder
        .AddMethod(L"GetHoldDuration", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback(&shard_gesture_GetGestureHoldDuration);

    raylibGestureBuilder
        .AddMethod(L"GetDragVector", raylib_Vector2, LINK_STATIC)
        .SetCallback(&shard_gesture_GetGestureDragVector);

    raylibGestureBuilder
        .AddMethod(L"GetDragAngle", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback(&shard_gesture_GetGestureDragAngle);

    raylibGestureBuilder
        .AddMethod(L"GetPinchVector", raylib_Vector2, LINK_STATIC)
        .SetCallback(&shard_gesture_GetGesturePinchVector);

    raylibGestureBuilder
        .AddMethod(L"GetPinchAngle", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback(&shard_gesture_GetGesturePinchAngle);

    // ------------------------------------------------------------------------
    // Shapes
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibShapesBuilder = raylib.AddClass(L"Shapes");

    raylibShapesBuilder
        .AddMethod(L"DrawPixel", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"posX", TYPE_INT)
        .AddParameter(L"posY", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawPixel);

    raylibShapesBuilder
        .AddMethod(L"DrawPixelV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"position", raylib_Vector2)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawPixelV);

    raylibShapesBuilder
        .AddMethod(L"DrawLine", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"startX", TYPE_INT)
        .AddParameter(L"startY", TYPE_INT)
        .AddParameter(L"endX", TYPE_INT)
        .AddParameter(L"endY", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawLine);

    raylibShapesBuilder
        .AddMethod(L"DrawLineV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"startPos", raylib_Vector2)
        .AddParameter(L"endPos", raylib_Vector2)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawLineV);

    raylibShapesBuilder
        .AddMethod(L"DrawLineEx", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"startPos", raylib_Vector2)
        .AddParameter(L"endPos", raylib_Vector2)
        .AddParameter(L"thick", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawLineEx);

    raylibShapesBuilder
        .AddMethod(L"DrawLineBezier", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"startPos", raylib_Vector2)
        .AddParameter(L"endPos", raylib_Vector2)
        .AddParameter(L"thick", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawLineBezier);

    raylibShapesBuilder
        .AddMethod(L"DrawCircle", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"centerX", TYPE_INT)
        .AddParameter(L"centerY", TYPE_INT)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawCircle);

    raylibShapesBuilder
        .AddMethod(L"DrawCircleV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawCircleV);

    raylibShapesBuilder
        .AddMethod(L"DrawCircleGradient", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"inner", raylib_color)
        .AddParameter(L"outer", raylib_color)
        .SetCallback(&shard_shapes_DrawCircleGradient);

    raylibShapesBuilder
        .AddMethod(L"DrawCircleSector", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"startAngle", TYPE_DOUBLE)
        .AddParameter(L"endAngle", TYPE_DOUBLE)
        .AddParameter(L"segments", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawCircleSector);

    raylibShapesBuilder
        .AddMethod(L"DrawCircleSectorLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"startAngle", TYPE_DOUBLE)
        .AddParameter(L"endAngle", TYPE_DOUBLE)
        .AddParameter(L"segments", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawCircleSectorLines);

    raylibShapesBuilder
        .AddMethod(L"DrawCircleLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"centerX", TYPE_INT)
        .AddParameter(L"centerY", TYPE_INT)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawCircleLines);

    raylibShapesBuilder
        .AddMethod(L"DrawCircleLinesV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawCircleLinesV);

    raylibShapesBuilder
        .AddMethod(L"DrawEllipse", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"centerX", TYPE_INT)
        .AddParameter(L"centerY", TYPE_INT)
        .AddParameter(L"radiusH", TYPE_DOUBLE)
        .AddParameter(L"radiusV", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawEllipse);

    raylibShapesBuilder
        .AddMethod(L"DrawEllipseLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"centerX", TYPE_INT)
        .AddParameter(L"centerY", TYPE_INT)
        .AddParameter(L"radiusH", TYPE_DOUBLE)
        .AddParameter(L"radiusV", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawEllipseLines);

    raylibShapesBuilder
        .AddMethod(L"DrawRing", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"innerRadius", TYPE_DOUBLE)
        .AddParameter(L"outerRadius", TYPE_DOUBLE)
        .AddParameter(L"startAngle", TYPE_DOUBLE)
        .AddParameter(L"endAngle", TYPE_DOUBLE)
        .AddParameter(L"segments", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRing);

    raylibShapesBuilder
        .AddMethod(L"DrawRingLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"innerRadius", TYPE_DOUBLE)
        .AddParameter(L"outerRadius", TYPE_DOUBLE)
        .AddParameter(L"startAngle", TYPE_DOUBLE)
        .AddParameter(L"endAngle", TYPE_DOUBLE)
        .AddParameter(L"segments", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRingLines);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangle", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangle);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"position", raylib_Vector2)
        .AddParameter(L"size", raylib_Vector2)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleV);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleRec", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"rec", raylib_Rectangle)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleRec);

    raylibShapesBuilder
        .AddMethod(L"DrawRectanglePro", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"rec", raylib_Rectangle)
        .AddParameter(L"origin", raylib_Vector2)
        .AddParameter(L"rotation", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectanglePro);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleGradientV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"posX", TYPE_INT)
        .AddParameter(L"posY", TYPE_INT)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"top", raylib_color)
        .AddParameter(L"bottom", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleGradientV);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleGradientH", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"posX", TYPE_INT)
        .AddParameter(L"posY", TYPE_INT)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"left", raylib_color)
        .AddParameter(L"right", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleGradientH);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleGradientEx", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"rec", raylib_Rectangle)
        .AddParameter(L"topLeft", raylib_color)
        .AddParameter(L"bottomLeft", raylib_color)
        .AddParameter(L"bottomRight", raylib_color)
        .AddParameter(L"topRight", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleGradientEx);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"posX", TYPE_INT)
        .AddParameter(L"posY", TYPE_INT)
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleLines);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleLinesEx", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"rec", raylib_Rectangle)
        .AddParameter(L"lineThick", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleLinesEx);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleRounded", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"rec", raylib_Rectangle)
        .AddParameter(L"roundness", TYPE_DOUBLE)
        .AddParameter(L"segments", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleRounded);

    raylibShapesBuilder
        .AddMethod(L"DrawRectangleRoundedLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"rec", raylib_Rectangle)
        .AddParameter(L"roundness", TYPE_DOUBLE)
        .AddParameter(L"segments", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawRectangleRoundedLines);

    raylibShapesBuilder
        .AddMethod(L"DrawTriangle", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"v1", raylib_Vector2)
        .AddParameter(L"v2", raylib_Vector2)
        .AddParameter(L"v3", raylib_Vector2)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawTriangle);

    raylibShapesBuilder
        .AddMethod(L"DrawTriangleLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"v1", raylib_Vector2)
        .AddParameter(L"v2", raylib_Vector2)
        .AddParameter(L"v3", raylib_Vector2)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawTriangleLines);

    raylibShapesBuilder
        .AddMethod(L"DrawPoly", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"sides", TYPE_INT)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"rotation", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawPoly);

    raylibShapesBuilder
        .AddMethod(L"DrawPolyLines", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"sides", TYPE_INT)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"rotation", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawPolyLines);

    raylibShapesBuilder
        .AddMethod(L"DrawPolyLinesEx", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"center", raylib_Vector2)
        .AddParameter(L"sides", TYPE_INT)
        .AddParameter(L"radius", TYPE_DOUBLE)
        .AddParameter(L"rotation", TYPE_DOUBLE)
        .AddParameter(L"lineThick", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawPolyLinesEx);

    raylibShapesBuilder
        .AddMethod(L"DrawText", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"text", TYPE_STRING)
        .AddParameter(L"x", TYPE_INT)
        .AddParameter(L"y", TYPE_INT)
        .AddParameter(L"fontSize", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_shapes_DrawText);

    // ------------------------------------------------------------------------
    // Text
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibTextBuilder = raylib.AddClass(L"Text");

    raylibTextBuilder
        .AddMethod(L"Measure", TYPE_INT, LINK_STATIC)
        .AddParameter(L"text", TYPE_STRING)
        .AddParameter(L"fontSize", TYPE_INT)
        .SetCallback(&shard_text_MeasureText);

    raylibTextBuilder
        .AddMethod(L"SetLineSpacing", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"spacing", TYPE_INT)
        .SetCallback(&shard_text_SetTextLineSpacing);

    // ------------------------------------------------------------------------
    // Texture instance methods
    // ------------------------------------------------------------------------

    textureBuilder
        .AddMethod(L"Unload", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_texture_UnloadTexture);

    textureBuilder
        .AddMethod(L"Draw", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"posX", TYPE_INT)
        .AddParameter(L"posY", TYPE_INT)
        .AddParameter(L"tint", raylib_color)
        .SetCallback(&shard_texture_DrawTexture);

    textureBuilder
        .AddMethod(L"DrawV", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"position", raylib_Vector2)
        .AddParameter(L"tint", raylib_color)
        .SetCallback(&shard_texture_DrawTextureV);

    textureBuilder
        .AddMethod(L"DrawEx", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"position", raylib_Vector2)
        .AddParameter(L"rotation", TYPE_DOUBLE)
        .AddParameter(L"scale", TYPE_DOUBLE)
        .AddParameter(L"tint", raylib_color)
        .SetCallback(&shard_texture_DrawTextureEx);

    textureBuilder
        .AddMethod(L"DrawRec", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"source", raylib_Rectangle)
        .AddParameter(L"position", raylib_Vector2)
        .AddParameter(L"tint", raylib_color)
        .SetCallback(&shard_texture_DrawTextureRec);

    textureBuilder
        .AddMethod(L"DrawPro", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"source", raylib_Rectangle)
        .AddParameter(L"dest", raylib_Rectangle)
        .AddParameter(L"origin", raylib_Vector2)
        .AddParameter(L"rotation", TYPE_DOUBLE)
        .AddParameter(L"tint", raylib_color)
        .SetCallback(&shard_texture_DrawTexturePro);

    textureBuilder
        .AddMethod(L"IsValid", TYPE_BOOL, LINK_INSTANCE)
        .SetCallback(&shard_texture_IsTextureValid);

    textureBuilder
        .AddMethod(L"SetFilter", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"filter", TYPE_INT)
        .SetCallback(&shard_texture_SetTextureFilter);

    textureBuilder
        .AddMethod(L"SetWrap", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"wrap", TYPE_INT)
        .SetCallback(&shard_texture_SetTextureWrap);

    // ------------------------------------------------------------------------
    // Image instance methods
    // ------------------------------------------------------------------------

    imageBuilder
        .AddMethod(L"Unload", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_image_UnloadImage);

    imageBuilder
        .AddMethod(L"Export", TYPE_BOOL, LINK_INSTANCE)
        .AddParameter(L"fileName", TYPE_STRING)
        .SetCallback(&shard_image_ExportImage);

    imageBuilder
        .AddMethod(L"Copy", raylib_Image, LINK_INSTANCE)
        .SetCallback(&shard_image_ImageCopy);

    imageBuilder
        .AddMethod(L"Resize", raylib_Image, LINK_INSTANCE)
        .AddParameter(L"newWidth", TYPE_INT)
        .AddParameter(L"newHeight", TYPE_INT)
        .SetCallback(&shard_image_ImageResize);

    imageBuilder
        .AddMethod(L"FlipVertical", raylib_Image, LINK_INSTANCE)
        .SetCallback(&shard_image_ImageFlipVertical);

    imageBuilder
        .AddMethod(L"FlipHorizontal", raylib_Image, LINK_INSTANCE)
        .SetCallback(&shard_image_ImageFlipHorizontal);

    imageBuilder
        .AddMethod(L"ColorTint", raylib_Image, LINK_INSTANCE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_image_ImageColorTint);

    imageBuilder
        .AddMethod(L"ColorInvert", raylib_Image, LINK_INSTANCE)
        .SetCallback(&shard_image_ImageColorInvert);

    imageBuilder
        .AddMethod(L"ColorGrayscale", raylib_Image, LINK_INSTANCE)
        .SetCallback(&shard_image_ImageColorGrayscale);

    // ------------------------------------------------------------------------
    // Sound instance methods
    // ------------------------------------------------------------------------

    soundBuilder
        .AddMethod(L"Unload", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_sound_UnloadSound);

    soundBuilder
        .AddMethod(L"Play", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_sound_PlaySound);

    soundBuilder
        .AddMethod(L"Stop", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_sound_StopSound);

    soundBuilder
        .AddMethod(L"Pause", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_sound_PauseSound);

    soundBuilder
        .AddMethod(L"Resume", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_sound_ResumeSound);

    soundBuilder
        .AddMethod(L"IsPlaying", TYPE_BOOL, LINK_INSTANCE)
        .SetCallback(&shard_sound_IsSoundPlaying);

    soundBuilder
        .AddMethod(L"SetVolume", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"volume", TYPE_DOUBLE)
        .SetCallback(&shard_sound_SetSoundVolume);

    soundBuilder
        .AddMethod(L"SetPitch", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"pitch", TYPE_DOUBLE)
        .SetCallback(&shard_sound_SetSoundPitch);

    soundBuilder
        .AddMethod(L"SetPan", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"pan", TYPE_DOUBLE)
        .SetCallback(&shard_sound_SetSoundPan);

    // ------------------------------------------------------------------------
    // Music instance methods
    // ------------------------------------------------------------------------

    musicBuilder
        .AddMethod(L"Unload", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_music_UnloadMusicStream);

    musicBuilder
        .AddMethod(L"Play", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_music_PlayMusicStream);

    musicBuilder
        .AddMethod(L"Stop", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_music_StopMusicStream);

    musicBuilder
        .AddMethod(L"Pause", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_music_PauseMusicStream);

    musicBuilder
        .AddMethod(L"Resume", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_music_ResumeMusicStream);

    musicBuilder
        .AddMethod(L"IsPlaying", TYPE_BOOL, LINK_INSTANCE)
        .SetCallback(&shard_music_IsMusicStreamPlaying);

    musicBuilder
        .AddMethod(L"Update", TYPE_VOID, LINK_INSTANCE)
        .SetCallback(&shard_music_UpdateMusicStream);

    musicBuilder
        .AddMethod(L"SetVolume", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"volume", TYPE_DOUBLE)
        .SetCallback(&shard_music_SetMusicVolume);

    musicBuilder
        .AddMethod(L"SetPitch", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"pitch", TYPE_DOUBLE)
        .SetCallback(&shard_music_SetMusicPitch);

    musicBuilder
        .AddMethod(L"SetPan", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"pan", TYPE_DOUBLE)
        .SetCallback(&shard_music_SetMusicPan);

    musicBuilder
        .AddMethod(L"GetTimeLength", TYPE_DOUBLE, LINK_INSTANCE)
        .SetCallback(&shard_music_GetMusicTimeLength);

    musicBuilder
        .AddMethod(L"GetTimePlayed", TYPE_DOUBLE, LINK_INSTANCE)
        .SetCallback(&shard_music_GetMusicTimePlayed);

    // ------------------------------------------------------------------------
    // Audio device
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibAudioBuilder = raylib.AddClass(L"Audio");

    raylibAudioBuilder
        .AddMethod(L"InitDevice", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_audio_InitAudioDevice);

    raylibAudioBuilder
        .AddMethod(L"CloseDevice", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_audio_CloseAudioDevice);

    raylibAudioBuilder
        .AddMethod(L"IsDeviceReady", TYPE_BOOL, LINK_STATIC)
        .SetCallback(&shard_audio_IsAudioDeviceReady);

    raylibAudioBuilder
        .AddMethod(L"SetMasterVolume", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"volume", TYPE_DOUBLE)
        .SetCallback(&shard_audio_SetMasterVolume);

    // ------------------------------------------------------------------------
    // Color
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibColorBuilder = raylib.AddClass(L"Color");

    raylibColorBuilder
        .AddMethod(L"FromRGBA", raylib_color, LINK_STATIC)
        .AddParameter(L"r", TYPE_INT)
        .AddParameter(L"g", TYPE_INT)
        .AddParameter(L"b", TYPE_INT)
        .AddParameter(L"a", TYPE_INT)
        .SetCallback([](const CallState& context) noexcept -> ObjectInstance* {
            return PackColor(context.Collector, Color{
                static_cast<unsigned char>(ArgInt(context.Args[0])),
                static_cast<unsigned char>(ArgInt(context.Args[1])),
                static_cast<unsigned char>(ArgInt(context.Args[2])),
                static_cast<unsigned char>(ArgInt(context.Args[3]))
            });
        });

    // ------------------------------------------------------------------------
    // Camera3D methods
    // ------------------------------------------------------------------------

    camera3DBuilder
        .AddMethod(L"Update", TYPE_VOID, LINK_INSTANCE)
        .AddParameter(L"mode", raylib_cameraMode)
        .SetCallback(&shard_3d_UpdateCamera);

    // ------------------------------------------------------------------------
    // 3D drawing
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibD3Builder = raylib.AddClass(L"D3");

    raylibD3Builder
        .AddMethod(L"BeginMode", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"camera", raylib_Camera3D)
        .SetCallback(&shard_3d_BeginMode3D);

    raylibD3Builder
        .AddMethod(L"EndMode", TYPE_VOID, LINK_STATIC)
        .SetCallback(&shard_3d_EndMode3D);

    raylibD3Builder
        .AddMethod(L"DrawCube", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"position", raylib_Vector3)
        .AddParameter(L"width", TYPE_DOUBLE)
        .AddParameter(L"height", TYPE_DOUBLE)
        .AddParameter(L"length", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_3d_DrawCube);

    raylibD3Builder
        .AddMethod(L"DrawCube", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"x", TYPE_DOUBLE)
        .AddParameter(L"y", TYPE_DOUBLE)
        .AddParameter(L"z", TYPE_DOUBLE)
        .AddParameter(L"width", TYPE_DOUBLE)
        .AddParameter(L"height", TYPE_DOUBLE)
        .AddParameter(L"length", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_3d_DrawCubeRaw);

    raylibD3Builder
        .AddMethod(L"DrawCubeV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"position", raylib_Vector3)
        .AddParameter(L"size", raylib_Vector3)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_3d_DrawCubeV);

    raylibD3Builder
        .AddMethod(L"DrawCubeWires", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"position", raylib_Vector3)
        .AddParameter(L"width", TYPE_DOUBLE)
        .AddParameter(L"height", TYPE_DOUBLE)
        .AddParameter(L"length", TYPE_DOUBLE)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_3d_DrawCubeWires);

    raylibD3Builder
        .AddMethod(L"DrawCubeWiresV", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"position", raylib_Vector3)
        .AddParameter(L"size", raylib_Vector3)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_3d_DrawCubeWiresV);

    raylibD3Builder
        .AddMethod(L"DrawWalls", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"map", raylibD3Builder.GetFactory().Array(TYPE_INT))
        .AddParameter(L"width", TYPE_INT)
        .AddParameter(L"height", TYPE_INT)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_3d_DrawWalls);

    raylibD3Builder
        .AddMethod(L"DrawLine", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"start", raylib_Vector3)
        .AddParameter(L"end", raylib_Vector3)
        .AddParameter(L"color", raylib_color)
        .SetCallback(&shard_3d_DrawLine3D);

    raylibD3Builder
        .AddMethod(L"DrawGrid", TYPE_VOID, LINK_STATIC)
        .AddParameter(L"slices", TYPE_INT)
        .AddParameter(L"spacing", TYPE_DOUBLE)
        .SetCallback(&shard_3d_DrawGrid);

    // ------------------------------------------------------------------------
    // Math
    // ------------------------------------------------------------------------

    SymbolBuilder<ClassSymbol> raylibMathBuilder = raylib.AddClass(L"Math");

    raylibMathBuilder
        .AddMethod(L"PI", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback([](const CallState& context) noexcept -> ObjectInstance* {
            return context.Collector.FromValue(3.14159265358979323846);
        });

    raylibMathBuilder
        .AddMethod(L"Deg2Rad", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback([](const CallState& context) noexcept -> ObjectInstance* {
            return context.Collector.FromValue(0.01745329251994329577);
        });

    raylibMathBuilder
        .AddMethod(L"Rad2Deg", TYPE_DOUBLE, LINK_STATIC)
        .SetCallback([](const CallState& context) noexcept -> ObjectInstance* {
            return context.Collector.FromValue(57.2957795130823208768);
        });

    // ------------------------------------------------------------------------
    // Store top-level type symbols
    // ------------------------------------------------------------------------

    raylib_Window = raylibWindowBuilder;
    raylib_Cursor = raylibCursorBuilder;
    raylib_Time = raylibTimeBuilder;
    raylib_Random = raylibRandomBuilder;
    raylib_Keyboard = raylibKeyboardBuilder;
    raylib_Mouse = raylibMouseBuilder;
    raylib_Gamepad = raylibGamepadBuilder;
    raylib_Touch = raylibTouchBuilder;
    raylib_Gesture = raylibGestureBuilder;
    raylib_Shapes = raylibShapesBuilder;
    raylib_Text = raylibTextBuilder;
    raylib_D3 = raylibD3Builder;
}
