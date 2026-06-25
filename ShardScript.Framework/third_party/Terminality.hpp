// ===========================================================================
// Terminality Single Header Library
// Auto-generated. Do not edit directly.
// 
// USAGE:
// Include this file normally where you need the Terminality API.
// In EXACTLY ONE .cpp file, define TERMINALITY_IMPLEMENTATION before including:
// 
// #define TERMINALITY_IMPLEMENTATION
// #include "Terminality.hpp"
// ===========================================================================

#ifndef TERMINALITY_SINGLE_HEADER_H
#define TERMINALITY_SINGLE_HEADER_H

// Disable DLL linkage for Single Header build
#ifndef TERMINALITY_STATIC
#define TERMINALITY_STATIC
#endif

// --- Standard Library Includes ---
#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

// --- Begin Header: out/Terminality.hpp ---
// ===========================================================================
// Terminality Single Header Library
// Auto-generated. Do not edit directly.
//
// USAGE:
// Include this file normally where you need the Terminality API.
// In EXACTLY ONE .cpp file, define TERMINALITY_IMPLEMENTATION before including:
//
// #define TERMINALITY_IMPLEMENTATION
// #include "Terminality.hpp"
// ===========================================================================

#ifndef TERMINALITY_SINGLE_HEADER_H
#define TERMINALITY_SINGLE_HEADER_H

// Disable DLL linkage for Single Header build
#ifndef TERMINALITY_STATIC
#define TERMINALITY_STATIC
#endif

#endif // TERMINALITY_SINGLE_HEADER_H

#ifdef TERMINALITY_IMPLEMENTATION

#endif // TERMINALITY_IMPLEMENTATION
// --- End Header: out/Terminality.hpp ---

// --- Begin Header: terminality/Core/Geometry.hpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)

namespace terminality
{
	struct Point
	{
		static const Point Zero;

		int32_t X;
		int32_t Y;

		Point(int32_t x = 0, int32_t y = 0)
			: X(x), Y(y) { }

		bool operator==(const Point& other) const;
		bool operator!=(const Point& other) const;
	};

	struct Vector
	{
		Point From;
		Point To;

		Vector(Point from = Point(), Point to = Point())
			: From(from), To(to) { }

		Vector(int32_t fromX, int32_t fromY, int32_t toX, int32_t toY)
			: From(fromX, fromY), To(toX, toY) { }

		bool operator==(const Vector& other) const;
		bool operator!=(const Vector& other) const;
	};

	struct Size
	{
		static const Size Zero;
		static const Size Auto;

		int32_t Width;
		int32_t Height;

		Size(int32_t width = 0, int32_t height = 0)
			: Width(width), Height(height) { }

		Size(Vector diagonal)
			: Width(std::max(0, diagonal.To.X - diagonal.From.X)), Height(std::max(0, diagonal.To.Y - diagonal.From.Y)) { }

		bool operator==(const Size& other) const;
		bool operator!=(const Size& other) const;
	};

	struct Thickness
	{
		static const Thickness Zero;
		static const Thickness Single;

		int32_t Left;
		int32_t Top;
		int32_t Right;
		int32_t Bottom;

		Thickness(int32_t uniform = 0)
			: Left(uniform), Top(uniform), Right(uniform), Bottom(uniform) { }

		Thickness(int32_t left, int32_t top, int32_t right, int32_t bottom)
			: Left(left), Top(top), Right(right), Bottom(bottom) { }

		bool operator==(const Thickness& other) const;
		bool operator!=(const Thickness& other) const;

		bool IsUniform() const;

		int32_t Horizontal() const
		{
			return Left + Right;
		}

		int32_t Vertical() const
		{
			return Top + Bottom;
		}
	};

	struct Rect
	{
		int32_t X;
		int32_t Y;
		int32_t Width;
		int32_t Height;

		Rect(int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0)
			: X(x), Y(y), Width(width), Height(height) { }

		bool operator==(const Rect& other) const;
		bool operator!=(const Rect& other) const;

		bool IsEmpty() const;
		int32_t Right() const;
		int32_t Bottom() const;
		Size AsSize() const;

		bool Contains(Point point) const;
		bool Contains(Rect inner) const;
		bool Intersects(Rect other) const;

		static Rect Union(const Rect& a, const Rect& b);
		static Rect Enclose(const Rect& into, const Rect& rect);
		static Rect Clip(const Rect& into, const Rect& rect);
	};
}
// --- End Header: terminality/Core/Geometry.hpp ---

// --- Begin Header: terminality/Core/InputEvent.hpp ---

// #include <cstdint> (Moved to top)
// #include <functional> (Moved to top)

namespace terminality
{
    enum class InputKey
    {
        None = -1,
        LBUTTON = 0x01,             // Left mouse button
        RBUTTON = 0x02,             // Right mouse button
        CANCEL = 0x03,              // Control-break processing
        MBUTTON = 0x04,             // Middle mouse button (three-button mouse)
        XBUTTON1 = 0x05,            // X1 mouse button
        XBUTTON2 = 0x06,            // X2 mouse button
        BACK = 0x08,                // BACKSPACE key
        TAB = 0x09,                 // TAB key
        CLEAR = 0x0C,               // CLEAR key
        RETURN = 0x0D,              // ENTER key
        MENU = 0x12,                // ALT key
        PAUSE = 0x13,               // PAUSE key
        CAPITAL = 0x14,             // CAPS LOCK key
        KANA = 0x15,                // IME Kana mode
        HANGUEL = 0x15,             // IME Hanguel mode (maintained for compatibility; use `VK_HANGUL`)
        HANGUL = 0x15,              // IME Hangul mode
        IME_ON = 0x16,              // IME On
        JUNJA = 0x17,               // IME Junja mode
        FINAL = 0x18,               // IME final mode
        HANJA = 0x19,               // IME Hanja mode
        KANJI = 0x19,               // IME Kanji mode
        IME_OFF = 0x1A,             // IME Off
        ESCAPE = 0x1B,              // ESC key
        CONVERT = 0x1C,             // IME convert
        NONCONVERT = 0x1D,          // IME nonconvert
        ACCEPT = 0x1E,              // IME accept
        MODECHANGE = 0x1F,          // IME mode change request
        SPACE = 0x20,               // SPACEBAR
        PRIOR = 0x21,               // PAGE UP key
        NEXT = 0x22,                // PAGE DOWN key
        END = 0x23,                 // END key
        HOME = 0x24,                // HOME key
        LEFT = 0x25,                // LEFT ARROW key
        UP = 0x26,                  // UP ARROW key
        RIGHT = 0x27,               // RIGHT ARROW key
        DOWN = 0x28,                // DOWN ARROW key
        SELECT = 0x29,              // SELECT key
        PRINT = 0x2A,               // PRINT key
        EXECUTE = 0x2B,             // EXECUTE key
        SNAPSHOT = 0x2C,            // PRINT SCREEN key
        INSERT = 0x2D,              // INS key
        DELETE = 0x2E,              // DEL key
        HELP = 0x2F,                // HELP key
        NUM0 = 0x30,                // 0 key
        NUM1 = 0x31,                // 1 key
        NUM2 = 0x32,                // 2 key
        NUM3 = 0x33,                // 3 key
        NUM4 = 0x34,                // 4 key
        NUM5 = 0x35,                // 5 key
        NUM6 = 0x36,                // 6 key
        NUM7 = 0x37,                // 7 key
        NUM8 = 0x38,                // 8 key
        NUM9 = 0x39,                // 9 key
        A = 0x41,                   // A key
        B = 0x42,                   // B key
        C = 0x43,                   // C key
        D = 0x44,                   // D key
        E = 0x45,                   // E key
        F = 0x46,                   // F key
        G = 0x47,                   // G key
        H = 0x48,                   // H key
        I = 0x49,                   // I key
        J = 0x4A,                   // J key
        K = 0x4B,                   // K key
        L = 0x4C,                   // L key
        M = 0x4D,                   // M key
        N = 0x4E,                   // N key
        O = 0x4F,                   // O key
        P = 0x50,                   // P key
        Q = 0x51,                   // Q key
        R = 0x52,                   // R key
        S = 0x53,                   // S key
        T = 0x54,                   // T key
        U = 0x55,                   // U key
        V = 0x56,                   // V key
        W = 0x57,                   // W key
        X = 0x58,                   // X key
        Y = 0x59,                   // Y key
        Z = 0x5A,                   // Z key
        LWIN = 0x5B,                // Left Windows key (Natural keyboard)
        RWIN = 0x5C,                // Right Windows key (Natural keyboard)
        APPS = 0x5D,                // Applications key (Natural keyboard)
        SLEEP = 0x5F,               // Computer Sleep key
        NUMPAD0 = 0x60,             // Numeric keypad 0 key
        NUMPAD1 = 0x61,             // Numeric keypad 1 key
        NUMPAD2 = 0x62,             // Numeric keypad 2 key
        NUMPAD3 = 0x63,             // Numeric keypad 3 key
        NUMPAD4 = 0x64,             // Numeric keypad 4 key
        NUMPAD5 = 0x65,             // Numeric keypad 5 key
        NUMPAD6 = 0x66,             // Numeric keypad 6 key
        NUMPAD7 = 0x67,             // Numeric keypad 7 key
        NUMPAD8 = 0x68,             // Numeric keypad 8 key
        NUMPAD9 = 0x69,             // Numeric keypad 9 key
        MULTIPLY = 0x6A,            // Multiply key
        ADD = 0x6B,                 // Add key
        SEPARATOR = 0x6C,           // Separator key
        SUBTRACT = 0x6D,            // Subtract key
        DECIMAL = 0x6E,             // Decimal key
        DIVIDE = 0x6F,              // Divide key
        F1 = 0x70,                  // F1 key
        F2 = 0x71,                  // F2 key
        F3 = 0x72,                  // F3 key
        F4 = 0x73,                  // F4 key
        F5 = 0x74,                  // F5 key
        F6 = 0x75,                  // F6 key
        F7 = 0x76,                  // F7 key
        F8 = 0x77,                  // F8 key
        F9 = 0x78,                  // F9 key
        F10 = 0x79,                 // F10 key
        F11 = 0x7A,                 // F11 key
        F12 = 0x7B,                 // F12 key
        F13 = 0x7C,                 // F13 key
        F14 = 0x7D,                 // F14 key
        F15 = 0x7E,                 // F15 key
        F16 = 0x7F,                 // F16 key
        F17 = 0x80,                 // F17 key
        F18 = 0x81,                 // F18 key
        F19 = 0x82,                 // F19 key
        F20 = 0x83,                 // F20 key
        F21 = 0x84,                 // F21 key
        F22 = 0x85,                 // F22 key
        F23 = 0x86,                 // F23 key
        F24 = 0x87,                 // F24 key
        NUMLOCK = 0x90,             // NUM LOCK key
        SCROLL = 0x91,              // SCROLL LOCK key
        LSHIFT = 0xA0,              // Left SHIFT key
        RSHIFT = 0xA1,              // Right SHIFT key
        LCONTROL = 0xA2,            // Left CONTROL key
        RCONTROL = 0xA3,            // Right CONTROL key
        LMENU = 0xA4,               // Left ALT key
        RMENU = 0xA5,               // Right ALT key
        BROWSER_BACK = 0xA6,        // Browser Back key
        BROWSER_FORWARD = 0xA7,     // Browser Forward key
        BROWSER_REFRESH = 0xA8,     // Browser Refresh key
        BROWSER_STOP = 0xA9,        // Browser Stop key
        BROWSER_SEARCH = 0xAA,      // Browser Search key
        BROWSER_FAVORITES = 0xAB,   // Browser Favorites key
        BROWSER_HOME = 0xAC,        // Browser Start and Home key
        VOLUME_MUTE = 0xAD,         // Volume Mute key
        VOLUME_DOWN = 0xAE,         // Volume Down key
        VOLUME_UP = 0xAF,           // Volume Up key
        MEDIA_NEXT_TRACK = 0xB0,    // Next Track key
        MEDIA_PREV_TRACK = 0xB1,    // Previous Track key
        MEDIA_STOP = 0xB2,          // Stop Media key
        MEDIA_PLAY_PAUSE = 0xB3,    // Play/Pause Media key
        LAUNCH_MAIL = 0xB4,         // Start Mail key
        LAUNCH_MEDIA_SELECT = 0xB5, // Select Media key
        LAUNCH_APP1 = 0xB6,         // Start Application 1 key
        LAUNCH_APP2 = 0xB7,         // Start Application 2 key
        OEM_1 = 0xBA,               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the ';:' key
        OEM_PLUS = 0xBB,            // For any country/region, the '+' key
        OEM_COMMA = 0xBC,           // For any country/region, the ',' key
        OEM_MINUS = 0xBD,           // For any country/region, the '-' key
        OEM_PERIOD = 0xBE,          // For any country/region, the '.' key
        OEM_2 = 0xBF,               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '/?' key
        OEM_3 = 0xC0,               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\`~' key
        OEM_4 = 0xDB,               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\[{' key
        OEM_5 = 0xDC,               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\\\|' key
        OEM_6 = 0xDD,               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the '\]}' key
        OEM_7 = 0xDE,               // Used for miscellaneous characters; it can vary by keyboard. For the US standard keyboard, the 'single-quote/double-quote' key
        OEM_8 = 0xDF,               // Used for miscellaneous characters; it can vary by keyboard.
        OEM_102 = 0xE2,             // The `<>` keys on the US standard keyboard, or the `\\|` key on the non-US 102-key keyboard
        PROCESSKEY = 0xE5,          // IME PROCESS key
        PACKET = 0xE7,              // Used to pass Unicode characters as if they were keystrokes. The `VK_PACKET` key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in [`KEYBDINPUT`](/windows/win32/api/winuser/ns-winuser-keybdinput), [`SendInput`](/windows/win32/api/winuser/nf-winuser-sendinput), [`WM_KEYDOWN`](wm-keydown.md), and [`WM_KEYUP`](wm-keyup.md)
        ATTN = 0xF6,                // Attn key
        CRSEL = 0xF7,               // CrSel key
        EXSEL = 0xF8,               // ExSel key
        EREOF = 0xF9,               // Erase EOF key
        PLAY = 0xFA,                // Play key
        ZOOM = 0xFB,                // Zoom key
        NONAME = 0xFC,              // Reserved
        PA1 = 0xFD,                 // PA1 key
        OEM_CLEAR = 0xFE,           // Clear key

        CHAR = 0xFC,                // Got char (Internal)
    };

    enum class InputModifier
    {
        None = 0,

        LeftAlt = 1 << 0,
        RightAlt = 1 << 1,
        Alt = LeftAlt | RightAlt,

        LeftCtrl = 1 << 2,
        RightCtrl = 1 << 3,
        Ctrl = LeftCtrl | RightCtrl,

        Shift = 1 << 4,
        NumLockOn = 1 << 5,
        ScrollLockOn = 1 << 6,
        CapsLockOn = 1 << 7,
        Special = 1 << 20,
    };

    constexpr InputModifier operator|(InputModifier a, InputModifier b)
    {
        return static_cast<InputModifier>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr InputModifier operator&(InputModifier a, InputModifier b)
    {
        return static_cast<InputModifier>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    struct InputEvent
    {
    	InputModifier Modifier = InputModifier::None;
    	InputKey Key = InputKey::None;
    	wchar_t Char = L'\0';
    	bool Pressed = false;

        InputEvent(wchar_t ch, bool pressed)
            : Key(InputKey::CHAR), Char(ch), Pressed(pressed) { }

    	InputEvent(InputModifier modifier, InputKey key, bool pressed)
            : Modifier(modifier), Key(key), Pressed(pressed) { }

    	InputEvent(InputModifier modifier, InputKey key, wchar_t ch, bool pressed)
            : Modifier(modifier), Key(key), Char(ch), Pressed(pressed) { }

        bool operator==(const InputEvent& other) const = default;
    };

    struct InputEventHasher
    {
        std::size_t operator()(const InputEvent& e) const
        {
            return
                (std::hash<int>()(static_cast<int>(e.Modifier)) << 0) ^
                (std::hash<int>()(static_cast<int>(e.Key)) << 1) ^
                (std::hash<wchar_t>()(e.Char) << 2) ^
                (std::hash<bool>()(e.Pressed) << 3);
        }
    };

    template <typename T>
    bool hasFlag(T value, T flag)
    {
    	return (static_cast<int>(value) & static_cast<int>(flag)) == static_cast<int>(flag);
    }
}
// --- End Header: terminality/Core/InputEvent.hpp ---

// --- Begin Header: terminality/Core/Color.hpp ---

namespace terminality
{
	enum class Color
	{
		TRANSPARENT = -1,
		BLACK = 0,
		DARK_BLUE = 1,
		DARK_GREEN = 2,
		DARK_CYAN = 3,
		DARK_RED = 4,
		DARK_MAGENTA = 5,
		DARK_YELLOW = 6,
		LIGHT_GRAY = 7,
		DARK_GRAY = 8,
		BLUE = 9,
		GREEN = 10,
		CYAN = 11,
		RED = 12,
		MAGENTA = 13,
		YELLOW = 14,
		WHITE = 15,
	};
}
// --- End Header: terminality/Core/Color.hpp ---

// --- Begin Header: terminality/Core/Layout.hpp ---

namespace terminality
{
	enum class HorizontalAlign
	{
		Left,
		Center,
		Right,
		Stretch
	};

	enum class VerticalAlign
	{
		Top,
		Center,
		Bottom,
		Stretch
	};

	enum class TextWrap
	{
		NoWrap,
		Wrap,
		WrapWholeWords
	};

	enum class TextAlign
	{
		Left,
		Center,
		Right,
		Justify
	};

	enum class Orientation
	{
		Vertical,
		Horizontal
	};
}
// --- End Header: terminality/Core/Layout.hpp ---

// --- Begin Header: terminality/Engine/RenderStream.hpp ---

// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)
// #include <optional> (Moved to top)

// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)

namespace terminality
{
    class RenderContext;

    struct RenderStreamColor
    {
        std::optional<Color> Foreground;
        std::optional<Color> Background;
    };

    class RenderStream
    {
        RenderContext& context_;
        Point pos_;
        Color fg_;
        Color bg_;
        bool wrap_;

    public:
        RenderStream(RenderContext& context, Point startPos = Point(0, 0))
            : context_(context), pos_(startPos), fg_(Color::WHITE), bg_(Color::BLACK), wrap_(false) {}

        RenderStream& operator<<(const Point& point)
        {
            pos_ = point;
            return *this;
        }

        RenderStream& operator<<(const RenderStreamColor& color)
        {
            fg_ = color.Foreground.value_or(fg_);
            bg_ = color.Background.value_or(bg_);
            return *this;
        }

        RenderStream& operator<<(const std::wstring& text);
        RenderStream& operator<<(const std::string& text);
        RenderStream& operator<<(const wchar_t* text);
        RenderStream& operator<<(const char* text);
        RenderStream& operator<<(int32_t value);
        RenderStream& operator<<(uint32_t value);
        RenderStream& operator<<(float value);
        RenderStream& operator<<(double value);
        RenderStream& operator<<(RenderStream& (*manipulator)(RenderStream&));

        void NewLine();
    };

    inline RenderStreamColor SetColor(Color fg, Color bg = Color::BLACK)
    {
        return RenderStreamColor{ fg, bg };
    }

    inline RenderStreamColor SetBack(Color bg)
    {
        return RenderStreamColor{ std::nullopt, bg };
    }

    inline RenderStreamColor SetFore(Color fg)
    {
        return RenderStreamColor{ fg, std::nullopt };
    }

    inline RenderStream& endl(RenderStream& stream)
    {
        stream.NewLine();
        return stream;
    }
}
// --- End Header: terminality/Engine/RenderStream.hpp ---

// --- Begin Header: terminality/Engine/RenderBuffer.hpp ---

// #include <cstdint> (Moved to top)
// #include <vector> (Moved to top)
// #include <mutex> (Moved to top)

// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)

namespace terminality
{
	struct CellInfo
	{
		wchar_t Symbol = L' ';
		Color Fore = Color::WHITE;
		Color Back = Color::BLACK;

		CellInfo() = default;

		CellInfo(wchar_t symbol, Color fore = Color::WHITE, Color back = Color::BLACK)
			: Symbol(symbol), Fore(fore), Back(back) { }

		bool operator==(const CellInfo& other) const
		{
			return Symbol == other.Symbol && Fore == other.Fore && Back == other.Back;
		}

		bool operator!=(const CellInfo& other) const
		{
			return !(*this == other);
		}
	};

	class RenderBuffer
	{
		friend class RenderContext;

		std::recursive_mutex renderMutex;

		uint32_t snapshotWidth = 0;
		uint32_t snapshotHeight = 0;
		std::vector<CellInfo> snapshotBuffer;

		uint32_t width = 0;
		uint32_t height = 0;
		std::vector<CellInfo> buffer;

		bool hasDirtyRect = true;
		Rect dirtyRect;

		std::size_t GetIndex(uint32_t x, uint32_t y) const;
		void MarkDirty(const Rect& rect);

	public:
		static constexpr std::size_t MAX_WIDTH = 512;
		static constexpr std::size_t MAX_HEIGHT = 256;

		RenderBuffer(uint32_t initialWidth, uint32_t initialHeight);

		uint32_t Width() const { return width; }
		uint32_t Height() const { return height; }
		void Resize(uint32_t newWidth, uint32_t newHeight);
		void Clear(const CellInfo& cell = CellInfo());

		void SetCell(uint32_t x, uint32_t y, const CellInfo& cell);
		const CellInfo& GetCell(uint32_t x, uint32_t y) const;

		void Snapshot();
		void DiffRender(std::wostream& out);
		void BulkRender(std::wostream& out);

		const wchar_t* GetAsniBg(Color color);
		const wchar_t* GetAsniFg(Color color);
	};
}
// --- End Header: terminality/Engine/RenderBuffer.hpp ---

// --- Begin Header: terminality/Engine/RenderContext.hpp ---


// #include <string> (Moved to top)
// #include <cstdint> (Moved to top)

// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Engine/RenderBuffer.hpp" (Merged)
// #include "terminality/Engine/RenderStream.hpp" (Merged)

namespace terminality
{
	typedef wchar_t (*RectangleStyle)(const Point& point, const Size& size);
	typedef wchar_t (*VectorStyle)(const Point& point, const Vector& vector);

	class RenderContext
	{
		RenderBuffer& buffer_;
		Rect rect_;

	public:
		RenderContext(RenderBuffer& buffer, Rect targetRect);
		RenderContext CreateInner(Rect targetRect);

		Rect ContextRect();

		void SetCell(uint32_t x, uint32_t y, const CellInfo& cell);
		void SetCell(uint32_t x, uint32_t y, const wchar_t puts, Color fg = Color::WHITE, Color bg = Color::BLACK);
		const CellInfo& GetCell(uint32_t x, uint32_t y) const;

		void RenderRaw(const Point& point, const std::string& rawData);
		RenderStream BeginText(Point startPos = Point(0, 0));

		void RenderText(const Point& point, const std::string& text, Color fg = Color::WHITE, Color bg = Color::BLACK, bool wrap = false);
		void RenderText(const Point& point, const std::wstring& text, Color fg = Color::WHITE, Color bg = Color::BLACK, bool wrap = false);
		void RenderText(const Point& point, const char* text, Color fg = Color::WHITE, Color bg = Color::BLACK, bool wrap = false);
		void RenderText(const Point& point, const wchar_t* text, Color fg = Color::WHITE, Color bg = Color::BLACK, bool wrap = false);

		void RenderRectangle(const Point& point, const Size& size);
		void RenderRectangle(const Point& point, const Size& size, Color fg, Color bg);
		void RenderRectangle(const Point& point, const Size& size, RectangleStyle style);
		void RenderRectangle(const Point& point, const Size& size, Color fg, Color bg, RectangleStyle style);
		void RenderRectangle(const Point& point, const int32_t width, const int32_t height);
		void RenderRectangle(const Point& point, const int32_t width, const int32_t height, Color fg, Color bg);
		void RenderRectangle(const Point& point, const int32_t width, const int32_t height, RectangleStyle style);
		void RenderRectangle(const Point& point, const int32_t width, const int32_t height, Color fg, Color bg, RectangleStyle style);

		void RenderLine(const Point& point, const int32_t length, short direction = 0);
		void RenderLine(const Point& point, const int32_t length, VectorStyle style);
		void RenderLine(const Point& fromPoint, const Point& toPoint);
		void RenderLine(const Point& fromPoint, const Point& toPoint, VectorStyle style);
		void RenderLine(const Vector& vector);
		void RenderLine(const Vector& vector, VectorStyle style);
	};
}
// --- End Header: terminality/Engine/RenderContext.hpp ---

// --- Begin Header: terminality/Core/Focus.hpp ---

namespace terminality
{
	enum class Direction
	{
		Up,
		Down,
		Left,
		Right,
		Next,	 // Tab
		Previous // Shift + Tab
	};
}
// --- End Header: terminality/Core/Focus.hpp ---

// --- Begin Header: terminality/Framework/VisualTreeNode.hpp ---

// #include <cstdint> (Moved to top)

// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)

namespace terminality
{
	struct UILayer;

	class VisualTreeNode
	{
		friend class VisualTree;

	protected:
		bool measureDirty_ = true;
		bool arrangeDirty_ = true;
		bool visualDirty_ = true;

		bool attached_ = false;
		bool focusable_ = true;
		bool focused_ = false;
		bool isTabStop_ = false;
		int tabIndex_ = 0;

		Size actualSize_;
		Rect arrangedRect_;

		UILayer* layer_;
		VisualTreeNode* parent_ = nullptr;

		static void PopFocus(Direction direction, InputModifier modifiers);
		static void PushFocus(VisualTreeNode* focused);

	public:
		VisualTreeNode() = default;
		virtual ~VisualTreeNode() = default;

		// Copy and move semantics
		VisualTreeNode(const VisualTreeNode&) = delete;
		VisualTreeNode& operator=(const VisualTreeNode&) = delete;

		// Tree structure
		VisualTreeNode* GetParent() const;
		virtual void SetParent(VisualTreeNode* parent) = 0;
		virtual void SetLayer(UILayer* layer) = 0;

		virtual size_t VisualChildrenCount() const = 0;
		virtual VisualTreeNode* GetVisualChild(std::size_t index) const = 0;

		// Layout
		void InvalidateMeasure();
		void InvalidateArrange();
		void InvalidateVisual();

		virtual Size Measure(const Size& availableSize) = 0;
		virtual void Arrange(const Rect& finalRect) = 0;
		virtual void Render(RenderContext& context) = 0;

		// Getters
		bool IsAttached() const;
		bool IsMeasureDirty() const;
		bool IsArrangeDirty() const;
		bool IsVisualDirty() const;

		virtual bool IsFocusable() const;
		virtual bool IsTabStop() const;
		virtual int GetTabIndex() const;

		// Setters
		virtual void SetFocusable(bool value) = 0;
		virtual void SetTabStop(bool value) = 0;
		virtual void SetTabIndex(int value) = 0;

		// User input
		virtual bool OnKeyDown(InputEvent input) = 0;
		virtual bool OnKeyUp(InputEvent input) = 0;

		// Focus management
		virtual bool MoveFocusNext(Direction direction, InputModifier modifiers = InputModifier::None);
		virtual void OnGotFocus();
		virtual void OnLostFocus();

		// Tree invalidation
		virtual void OnChildInvalidated(VisualTreeNode& child);
		virtual void OnAttachedToTree();
		virtual void OnDettachedFromTree();

	//protected:
		Size GetActualSize() const;
		Rect GetArrangedRect() const;
	};
}
// --- End Header: terminality/Framework/VisualTreeNode.hpp ---

// --- Begin Header: terminality/Framework/Event.hpp ---

// #include <unordered_map> (Moved to top)
// #include <functional> (Moved to top)
// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)

namespace terminality
{
	template<typename... Args>
	class Event;

	template<typename... Args>
	using Handler = std::function<void(Args...)>;

	template<typename... Args>
	class EventConnection
	{
		friend class Event<Args...>;

		std::weak_ptr<Event<Args...>*> tracker_;
		std::size_t id_ = 0;

		EventConnection(std::weak_ptr<Event<Args...>*> tracker, std::size_t id);

	public:
		EventConnection() = default;

		EventConnection(EventConnection<Args...>&& other) noexcept;
		EventConnection& operator=(EventConnection<Args...>&& other) noexcept;

		EventConnection(const EventConnection<Args...>&) = delete;
		EventConnection& operator=(const EventConnection<Args...>&) = delete;

		~EventConnection();

		void Disconnect();
	};

	template<typename... Args>
	class Event
	{
		friend class EventConnection<Args...>;

		std::shared_ptr<Event<Args...>*> selfToken_;
		std::unordered_map<std::size_t, Handler<Args...>> handlers_;
		std::size_t nextId_ = 1;

	public:
		Event();
		~Event();

		Event(Event&& other) noexcept;
		Event& operator=(Event&& other) noexcept;

		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;

		void operator+=(Handler<Args...> handler);
		[[nodiscard]] EventConnection<Args...> Connect(Handler<Args...> handler);
		void Emit(Args... args);

	private:
		void Disconnect(std::size_t id);
	};
}

template<typename ...Args>
terminality::EventConnection<Args...>::EventConnection(std::weak_ptr<Event<Args...>*> tracker, std::size_t id)
	: tracker_(std::move(tracker)), id_(id) { }

template<typename ...Args>
terminality::EventConnection<Args...>::EventConnection(EventConnection<Args...>&& other) noexcept
	: tracker_(std::move(other.tracker_)), id_(other.id_)
{
	other.id_ = 0;
}

template<typename ...Args>
terminality::EventConnection<Args...>& terminality::EventConnection<Args...>::operator=(EventConnection<Args...>&& other) noexcept
{
	if (this != &other)
	{
		Disconnect();

		tracker_ = std::move(other.tracker_);
		id_ = other.id_;
		other.id_ = 0;
	}

	return *this;
}

template<typename... Args>
terminality::EventConnection<Args...>::~EventConnection()
{
	Disconnect();
}

template<typename... Args>
void terminality::EventConnection<Args...>::Disconnect()
{
	if (auto locked = tracker_.lock())
	{
		if (Event<Args...>* owner = *locked)
			owner->Disconnect(id_);
	}

	tracker_.reset();
}

template<typename ...Args>
terminality::Event<Args...>::Event()
	: selfToken_(std::make_shared<Event<Args...>*>(this)) { }

template<typename ...Args>
terminality::Event<Args...>::~Event()
{
	if (selfToken_)
	{
		*selfToken_ = nullptr;
	}
}

template<typename ...Args>
terminality::Event<Args...>::Event(Event&& other) noexcept
	: handlers_(std::move(other.handlers_)), nextId_(other.nextId_), selfToken_(std::move(other.selfToken_))
{
	if (selfToken_)
	{
		*selfToken_ = this;
	}
}

template<typename ...Args>
terminality::Event<Args...>& terminality::Event<Args...>::operator=(Event&& other) noexcept
{
	if (this != &other)
	{
		if (selfToken_)
			*selfToken_ = nullptr;

		handlers_ = std::move(other.handlers_);
		nextId_ = other.nextId_;
		selfToken_ = std::move(other.selfToken_);

		if (selfToken_)
			*selfToken_ = this;
	}

	return *this;
}

template<typename ...Args>
void terminality::Event<Args...>::operator+=(terminality::Handler<Args...> handler)
{
	const std::size_t id = nextId_++;
	handlers_.emplace(id, std::move(handler));
}

template<typename... Args>
terminality::EventConnection<Args...> terminality::Event<Args...>::Connect(terminality::Handler<Args...> handler)
{
	const std::size_t id = nextId_++;
	handlers_.emplace(id, std::move(handler));

	return EventConnection<Args...>(selfToken_, id);
}

template<typename... Args>
void terminality::Event<Args...>::Emit(Args... args)
{
	const std::unordered_map<std::size_t, Handler<Args...>> snapshot = handlers_;
	for (const auto& entry : snapshot)
	{
		entry.second(args...);
	}
}

template<typename... Args>
void terminality::Event<Args...>::Disconnect(std::size_t id)
{
	handlers_.erase(id);
}
// --- End Header: terminality/Framework/Event.hpp ---

// --- Begin Header: terminality/Engine/FocusManager.hpp ---

// #include <vector> (Moved to top)

// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Framework/VisualTreeNode.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)

namespace terminality
{
	class FocusManager
	{
		std::vector<VisualTreeNode*> focusStack;

	public:
		FocusManager() = default;

		Event<VisualTreeNode*, VisualTreeNode*> FocusChanged;

		static FocusManager& Current();

		VisualTreeNode* GetFocused() const;
		bool SetFocused(VisualTreeNode* node);
		bool MoveNext(Direction direction, InputModifier modifiers = InputModifier::None);
		void ClearFocus(VisualTreeNode* node);
	};
}
// --- End Header: terminality/Engine/FocusManager.hpp ---

// --- Begin Header: terminality/Framework/VisualTree.hpp ---

// #include <cstdint> (Moved to top)
// #include <functional> (Moved to top)
// #include <memory> (Moved to top)
// #include <atomic> (Moved to top)
// #include <vector> (Moved to top)

// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/VisualTreeNode.hpp" (Merged)
// #include "terminality/Engine/RenderBuffer.hpp" (Merged)
// #include "terminality/Engine/FocusManager.hpp" (Merged)

namespace terminality
{
	struct UILayer
	{
		std::unique_ptr<VisualTreeNode> RootNode;
		std::atomic<bool> Running { false };
		FocusManager Focus;

		UILayer() = delete;

		UILayer(std::unique_ptr<VisualTreeNode> rootNode) : RootNode(std::move(rootNode))
		{
			RootNode->SetLayer(this);
		}

		UILayer(const UILayer&) = delete;
		UILayer& operator=(const UILayer&) = delete;
		UILayer(UILayer&&) = delete;
		UILayer& operator=(UILayer&&) = delete;
	};

	class VisualTree
	{
		std::vector<std::unique_ptr<UILayer>> layers_;
		bool hasDirtyVisual_ = true;
		Rect dirtyRect_;

		VisualTree();
		VisualTree(const VisualTree&) = delete;
		VisualTree& operator=(const VisualTree&) = delete;

		void CollectDirtyNodeRect(const VisualTreeNode& node);

	public:
		static VisualTree& Current();

		std::size_t LayerCount();
		VisualTreeNode* Root() const;
		VisualTreeNode* PeekLayer() const;
		UILayer& PushLayer(std::unique_ptr<VisualTreeNode> layerRoot);
		void PopLayer();

		FocusManager& GetFocusManager();

		void Invalidate(const Rect& dirtyRect);
		bool HasDirtyVisual() const { return hasDirtyVisual_; }

		void RunLayout(const Size& viewportSize);
		void Render(RenderBuffer& buffer);
	};
}
// --- End Header: terminality/Framework/VisualTree.hpp ---

// --- Begin Header: terminality/Engine/Navigator.hpp ---

// #include <memory> (Moved to top)

// #include "terminality/Framework/VisualTree.hpp" (Merged)
// #include "terminality/Framework/VisualTreeNode.hpp" (Merged)

namespace terminality
{
	class Navigator
	{
		Navigator() = default;

	public:
		static Navigator& Current();

		void Navigate(std::unique_ptr<VisualTreeNode> page);

		bool CanGoBack() const;
		bool GoBack();
		void GoHome();
	};
}
// --- End Header: terminality/Engine/Navigator.hpp ---

// --- Begin Header: terminality/Dialogs/ContextMenu.hpp ---

// #include <string> (Moved to top)
// #include <functional> (Moved to top)

// #include "terminality/Core/Geometry.hpp" (Merged)

namespace terminality
{
	struct ContextMenuItem
	{
		std::wstring Text;
		std::function<void()> Action;
	};

	class ContextMenu
	{
		std::vector<ContextMenuItem> items_;

	public:
		ContextMenu() = default;

		void AddItem(const std::wstring& text, std::function<void()> action);
		void Clear();

		void Open(Point position);
	};
}
// --- End Header: terminality/Dialogs/ContextMenu.hpp ---

// --- Begin Header: terminality/Framework/Property.hpp ---

// #include <cstdint> (Moved to top)
// #include <functional> (Moved to top)

// #include "terminality/Framework/Event.hpp" (Merged)

namespace terminality
{
	enum class InvalidationKind
	{
		None = 0,
		Visual = 1,
		Arrange = 2,
		Measure = 4
	};

	template<typename TOwner, typename T>
	class Property
	{
		TOwner* owner_;
		const char* name_;
		T value_;
		InvalidationKind invalidation_;

	public:
		Property(TOwner* owner, const char* name, T defaultValue = T(), InvalidationKind invalidation = InvalidationKind::None);

		Property& operator=(const T& value);
		Property& operator=(T&& value);

		operator const T&() const;
		const T* operator->() const;

		const T& Get() const;
		TOwner& Set(T&& value);

		bool operator==(const T& other) const;
		bool operator!=(const T& other) const;
	};
}

template<typename TOwner, typename T>
terminality::Property<TOwner, T>::Property(TOwner* owner, const char* name, T defaultValue, InvalidationKind invalidation)
	: owner_(owner), name_(name), value_(std::move(defaultValue)), invalidation_(invalidation) { }

template<typename TOwner, typename T>
terminality::Property<TOwner, T>& terminality::Property<TOwner, T>::operator=(const T& value)
{
	if (value_ == value)
		return *this;

	value_ = value;
	if (owner_)
	{
		owner_->ApplyInvalidation(invalidation_);
		owner_->OnPropertyChanged(name_);
	}

	return *this;
}

template<typename TOwner, typename T>
terminality::Property<TOwner, T>& terminality::Property<TOwner, T>::operator=(T&& value)
{
	if (value_ == value)
		return *this;

	value_ = std::move(value);
	if (owner_ != nullptr)
	{
		owner_->ApplyInvalidation(invalidation_);
		owner_->OnPropertyChanged(name_);
	}

	return *this;
}

template<typename TOwner, typename T>
terminality::Property<TOwner, T>::operator const T& () const
{
	return value_;
}

template<typename TOwner, typename T>
const T& terminality::Property<TOwner, T>::Get() const
{
	return value_;
}

template<typename TOwner, typename T>
TOwner& terminality::Property<TOwner, T>::Set(T&& value)
{
	if (value_ == value)
		return *owner_;

	value_ = std::move(value);
	if (owner_ != nullptr)
	{
		owner_->ApplyInvalidation(invalidation_);
		owner_->OnPropertyChanged(name_);
	}

	return *owner_;
}

template<typename TOwner, typename T>
const T* terminality::Property<TOwner, T>::operator->() const
{
	return &value_;
}

template<typename TOwner, typename T>
bool terminality::Property<TOwner, T>::operator==(const T& other) const
{
	return value_ == other;
}

template<typename TOwner, typename T>
bool terminality::Property<TOwner, T>::operator!=(const T& other) const
{
	return value_ != other;
}
// --- End Header: terminality/Framework/Property.hpp ---

// --- Begin Header: terminality/Framework/ControlBase.hpp ---

// #include <memory> (Moved to top)
// #include <functional> (Moved to top)
// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)
// #include "terminality/Framework/VisualTreeNode.hpp" (Merged)
// #include "terminality/Dialogs/ContextMenu.hpp" (Merged)

namespace terminality
{
	class ControlBase;

	typedef std::function<bool(const ControlBase*)> ControlPredicate;
	typedef std::function<void(ControlBase*)> HotkeyCallback;

	class ChildIterator
	{
		const VisualTreeNode* node_;
		std::size_t index_;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = VisualTreeNode*;
		using difference_type = std::ptrdiff_t;
		using pointer = VisualTreeNode**;
		using reference = VisualTreeNode*&;

		ChildIterator(const VisualTreeNode* node, std::size_t index)
			: node_(node), index_(index) { }

		VisualTreeNode* operator*() const
		{
			return node_->GetVisualChild(index_);
		}

		ChildIterator& operator++()
		{
			index_++;
			return *this;
		}

		ChildIterator operator++(int)
		{
			ChildIterator temp = *this;
			index_++;
			return temp;
		}

		bool operator==(const ChildIterator& other) const
		{
			return node_ == other.node_ && index_ == other.index_;
		}

		bool operator!=(const ChildIterator& other) const
		{
			return !(*this == other);
		}
	};

	class ControlBase : public VisualTreeNode
	{
		std::unordered_map<InputEvent, HotkeyCallback, InputEventHasher> hotkeys_;

	public:
		Event<const char*> PropertyChanged;
		Event<InputEvent> KeyDown;
		Event<InputEvent> KeyUp;

		Property<ControlBase, std::string> Tag { this, "Tag", "", InvalidationKind::None };

		Property<ControlBase, Size> MinSize { this, "MinSize", Size::Auto, InvalidationKind::Measure };
		Property<ControlBase, Size> MaxSize { this, "MaxSize", Size::Auto, InvalidationKind::Measure };
		Property<ControlBase, Size> ExpSize { this, "ExpSize", Size::Auto, InvalidationKind::Measure };

		Property<ControlBase, Thickness> Margin                    { this, "Margin", Thickness::Zero, InvalidationKind::Measure };
		Property<ControlBase, HorizontalAlign> HorizontalAlignment { this, "HorizontalAlign", HorizontalAlign::Stretch, InvalidationKind::Measure };
		Property<ControlBase, VerticalAlign> VerticalAlignment     { this, "VerticalAlign", VerticalAlign::Stretch, InvalidationKind::Measure };

		Property<ControlBase, Color> ForegroundColor	    { this, "ForegroundColor", Color::WHITE, InvalidationKind::Visual };
		Property<ControlBase, Color> BackgroundColor	    { this, "BackgroundColor", Color::BLACK, InvalidationKind::Visual };
		Property<ControlBase, Color> FocusedForegroundColor { this, "FocusedForegroundColor", Color::BLACK, InvalidationKind::Visual };
		Property<ControlBase, Color> FocusedBackgroundColor { this, "FocusedBackgroundColor", Color::WHITE, InvalidationKind::Visual };

		Property<ControlBase, bool> IsVisible                       { this, "IsVisible", true, InvalidationKind::Visual };
		Property<ControlBase, std::unique_ptr<ContextMenu>> CtxMenu { this, "ContextMenu", nullptr, InvalidationKind::None };

		// Setters
		void SetParent(VisualTreeNode* parent) override;
		void SetLayer(UILayer* layer) override;

		void SetFocusable(bool value) override;
		void SetTabStop(bool value) override;
		void SetTabIndex(int value) override;

		// Layout
		Size Measure(const Size& availableSize) override;
		void Arrange(const Rect& finalRect) override;
		void Render(RenderContext& context) override;

		// Depends
		void ApplyInvalidation(InvalidationKind invalidation);
		virtual void OnPropertyChanged(const char* propertyName);

		// User input
		bool OnKeyDown(InputEvent input) override;
		bool OnKeyUp(InputEvent input) override;
		void OnHotkey(InputModifier modifier, InputKey key, HotkeyCallback callback);

		// Ownership
		void OpenContextMenu();

		static void ResetHotkeyExecutionState();

		virtual std::size_t VisualChildrenCount() const override;
		virtual VisualTreeNode* GetVisualChild(std::size_t index) const override;

		const ChildIterator child_begin() const;
		const ChildIterator child_end() const;

		// Navigation
		void Close();

		template<typename T = ControlBase>
		T* QueryByTag(std::string_view tag)
		{
			if (Tag.Get() == tag)
				return dynamic_cast<T*>(this);

			for (std::size_t i = 0; i < VisualChildrenCount(); ++i)
			{
				VisualTreeNode* childNode = GetVisualChild(i);
				if (auto* childControl = dynamic_cast<ControlBase*>(childNode))
				{
					if (auto* result = childControl->QueryByTag<T>(tag))
						return result;
				}
			}

			return nullptr;
		}

	protected:
		// Layout
		virtual Size MeasureOverride(const Size& availableSize) = 0;
		virtual void ArrangeOverride(const Rect& finalRect) = 0;
		virtual void RenderOverride(RenderContext& context) = 0;
	};

	template <typename T>
	std::unique_ptr<T> init(std::function<void(T*)> init)
	{
		std::unique_ptr<T> widget = std::make_unique<T>();
		if (init != nullptr)
			init(widget.get());

		return std::move(widget);
	}

	template <typename T>
	std::unique_ptr<T> init()
	{
		std::unique_ptr<T> widget = std::make_unique<T>();
		return std::move(widget);
	}
}
// --- End Header: terminality/Framework/ControlBase.hpp ---

// --- Begin Header: terminality/Controls/Layout/Grid.hpp ---

// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)
// #include <vector> (Moved to top)
// #include <string> (Moved to top)
// #include <string_view> (Moved to top)

// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)

namespace terminality
{
    enum class GridUnitType
    {
        Auto,
        Cell,
        Star
    };

    struct GridLength
    {
        float Value;
        GridUnitType Type;

        GridLength(float value = 1.0f, GridUnitType type = GridUnitType::Star)
            : Value(value), Type(type) { }

        static GridLength Auto();
        static GridLength Cell(int32_t cells);
        static GridLength Star(float weight = 1.0f);
    };

    struct RowDefinition
    {
        GridLength Height = GridLength::Star();
        int32_t MinHeight = 0;
        int32_t MaxHeight = -1;

        int32_t ActualHeight = 0;
        int32_t OffsetY = 0;
    };

    struct ColumnDefinition
    {
        GridLength Width = GridLength::Star();
        int32_t MinWidth = 0;
        int32_t MaxWidth = -1;

        int32_t ActualWidth = 0;
        int32_t OffsetX = 0;
    };

    class Grid : public ControlBase
    {
        struct GridChild
        {
            std::unique_ptr<ControlBase> Control;
            int32_t Row;
            int32_t Column;
            int32_t RowSpan;
            int32_t ColumnSpan;
        };

        std::vector<RowDefinition> rowDefs_;
        std::vector<ColumnDefinition> colDefs_;
        std::vector<GridChild> children_;
        std::size_t focusedIndex_ = 0;

        void EnsureGridDefinitions();

    public:
        Grid() = default;

        void SetRowDefinitions(std::string_view definitions);
        void SetColumnDefinitions(std::string_view definitions);
        void AddRow(const RowDefinition& def);
        void AddColumn(const ColumnDefinition& def);
        void AddChild(int32_t row, int32_t column, int32_t rowSpan, int32_t colSpan, std::unique_ptr<ControlBase> child);
        void AddChild(int32_t row, int32_t column, std::unique_ptr<ControlBase> child);

    protected:
        bool MoveFocusNext(Direction direction, InputModifier modifiers) override;
        void OnGotFocus() override;
        void OnLostFocus() override;

        Size MeasureOverride(const Size& availableSize) override;
        void ArrangeOverride(const Rect& contentRect) override;
        void RenderOverride(RenderContext& context) override;

        std::size_t VisualChildrenCount() const override;
        VisualTreeNode* GetVisualChild(std::size_t index) const override;
    };
}
// --- End Header: terminality/Controls/Layout/Grid.hpp ---

// --- Begin Header: terminality/Controls/Layout/StackPanel.hpp ---

// #include <cstdint> (Moved to top)
// #include <vector> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)

namespace terminality
{
	class StackPanel : public ControlBase
	{
	protected:
		std::vector<std::unique_ptr<ControlBase>> contents_;
		std::size_t focusedIndex_ = 0;
		int32_t scrollOffset_ = 0;
		bool forceScrollToEnd_ = false;

	public:
		Property<StackPanel, Orientation> ContentOrientation						  { this, "ContentOrientation", Orientation::Vertical, InvalidationKind::Measure };
		Property<StackPanel, terminality::HorizontalAlign> HorizontalContentAlignment { this, "HorizontalContentAlignment", HorizontalAlign::Stretch, InvalidationKind::Measure };
		Property<StackPanel, terminality::VerticalAlign> VerticalContentAlignment	  { this, "VerticalContentAlignment", VerticalAlign::Stretch, InvalidationKind::Measure };
		Property<StackPanel, bool> Looping											  { this, "Looping", false, InvalidationKind::None };
		Property<StackPanel, bool> Scrollable                                         { this, "Scrollable", false, InvalidationKind::Arrange };
		Property<StackPanel, bool> AutoScrollToEnd                                    { this, "AutoScrollToEnd", false, InvalidationKind::None };

		StackPanel() = default;

		void AddChild(std::unique_ptr<ControlBase> child);
		void Insert(std::size_t index, std::unique_ptr<ControlBase> child);
		std::unique_ptr<ControlBase> RemoveChild(ControlPredicate predicate);
		std::unique_ptr<ControlBase> RemoveAt(std::size_t index);
		void Clear();

		void OnPropertyChanged(const char* propertyName) override;

		bool MoveFocusNext(Direction direction, InputModifier modifiers) override;
		void OnGotFocus() override;
		void OnLostFocus() override;

		std::size_t VisualChildrenCount() const override;
		VisualTreeNode* GetVisualChild(std::size_t index) const override;

	protected:
		Size MeasureOverride(const Size& availableSize) override;
		void ArrangeOverride(const Rect& contentRect) override;
		void RenderOverride(RenderContext& context) override;

		void ScrollIntoView();
	};
}
// --- End Header: terminality/Controls/Layout/StackPanel.hpp ---

// --- Begin Header: terminality/Core/TextHelper.hpp ---

// #include <cstdint> (Moved to top)
// #include <vector> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Core/Layout.hpp" (Merged)

namespace terminality
{
    struct LineBounds
    {
        std::size_t Start;
        std::size_t End;
        std::size_t NextStart;
    };

	struct LineInfo
	{
		std::wstring Text;
		std::size_t StartIndex;
	};

	class TextHelper
	{
    public:
        static std::vector<LineBounds> CalculateLineBounds(const std::wstring& text, int32_t availableWidth, TextWrap wrapping);
        static std::vector<LineInfo> GetLines(const std::wstring& text, int32_t availableWidth, TextWrap wrapping);
        static std::vector<int32_t> MeasureLines(const std::wstring& text, int32_t availableWidth, TextWrap wrapping);
	};
}
// --- End Header: terminality/Core/TextHelper.hpp ---

// --- Begin Header: terminality/Controls/Interactable/TextBox.hpp ---

// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Core/Focus.hpp" (Merged)

namespace terminality
{
	class TextBox : public ControlBase
	{
		std::size_t cursorPosition_ = 0;

	public:
		Property<TextBox, std::wstring> Text	   { this, "Text", L"", InvalidationKind::Measure };
		Property<TextBox, TextWrap> TextWrapping   { this, "TextWrapping", terminality::TextWrap::NoWrap, InvalidationKind::Measure };
		Property<TextBox, TextAlign> TextAlignment { this, "TextAlignment", terminality::TextAlign::Left, InvalidationKind::Visual };
		Property<TextBox, bool> AcceptsReturn	   { this, "AcceptsReturn", false, InvalidationKind::Measure };

		Event<> TextChanged;

		TextBox();

		void OnPropertyChanged(const char* propertyName) override;

		bool OnKeyDown(InputEvent input) override;
		bool OnKeyUp(InputEvent input) override;

		bool MoveFocusNext(Direction direction, InputModifier modifiers = InputModifier::None) override;
		void OnGotFocus() override;
		void OnLostFocus() override;

	protected:
		Size MeasureOverride(const Size& availableSize) override;
		void ArrangeOverride(const Rect& contentRect) override;
		void RenderOverride(RenderContext& context) override;
	};
}
// --- End Header: terminality/Controls/Interactable/TextBox.hpp ---

// --- Begin Header: terminality/Controls/Interactable/CheckBox.hpp ---

// #include <optional> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)

namespace terminality
{
	class CheckBox : public ControlBase
	{
		bool isPressed_ = false;
		std::optional<bool> isChecked_ = false;

	public:
		Property<CheckBox, std::wstring> Text{ this, "Text", L"", InvalidationKind::Measure };
		Property<CheckBox, Color> PressedForegroundColor{ this, "PressedForegroundColor", Color::BLACK, InvalidationKind::Visual };
		Property<CheckBox, Color> PressedBackgroundColor{ this, "PressedBackgroundColor", Color::CYAN, InvalidationKind::Visual };

		Event<std::optional<bool>> Toggled;
		Event<> Checked;
		Event<> Unchecked;

		void Toggle(std::optional<bool> value);

		void OnPropertyChanged(const char* propertyName) override;

		void OnLostFocus() override;

		bool OnKeyDown(InputEvent input) override;
		bool OnKeyUp(InputEvent input) override;

	protected:
		Size MeasureOverride(const Size& availableSize) override;
		void ArrangeOverride(const Rect& contentRect) override;
		void RenderOverride(RenderContext& context) override;
	};
}
// --- End Header: terminality/Controls/Interactable/CheckBox.hpp ---

// --- Begin Header: terminality/Engine/DispatchTimer.hpp ---

// #include <cstdint> (Moved to top)
// #include <atomic> (Moved to top)
// #include <chrono> (Moved to top)
// #include <thread> (Moved to top)
// #include <mutex> (Moved to top)
// #include <functional> (Moved to top)

// #include "terminality/Framework/Event.hpp" (Merged)

namespace terminality
{
	class DispatchTimer
	{
		std::optional<std::thread::id> uiThreadId_;
		std::mutex mutex_;
		std::vector<std::function<void()>> tasks_;

		float deltaTime_ = 0.0f;
		float totalTime_ = 0.0f;
		std::atomic<bool> running_ = false;

		// ticking
		std::chrono::time_point<std::chrono::high_resolution_clock> lastTime_;
		std::chrono::time_point<std::chrono::high_resolution_clock> frameStart_;

		// debouncing
		bool isResizing_ = false;
		float resizeDebounceTimer_ = 0.0f;
		const float RESIZE_DELAY = 0.1f;

		DispatchTimer() = default;
		DispatchTimer(const DispatchTimer&) = delete;
		DispatchTimer& operator=(const DispatchTimer&) = delete;

	public:
		Event<float> TickEvent;
		Event<> ResizeFinishedEvent;

		static DispatchTimer& Current();

		void SetUIThread();
		bool CheckAccess() const;
		void VerifyAccess() const;

		void InvokeAsync(std::function<void()> task);
		void ProcessTasks();

		bool IsRunning() const;
		bool IsResizing() const;

		float DeltaTime() const { return deltaTime_; }
		float TotalTime() const { return totalTime_; }

		void Start();
		void Stop();

		void Tick();
		void BeginResize();

		std::chrono::milliseconds GetRemainingFrameTime(int targetFPS = 60);
	};
}
// --- End Header: terminality/Engine/DispatchTimer.hpp ---

// --- Begin Header: terminality/Framework/Collections/ObservableCollection.hpp ---

// #include <cstdint> (Moved to top)
// #include <vector> (Moved to top)
// #include <stdexcept> (Moved to top)

// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Engine/DispatchTimer.hpp" (Merged)

namespace terminality
{
	template<typename T>
	class ObservableCollection
	{
		std::vector<T> items_;

	public:
		Event<std::size_t, const T&> ItemAdded;
		Event<std::size_t, const T&> ItemRemoved;
		Event<std::size_t, const T&, const T&> ItemReplaced;
		Event<> CollectionCleared;

		ObservableCollection() = default;
		~ObservableCollection() = default;

		// Iterators
		auto begin() { return items_.begin(); }
		auto end() { return items_.end(); }
		auto begin() const { return items_.begin(); }
		auto end() const { return items_.end(); }
		auto cbegin() const { return items_.cbegin(); }
		auto cend() const { return items_.cend(); }

		// Capacity
		std::size_t size() const
		{
			return items_.size();
		}

		bool empty() const
		{
			return items_.empty();
		}

		// Element access
		T& operator[](std::size_t index)
		{
			return items_[index];
		}

		const T& operator[](std::size_t index) const
		{
			return items_[index];
		}

		T& at(std::size_t index)
		{
			return items_.at(index);
		}

		const T& at(std::size_t index) const
		{
			return items_.at(index);
		}

		// Modifiers
		void push_back(const T& item)
		{
			DispatchTimer::Current().VerifyAccess();
			items_.push_back(item);
			ItemAdded.Emit(items_.size() - 1, item);
		}

		void push_back(T&& item)
		{
			DispatchTimer::Current().VerifyAccess();
			items_.push_back(std::move(item));
			ItemAdded.Emit(items_.size() - 1, items_.back());
		}

		void pop_back()
		{
			DispatchTimer::Current().VerifyAccess();
			if (items_.empty())
				return;

			T removedItem = std::move(items_.back());
			std::size_t index = items_.size() - 1;
			items_.pop_back();
			ItemRemoved.Emit(index, removedItem);
		}

		void insert(std::size_t index, const T& item)
		{
			DispatchTimer::Current().VerifyAccess();
			if (index > items_.size())
				throw std::out_of_range("Index out of range");

			items_.insert(items_.begin() + index, item);
			ItemAdded.Emit(index, item);
		}

		void insert(std::size_t index, T&& item)
		{
			DispatchTimer::Current().VerifyAccess();
			if (index > items_.size())
				throw std::out_of_range("Index out of range");

			items_.insert(items_.begin() + index, std::move(item));
			ItemAdded.Emit(index, items_[index]);
		}

		void erase(std::size_t index)
		{
			DispatchTimer::Current().VerifyAccess();
			if (index >= items_.size())
				throw std::out_of_range("Index out of range");

			T removedItem = std::move(items_[index]);
			items_.erase(items_.begin() + index);
			ItemRemoved.Emit(index, removedItem);
		}

		void replace(std::size_t index, const T& item)
		{
			DispatchTimer::Current().VerifyAccess();
			if (index >= items_.size())
				throw std::out_of_range("Index out of range");

			T oldItem = std::move(items_[index]);
			items_[index] = item;
			ItemReplaced.Emit(index, oldItem, item);
		}

		void replace(std::size_t index, T&& item)
		{
			DispatchTimer::Current().VerifyAccess();
			if (index >= items_.size())
				throw std::out_of_range("Index out of range");

			T oldItem = std::move(items_[index]);
			items_[index] = std::move(item);
			ItemReplaced.Emit(index, oldItem, items_[index]);
		}

		void clear()
		{
			DispatchTimer::Current().VerifyAccess();
			if (items_.empty())
				return;

			items_.clear();
			CollectionCleared.Emit();
		}
	};
}
// --- End Header: terminality/Framework/Collections/ObservableCollection.hpp ---

// --- Begin Header: terminality/Controls/Layout/ItemsControl.hpp ---

// #include <cstdint> (Moved to top)
// #include <optional> (Moved to top)

// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Controls/Layout/StackPanel.hpp" (Merged)
// #include "terminality/Framework/Collections/ObservableCollection.hpp" (Merged)
// #include "terminality/Engine/FocusManager.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)

namespace terminality
{
	template<typename T>
	class ItemsControl : public StackPanel
	{
	public:
		using ItemTemplate = std::function<std::unique_ptr<ControlBase>(const T&)>;

	private:
		ObservableCollection<T>* itemsSource_ = nullptr;
		ItemTemplate itemTemplate_;

		std::optional<EventConnection<std::size_t, const T&>> addedConnection_;
		std::optional<EventConnection<std::size_t, const T&>> removedConnection_;
		std::optional<EventConnection<std::size_t, const T&, const T&>> replacedConnection_;
		std::optional<EventConnection<>> clearedConnection_;

		void RebuildItems();
		void OnItemAdded(std::size_t index, const T& item);
		void OnItemRemoved(std::size_t index, const T& item);
		void OnItemReplaced(std::size_t index, const T& oldItem, const T& newItem);
		void OnCollectionCleared();

	public:
		ItemsControl() = default;
		virtual ~ItemsControl() = default;

		bool MoveFocusNext(Direction direction, InputModifier modifiers) override;

		void SetItemTemplate(ItemTemplate itemTemplate);
		void SetItemsSource(ObservableCollection<T>* itemsSource);

		ObservableCollection<T>* GetItemsSource() const;
	};
}

template<typename T>
bool terminality::ItemsControl<T>::MoveFocusNext(Direction direction, InputModifier modifiers)
{
	if (modifiers == InputModifier::Special)
	{
		focusedIndex_ = 0;
		return true;
	}

	return StackPanel::MoveFocusNext(direction, modifiers);
}

template<typename T>
void terminality::ItemsControl<T>::RebuildItems()
{
	this->Clear();
	if (itemsSource_ && itemTemplate_)
	{
		for (const auto& item : *itemsSource_)
		{
			this->AddChild(itemTemplate_(item));
		}
	}
}

template<typename T>
void terminality::ItemsControl<T>::OnItemAdded(std::size_t index, const T& item)
{
	if (itemTemplate_)
	{
		this->Insert(index, itemTemplate_(item));
	}
}

template<typename T>
void terminality::ItemsControl<T>::OnItemRemoved(std::size_t index, const T& item)
{
	this->RemoveAt(index);
}

template<typename T>
void terminality::ItemsControl<T>::OnItemReplaced(std::size_t index, const T& oldItem, const T& newItem)
{
	if (itemTemplate_)
	{
		this->RemoveAt(index);
		this->Insert(index, itemTemplate_(newItem));
	}
}

template<typename T>
void terminality::ItemsControl<T>::OnCollectionCleared()
{
	FocusManager::Current().MoveNext(Direction::Previous, InputModifier::Special);
	this->Clear();
}

template<typename T>
void terminality::ItemsControl<T>::SetItemTemplate(ItemTemplate itemTemplate)
{
	itemTemplate_ = std::move(itemTemplate);
	RebuildItems();
}

template<typename T>
void terminality::ItemsControl<T>::SetItemsSource(terminality::ObservableCollection<T>* itemsSource)
{
	if (itemsSource_ == itemsSource)
		return;

	if (itemsSource_)
	{
		addedConnection_.reset();
		removedConnection_.reset();
		replacedConnection_.reset();
		clearedConnection_.reset();
	}

	itemsSource_ = itemsSource;

	if (itemsSource_)
	{
		addedConnection_.emplace(itemsSource_->ItemAdded.Connect(
			[this](std::size_t index, const T& item) { OnItemAdded(index, item); }));

		removedConnection_.emplace(itemsSource_->ItemRemoved.Connect(
			[this](std::size_t index, const T& item) { OnItemRemoved(index, item); }));

		replacedConnection_.emplace(itemsSource_->ItemReplaced.Connect(
			[this](std::size_t index, const T& oldItem, const T& newItem) { OnItemReplaced(index, oldItem, newItem); }));

		clearedConnection_.emplace(itemsSource_->CollectionCleared.Connect(
			[this]() { OnCollectionCleared(); }));
	}

	RebuildItems();
}

template<typename T>
terminality::ObservableCollection<T>* terminality::ItemsControl<T>::GetItemsSource() const
{
	return itemsSource_;
}
// --- End Header: terminality/Controls/Layout/ItemsControl.hpp ---

// --- Begin Header: terminality/Controls/Layout/TabControl.hpp ---

// #include <vector> (Moved to top)
// #include <string> (Moved to top)
// #include <memory> (Moved to top)
// #include <cstdint> (Moved to top)

// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)

namespace terminality
{
    class TabItem
    {
    public:
        std::string Header;
        std::unique_ptr<ControlBase> Content;

        TabItem(std::string header, std::unique_ptr<ControlBase> content)
            : Header(std::move(header)), Content(std::move(content)) {}
    };

    class TabControl : public ControlBase
    {
    protected:
        std::vector<TabItem> tabs_;
        int32_t headerScrollOffset_ = 0;

    public:
        Property<TabControl, int> SelectedIndex { this, "SelectedIndex", 0, InvalidationKind::Arrange };

        TabControl() = default;

        void AddTab(const std::string& header, std::unique_ptr<ControlBase> content);
        void RemoveTab(int index);
        void ClearTabs();

        size_t GetTabCount() const;

        bool IsFocusable() const override { return true; }

        bool OnKeyDown(InputEvent input) override;

        std::size_t VisualChildrenCount() const override;
        VisualTreeNode* GetVisualChild(std::size_t index) const override;

        bool MoveFocusNext(Direction direction, InputModifier modifiers) override;
        void OnGotFocus() override;
        void OnLostFocus() override;
        void OnPropertyChanged(const char* propertyName) override;

    protected:
        Size MeasureOverride(const Size& availableSize) override;
        void ArrangeOverride(const Rect& finalRect) override;
        void RenderOverride(RenderContext& context) override;
    };
}
// --- End Header: terminality/Controls/Layout/TabControl.hpp ---

// --- Begin Header: terminality/Controls/Visual/ProgressBar.hpp ---
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)

namespace terminality
{
	class ProgressBar : public ControlBase
	{
	public:
		Property<ProgressBar, float> Minimum { this, "Minimum", 0.0f, InvalidationKind::Visual };
		Property<ProgressBar, float> Maximum { this, "Maximum", 100.0f, InvalidationKind::Visual };
		Property<ProgressBar, float> Value   { this, "Value", 0.0f, InvalidationKind::Visual };

		Property<ProgressBar, Color> BarColor   { this, "BarColor", Color::GREEN, InvalidationKind::Visual };
		Property<ProgressBar, Color> TrackColor { this, "TrackColor", Color::DARK_GRAY, InvalidationKind::Visual };

		ProgressBar() = default;

	protected:
		Size MeasureOverride(const Size& availableSize) override;
		void ArrangeOverride(const Rect& contentRect) override;
		void RenderOverride(RenderContext& context) override;
	};
}
// --- End Header: terminality/Controls/Visual/ProgressBar.hpp ---

// --- Begin Header: terminality/Controls/Layout/ScrollViewer.hpp ---

// #include <memory> (Moved to top)
// #include <algorithm> (Moved to top)

// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)

namespace terminality
{
    class ScrollViewer : public ControlBase
    {
    public:
        std::unique_ptr<ControlBase> Content;

        Property<ScrollViewer, int> ScrollX{ this, "ScrollX", 0, InvalidationKind::Arrange };
        Property<ScrollViewer, int> ScrollY{ this, "ScrollY", 0, InvalidationKind::Arrange };

        ScrollViewer() = default;

        bool IsFocusable() const override
        {
            return true;
        }

        Size MeasureOverride(const Size& availableSize) override;
        void ArrangeOverride(const Rect& finalRect) override;
        void RenderOverride(RenderContext& context) override;

        bool OnKeyDown(InputEvent input) override;

        int GetExtentWidth() const;
        int GetExtentHeight() const;
        int GetViewportWidth() const;
        int GetViewportHeight() const;
    };
}
// --- End Header: terminality/Controls/Layout/ScrollViewer.hpp ---

// --- Begin Header: terminality/Framework/PlatformSpecific/Windows.hpp ---

// #include <string> (Moved to top)

#ifdef _WIN32
namespace terminality
{
    void AlertAsync(const std::wstring& text, const std::wstring& title);
}

#endif // _WIN32
// --- End Header: terminality/Framework/PlatformSpecific/Windows.hpp ---

// --- Begin Header: terminality/Controls/Visual/Border.hpp ---

// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)

namespace terminality
{
	enum class RectanglePos
	{
		LeftTopCorner,
		LeftBottomCorner,

		RightTopCorner,
		RightBottomCorner,

		LeftVerticalLine,
		RightVerticalLine,

		TopHorizontalLine,
		BottomHorizontalLine
	};

	typedef wchar_t (*BorderStyle)(const RectanglePos pos);

	class Border : public ControlBase
	{
	public:
		Property<Border, Color> BorderColor					   { this, "BorderColor", Color::DARK_GRAY, InvalidationKind::Visual};
		Property<Border, Color> FocusedBorderColor             { this, "FocusedBorderColor", Color::CYAN, InvalidationKind::Visual };
		Property<Border, Thickness> BorderThickness			   { this, "BorderThickness", Thickness::Single, InvalidationKind::Visual };
		Property<Border, std::wstring> HeaderText			   { this, "HeaderText", L"", InvalidationKind::Visual };
		Property<Border, std::unique_ptr<ControlBase>> Content { this, "Content", nullptr, InvalidationKind::Visual };
		Property<Border, BorderStyle> Style				       { this, "BorderStyle", nullptr, InvalidationKind::Visual };

		Border();
		Border(std::unique_ptr<ControlBase> content);

		void OnPropertyChanged(const char* propertyName) override;

		bool MoveFocusNext(Direction direction, InputModifier modifiers = InputModifier::None) override;
		void OnGotFocus() override;
		void OnLostFocus() override;

		Size MeasureOverride(const Size& availableSize) override;
		void ArrangeOverride(const Rect& contentRect) override;
		void RenderOverride(RenderContext& context) override;

		std::size_t VisualChildrenCount() const override;
		VisualTreeNode* GetVisualChild(std::size_t index) const override;
	};
}
// --- End Header: terminality/Controls/Visual/Border.hpp ---

// --- Begin Header: terminality/Dialogs/OpenFileDialog.hpp ---

// #include <string> (Moved to top)
// #include <filesystem> (Moved to top)

namespace terminality
{
	class OpenFileDialog
	{
	public:
		static std::optional<std::filesystem::path> Show(const std::wstring& title = L"Open File", const std::filesystem::path& initialDirectory = std::filesystem::current_path());
	};
}
// --- End Header: terminality/Dialogs/OpenFileDialog.hpp ---

// --- Begin Header: terminality/Framework/Collections/Queries.hpp ---

// #include <cstdint> (Moved to top)
// #include <vector> (Moved to top)
// #include <type_traits> (Moved to top)
// #include <ranges> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <stdexcept> (Moved to top)

namespace terminality
{
	template <typename Derived>
	struct QueryOperator {};

	template <std::ranges::range R, typename Derived>
	auto operator|(R&& range, const QueryOperator<Derived>& op)
	{
		return static_cast<const Derived&>(op)(std::forward<R>(range));
	}

	// Where (Filter)
	template <typename Predicate>
	struct WhereOp : QueryOperator<WhereOp<Predicate>>
	{
		Predicate pred;
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			return std::forward<R>(range) | std::views::filter(pred);
		}
	};

	template <typename Predicate>
	WhereOp<std::decay_t<Predicate>> Where(Predicate&& pred)
	{
		return { std::forward<Predicate>(pred) };
	}

	// Select (Transform)
	template <typename Selector>
	struct SelectOp : QueryOperator<SelectOp<Selector>>
	{
		Selector sel;
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			return std::forward<R>(range) | std::views::transform(sel);
		}
	};

	template <typename Selector>
	SelectOp<std::decay_t<Selector>> Select(Selector&& sel)
	{
		return { std::forward<Selector>(sel) };
	}

	// ToList
	struct ToListOp : QueryOperator<ToListOp>
	{
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			using ElementType = std::ranges::range_value_t<R>;
			std::vector<ElementType> result;
			if constexpr (std::ranges::sized_range<R>)
				result.reserve(std::ranges::size(range));

			for (auto&& item : range)
				result.push_back(std::forward<decltype(item)>(item));

			return result;
		}
	};

	inline ToListOp ToList()
	{
		return {};
	}

	// First
	template <typename Predicate>
	struct FirstPredOp : QueryOperator<FirstPredOp<Predicate>>
	{
		Predicate pred;
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			for (auto&& item : range)
			{
				if (pred(item))
					return item;
			}

			throw std::runtime_error("Sequence contains no matching element");
		}
	};

	template <typename Predicate>
	FirstPredOp<std::decay_t<Predicate>> First(Predicate&& pred)
	{
		return { std::forward<Predicate>(pred) };
	}

	struct FirstOp : QueryOperator<FirstOp>
	{
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			auto it = std::ranges::begin(range);
			if (it != std::ranges::end(range))
				return *it;

			throw std::runtime_error("Sequence contains no elements");
		}
	};

	inline FirstOp First()
	{
		return {};
	}

	// FirstOrDefault
	template <typename Predicate>
	struct FirstOrDefaultPredOp : QueryOperator<FirstOrDefaultPredOp<Predicate>>
	{
		Predicate pred;
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			using ElementType = std::ranges::range_value_t<R>;
			for (auto&& item : range)
			{
				if (pred(item))
					return std::optional<ElementType>(item);
			}

			return std::optional<ElementType>(std::nullopt);
		}
	};

	template <typename Predicate>
	FirstOrDefaultPredOp<std::decay_t<Predicate>> FirstOrDefault(Predicate&& pred)
	{
		return { std::forward<Predicate>(pred) };
	}

	struct FirstOrDefaultOp : QueryOperator<FirstOrDefaultOp>
	{
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			using ElementType = std::ranges::range_value_t<R>;
			auto it = std::ranges::begin(range);
			if (it != std::ranges::end(range))
				return std::optional<ElementType>(*it);

			return std::optional<ElementType>(std::nullopt);
		}
	};

	inline FirstOrDefaultOp FirstOrDefault()
	{
		return {};
	}

	// Any
	template <typename Predicate>
	struct AnyPredOp : QueryOperator<AnyPredOp<Predicate>>
	{
		Predicate pred;
		template <std::ranges::range R>
		bool operator()(R&& range) const
		{
			for (auto&& item : range)
			{
				if (pred(item))
					return true;
			}

			return false;
		}
	};

	template <typename Predicate>
	AnyPredOp<std::decay_t<Predicate>> Any(Predicate&& pred)
	{
		return { std::forward<Predicate>(pred) };
	}

	struct AnyOp : QueryOperator<AnyOp>
	{
		template <std::ranges::range R>
		bool operator()(R&& range) const
		{
			return !std::ranges::empty(range);
		}
	};

	inline AnyOp Any()
	{
		return {};
	}

	// All
	template <typename Predicate>
	struct AllOp : QueryOperator<AllOp<Predicate>>
	{
		Predicate pred;
		template <std::ranges::range R>
		bool operator()(R&& range) const
		{
			for (auto&& item : range)
			{
				if (!pred(item))
					return false;
			}

			return true;
		}
	};

	template <typename Predicate>
	AllOp<std::decay_t<Predicate>> All(Predicate&& pred)
	{
		return { std::forward<Predicate>(pred) };
	}

	// Count
	struct CountOp : QueryOperator<CountOp>
	{
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			return std::ranges::distance(range);
		}
	};

	inline CountOp Count()
	{
		return {};
	}

	template <typename Predicate>
	struct CountPredOp : QueryOperator<CountPredOp<Predicate>>
	{
		Predicate pred;
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			std::size_t c = 0;
			for (auto&& item : range)
			{
				if (pred(item))
					c++;
			}

			return c;
		}
	};

	template <typename Predicate>
	CountPredOp<std::decay_t<Predicate>> Count(Predicate&& pred)
	{
		return { std::forward<Predicate>(pred) };
	}

	// OrderBy
	template <typename KeySelector>
	struct OrderByOp : QueryOperator<OrderByOp<KeySelector>>
	{
		KeySelector sel;
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			using ElementType = std::ranges::range_value_t<R>;
			std::vector<ElementType> result;
			if constexpr (std::ranges::sized_range<R>)
			{
				result.reserve(std::ranges::size(range));
			}

			for (auto&& item : range)
			{
				result.push_back(item);
			}

			std::ranges::sort(result, {}, sel);
			return result;
		}
	};

	template <typename KeySelector>
	OrderByOp<std::decay_t<KeySelector>> OrderBy(KeySelector&& sel)
	{
		return { std::forward<KeySelector>(sel) };
	}

	// OrderByDescending
	template <typename KeySelector>
	struct OrderByDescendingOp : QueryOperator<OrderByDescendingOp<KeySelector>>
	{
		KeySelector sel;
		template <std::ranges::range R>
		auto operator()(R&& range) const
		{
			using ElementType = std::ranges::range_value_t<R>;
			std::vector<ElementType> result;

			if constexpr (std::ranges::sized_range<R>)
			{
				result.reserve(std::ranges::size(range));
			}

			for (auto&& item : range)
			{
				result.push_back(item);
			}

			std::ranges::sort(result, std::greater<>{}, sel);
			return result;
		}
	};

	template <typename KeySelector>
	OrderByDescendingOp<std::decay_t<KeySelector>> OrderByDescending(KeySelector&& sel)
	{
		return { std::forward<KeySelector>(sel) };
	}
}
// --- End Header: terminality/Framework/Collections/Queries.hpp ---

// --- Begin Header: terminality/Controls/Visual/Spinner.hpp ---

// #include <string> (Moved to top)
// #include <vector> (Moved to top)

// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Engine/DispatchTimer.hpp" (Merged)

namespace terminality
{
    class Spinner : public ControlBase
    {
        EventConnection<float> tickConnection_;
        float accumulator_ = 0.0f;
        int frame_ = 0;

    public:
        Property<Spinner, std::vector<std::wstring>> Frames = { this, "Frames", std::vector<std::wstring>{ L"-", L"\\", L"|", L"/" }, InvalidationKind::Visual };

        Spinner();

    protected:
        Size MeasureOverride(const Size& availableSize) override;
        void ArrangeOverride(const Rect& finalRect) override;
        void RenderOverride(RenderContext& context) override;
    };
}
// --- End Header: terminality/Controls/Visual/Spinner.hpp ---

// --- Begin Header: terminality/Controls/Visual/Label.hpp ---

// #include <string> (Moved to top)

// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)

namespace terminality
{
	class Label : public ControlBase
	{
		bool isPressed_ = false;

	public:
		Property<Label, std::wstring> Text		 { this, "Text", L"", InvalidationKind::Measure };
		Property<Label, TextWrap> TextWrapping   { this, "TextWrapping", terminality::TextWrap::NoWrap, InvalidationKind::Measure };
		Property<Label, TextAlign> TextAlignment { this, "TextAlignment", terminality::TextAlign::Left, InvalidationKind::Visual };

		Event<> TextChanged;

		Label();
		Label(std::wstring& text);

		void OnPropertyChanged(const char* propertyName) override;

	protected:
		Size MeasureOverride(const Size& availableSize) override;
		void ArrangeOverride(const Rect& contentRect) override;
		void RenderOverride(RenderContext& context) override;
	};
}
// --- End Header: terminality/Controls/Visual/Label.hpp ---

// --- Begin Header: terminality/Framework/HostApplication.hpp ---

// #include <optional> (Moved to top)
// #include <chrono> (Moved to top)
// #include <thread> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Framework/VisualTree.hpp" (Merged)
// #include "terminality/Framework/VisualTreeNode.hpp" (Merged)
// #include "terminality/Engine/RenderBuffer.hpp" (Merged)
// #include "terminality/Engine/FocusManager.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)

namespace terminality
{
	class HostBackend
	{
	public:
		static Size QueryViewportSize();
		static InputEvent PollInput(std::chrono::milliseconds timeout);
	};

	class HostApplication
	{
		bool isResizing_ = false;
		float resizeDebounceTimer_ = 0.0f;
		float RESIZE_DELAY = 0.1f;

		RenderBuffer renderBuffer_{ 1, 1 };

		HostApplication() = default;
		HostApplication(const HostApplication&) = delete;
		HostApplication& operator=(const HostApplication&) = delete;

	public:
		static HostApplication& Current();

		void EnterTerminal();
		void ExitTerminal();

		void RunUILoop(std::unique_ptr<VisualTreeNode> root);
		void NestUILoop(UILayer& layer);
		void RequestStop();
	};
}
// --- End Header: terminality/Framework/HostApplication.hpp ---

// --- Begin Header: terminality/Controls/Interactable/Button.hpp ---

// #include <string> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)

namespace terminality
{
	class Button : public ControlBase
	{
		bool isPressed_ = false;

	public:
		Property<Button, std::wstring> Text  { this, "Text", L"", InvalidationKind::Measure };
		Property<Button, Color> PressedForegroundColor { this, "PressedForegroundColor", Color::BLACK, InvalidationKind::Visual };
		Property<Button, Color> PressedBackgroundColor { this, "PressedBackgroundColor", Color::CYAN, InvalidationKind::Visual };

		Event<> Clicked;

		void Click();

		void OnPropertyChanged(const char* propertyName) override;

		void OnLostFocus() override;

		bool OnKeyDown(InputEvent input) override;
		bool OnKeyUp(InputEvent input) override;

	protected:
		Size MeasureOverride(const Size& availableSize) override;
		void ArrangeOverride(const Rect& contentRect) override;
		void RenderOverride(RenderContext& context) override;
	};
}
// --- End Header: terminality/Controls/Interactable/Button.hpp ---

// --- Begin Header: terminality/Dialogs/MessageBox.hpp ---

// #include <string> (Moved to top)

namespace terminality
{
	enum class MessageBoxButton
	{
		Ok,
		OkCancel,
		YesNo,
		YesNoCancel
	};

	enum class MessageBoxResult
	{
		None,
		Ok,
		Cancel,
		Yes,
		No
	};

	class MessageBox
	{
	public:
		static MessageBoxResult Show(const std::wstring& title, const std::wstring& message, MessageBoxButton buttons = MessageBoxButton::Ok);
	};
}
// --- End Header: terminality/Dialogs/MessageBox.hpp ---

// --- Begin Header: terminality/Terminality.hpp ---
// terminality.Core
// #include "terminality/Core/Color.hpp" (Merged)
// #include "terminality/Core/Focus.hpp" (Merged)
// #include "terminality/Core/Geometry.hpp" (Merged)
// #include "terminality/Core/InputEvent.hpp" (Merged)
// #include "terminality/Core/Layout.hpp" (Merged)
// #include "terminality/Core/TextHelper.hpp" (Merged)

// terminality.Controls.Layout
// #include "terminality/Controls/Layout/Grid.hpp" (Merged)
// #include "terminality/Controls/Layout/StackPanel.hpp" (Merged)
// #include "terminality/Controls/Layout/ItemsControl.hpp" (Merged)
// #include "terminality/Controls/Layout/ScrollViewer.hpp" (Merged)
// #include "terminality/Controls/Layout/TabControl.hpp" (Merged)

// terminality.Controls.Visual
// #include "terminality/Controls/Visual/Label.hpp" (Merged)
// #include "terminality/Controls/Visual/Border.hpp" (Merged)
// #include "terminality/Controls/Visual/Spinner.hpp" (Merged)
// #include "terminality/Controls/Visual/ProgressBar.hpp" (Merged)

// terminality.Controls.Interactable
// #include "terminality/Controls/Interactable/CheckBox.hpp" (Merged)
// #include "terminality/Controls/Interactable/Button.hpp" (Merged)
// #include "terminality/Controls/Interactable/TextBox.hpp" (Merged)

// terminality.Dialogs
// #include "terminality/Dialogs/ContextMenu.hpp" (Merged)
// #include "terminality/Dialogs/MessageBox.hpp" (Merged)
// #include "terminality/Dialogs/OpenFileDialog.hpp" (Merged)

// terminality.Engine
// #include "terminality/Engine/DispatchTimer.hpp" (Merged)
// #include "terminality/Engine/FocusManager.hpp" (Merged)
// #include "terminality/Engine/Navigator.hpp" (Merged)
// #include "terminality/Engine/RenderBuffer.hpp" (Merged)
// #include "terminality/Engine/RenderContext.hpp" (Merged)
// #include "terminality/Engine/RenderStream.hpp" (Merged)

// terminality.Framework.Collections
// #include "terminality/Framework/Collections/ObservableCollection.hpp" (Merged)
// #include "terminality/Framework/Collections/Queries.hpp" (Merged)

// terminality.Framework
// #include "terminality/Framework/VisualTree.hpp" (Merged)
// #include "terminality/Framework/VisualTreeNode.hpp" (Merged)
// #include "terminality/Framework/Property.hpp" (Merged)
// #include "terminality/Framework/ControlBase.hpp" (Merged)
// #include "terminality/Framework/Event.hpp" (Merged)
// #include "terminality/Framework/HostApplication.hpp" (Merged)

// terminality.PlatformSpecific
#ifdef _WIN32
// #include "terminality/Framework/PlatformSpecific/Windows.hpp" (Merged)
#endif // _WIN32
// --- End Header: terminality/Terminality.hpp ---

#endif // TERMINALITY_SINGLE_HEADER_H

#ifdef TERMINALITY_IMPLEMENTATION

// --- Begin Source: Controls/Interactable/Button.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

void Button::Click()
{
	Clicked.Emit();
	InvalidateVisual();
}

void Button::OnPropertyChanged(const char* propertyName)
{
	ControlBase::OnPropertyChanged(propertyName);
}

bool Button::OnKeyDown(InputEvent input)
{
	switch (input.Key)
	{
		case InputKey::RETURN:
		case InputKey::SPACE:
		{
			isPressed_ = true;
			InvalidateVisual();
			Clicked.Emit();
			return true;
		}
	}

	return ControlBase::OnKeyDown(input);
}

bool Button::OnKeyUp(InputEvent input)
{
	switch (input.Key)
	{
		case InputKey::RETURN:
		case InputKey::SPACE:
		{
			isPressed_ = false;
			InvalidateVisual();
			return true;
		}
	}

	return ControlBase::OnKeyUp(input);
}

void Button::OnLostFocus()
{
	focused_ = false;
	isPressed_ = false;
	InvalidateVisual();
}

Size Button::MeasureOverride(const Size& availableSize)
{
	int32_t contentWidth = static_cast<int32_t>(Text->size()) + 6;
	int32_t width = availableSize.Width >= 0 ? std::min(availableSize.Width, contentWidth) : contentWidth;
	int32_t height = availableSize.Height >= 0 ? std::min(availableSize.Height, 1) : 1;
	return Size(width, height);
}

void Button::ArrangeOverride(const Rect& contentRect)
{
	// bleh U_U
	return;
}

void Button::RenderOverride(RenderContext& context)
{
	const Rect rect = context.ContextRect();
	std::wstring line = L"[  " + Text.Get() + L"  ]";

	Color fore = ForegroundColor;
	Color back = BackgroundColor;

	if (focused_)
	{
		if (isPressed_)
		{
			fore = PressedForegroundColor;
			back = PressedBackgroundColor;
		}
		else
		{
			fore = FocusedForegroundColor;
			back = FocusedBackgroundColor;
		}
	}

	context.RenderText(Point::Zero, line, fore, back, false);
}
// --- End Source: Controls/Interactable/Button.cpp ---

// --- Begin Source: Controls/Interactable/CheckBox.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <optional> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

void CheckBox::Toggle(std::optional<bool> value)
{
	isChecked_ = value;
	Toggled.Emit(isChecked_);
	InvalidateVisual();

	if (isChecked_.has_value())
	{
		if (isChecked_.value())
			Checked.Emit();
		else
			Unchecked.Emit();
	}
}

void CheckBox::OnPropertyChanged(const char* propertyName)
{
	ControlBase::OnPropertyChanged(propertyName);
}

bool CheckBox::OnKeyDown(InputEvent input)
{
	switch (input.Key)
	{
		case InputKey::RETURN:
		case InputKey::SPACE:
		{
			isPressed_ = true;
			InvalidateVisual();
			return true;
		}
	}

	return ControlBase::OnKeyDown(input);
}

bool CheckBox::OnKeyUp(InputEvent input)
{
	switch (input.Key)
	{
		case InputKey::RETURN:
		case InputKey::SPACE:
		{
			isPressed_ = false;
			if (!isChecked_.has_value())
			{
				isChecked_ = true;
			}
			else if (isChecked_.value())
			{
				isChecked_ = false;
			}
			else
			{
				isChecked_ = true;
			}

			InvalidateVisual();
			return true;
		}
	}

	return ControlBase::OnKeyUp(input);
}

void CheckBox::OnLostFocus()
{
	focused_ = false;
	isPressed_ = false;
	InvalidateVisual();
}

Size CheckBox::MeasureOverride(const Size& availableSize)
{
	int32_t contentWidth = static_cast<int32_t>(Text->size()) + 7 + 3;
	int32_t width = availableSize.Width >= 0 ? std::min(availableSize.Width, contentWidth) : contentWidth;
	int32_t height = availableSize.Height >= 0 ? std::min(availableSize.Height, 1) : 1;
	return Size(width, height);
}

void CheckBox::ArrangeOverride(const Rect& contentRect)
{
	// bleh U_U
	return;
}

void CheckBox::RenderOverride(RenderContext& context)
{
	const Rect rect = context.ContextRect();
	std::wstring line = L"";

	if (!isChecked_.has_value())
	{
		line += L"[?]";
	}
	else if (isChecked_.value())
	{
		line += L"[X]";
	}
	else
	{
		line += L"[ ]";
	}

	line += L" [  " + Text.Get() + L"  ]";
	Color fore = ForegroundColor;
	Color back = BackgroundColor;

	if (focused_)
	{
		if (isPressed_)
		{
			fore = PressedForegroundColor;
			back = PressedBackgroundColor;
		}
		else
		{
			fore = FocusedForegroundColor;
			back = FocusedBackgroundColor;
		}
	}

	context.RenderText(Point::Zero, line, fore, back, false);
}
// --- End Source: Controls/Interactable/CheckBox.cpp ---

// --- Begin Source: Controls/Interactable/TextBox.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <vector> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

TextBox::TextBox()
{
	isTabStop_ = true;
	FocusedBackgroundColor = BackgroundColor;
	FocusedForegroundColor = ForegroundColor;
}

void TextBox::OnPropertyChanged(const char* propertyName)
{
	if (std::strcmp(propertyName, "Text") == 0)
	{
		cursorPosition_ = std::min(cursorPosition_, Text.Get().length());
		TextChanged.Emit();
	}

	ControlBase::OnPropertyChanged(propertyName);
}

Size TextBox::MeasureOverride(const Size& availableSize)
{
	std::vector<int32_t> lines = TextHelper::MeasureLines(Text, availableSize.Width, TextWrapping);

	int32_t maxWidth = 0;
	for (const auto& line : lines)
		maxWidth = std::max(maxWidth, line);

	int32_t width = availableSize.Width >= 0 ? std::clamp(maxWidth + 1, 0, availableSize.Width) : maxWidth + 1;
	int32_t desiredHeight = std::max<int32_t>(1, static_cast<int32_t>(lines.size()));
	int32_t height = availableSize.Height >= 0 ? std::min(desiredHeight, availableSize.Height) : desiredHeight;

	return Size(width, height);
}

void TextBox::ArrangeOverride(const Rect& contentRect)
{
	return;
}

void TextBox::RenderOverride(RenderContext& context)
{
	const Rect rect = context.ContextRect();

	Color fore = FocusedForegroundColor;
	Color back = FocusedBackgroundColor;

	for (int32_t y = 0; y < rect.Height; ++y)
	{
		for (int32_t x = 0; x < rect.Width; ++x)
		{
			context.SetCell(x, y, L' ', fore, back);
		}
	}

	std::vector<LineInfo> lines = TextHelper::GetLines(Text.Get(), rect.Width, TextWrapping.Get());

	int32_t cursorY = 0;
	int32_t cursorX = 0;
	bool cursorFound = false;

	// First find the cursor
	for (std::size_t y = 0; y < lines.size(); ++y)
	{
		const auto& line = lines[y];
		if (!cursorFound && cursorPosition_ >= line.StartIndex && (y + 1 == lines.size() || cursorPosition_ < lines[y + 1].StartIndex))
		{
			cursorY = static_cast<int32_t>(y);
			cursorX = static_cast<int32_t>(cursorPosition_ - line.StartIndex);
			cursorFound = true;
			break;
		}
	}

	int32_t viewWidth = rect.Width;
	int32_t offset = 0;

	if (TextWrapping == TextWrap::NoWrap)
	{
		if (cursorX >= viewWidth)
		{
			offset = cursorX - viewWidth + 1;
		}
	}

	// Adjust cursorX based on scroll offset
	cursorX -= offset;

	for (int32_t y = 0; y < std::min(rect.Height, static_cast<int32_t>(lines.size())); ++y)
	{
		const auto& line = lines[y];
		int32_t xOffset = 0;

		std::wstring visibleText = line.Text;
		if (offset > 0 && offset < visibleText.length())
			visibleText = visibleText.substr(offset);
		else if (offset >= visibleText.length())
			visibleText = L"";

		int32_t textLen = static_cast<int32_t>(visibleText.length());
		if (TextAlignment.Get() == terminality::TextAlign::Center)
		{
			xOffset = std::max(0, (rect.Width - textLen) / 2);
		}
		else if (TextAlignment.Get() == terminality::TextAlign::Right)
		{
			xOffset = std::max(0, rect.Width - textLen);
		}

		if (y == cursorY)
		{
			cursorX += xOffset;
		}

		int32_t renderWidth = std::min(rect.Width - xOffset, textLen);
		if (renderWidth > 0)
		{
			context.RenderText(Point(xOffset, y), visibleText.substr(0, renderWidth), fore, back, false);
		}
	}

	if (focused_ && cursorY < rect.Height && cursorX < rect.Width && cursorX >= 0)
	{
		wchar_t cursorChar = (cursorPosition_ < Text.Get().length() && Text.Get()[cursorPosition_] != L'\n') ? Text.Get()[cursorPosition_] : L' ';
		context.SetCell(cursorX, cursorY, cursorChar, back, fore);
	}
}

bool TextBox::OnKeyDown(InputEvent input)
{
	if (ControlBase::OnKeyDown(input))
		return true;

	std::wstring currentText = Text.Get();
	auto applyTextChange = [&]()
	{
		Text = currentText;
		InvalidateMeasure();
		InvalidateVisual();
	};

	auto applyCursorMove = [&]()
	{
		InvalidateMeasure();
		InvalidateVisual();
	};

	auto getLineIndex = [&](const std::vector<LineInfo>& lines) -> size_t
	{
		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			if (cursorPosition_ >= lines[i].StartIndex && (i + 1 == lines.size() || cursorPosition_ < lines[i + 1].StartIndex))
				return i;
		}

		return 0;
	};

	switch (input.Key)
	{
		default:
		{
			if (input.Char == L'\0' || input.Char < 32)
				return false;

			currentText.insert(cursorPosition_++, 1, input.Char);
			applyTextChange();
			return true;
		}

		case InputKey::SPACE:
		{
			currentText.insert(cursorPosition_++, 1, L' ');
			applyTextChange();
			return true;
		}

		case InputKey::RETURN:
		{
			if (!AcceptsReturn.Get())
				return true;

			currentText.insert(cursorPosition_++, 1, L'\n');
			applyTextChange();
			return true;
		}

		case InputKey::UP:
		{
			if (!AcceptsReturn.Get() && TextWrapping.Get() == terminality::TextWrap::NoWrap)
			{
				PopFocus(Direction::Up, input.Modifier);
				return true;
			}

			auto lines = TextHelper::GetLines(currentText, arrangedRect_.Width, TextWrapping.Get());
			size_t lineIdx = getLineIndex(lines);

			if (lineIdx == 0)
			{
				PopFocus(Direction::Up, input.Modifier);
				return true;
			}

			size_t col = cursorPosition_ - lines[lineIdx].StartIndex;
			size_t prevLen = lines[lineIdx - 1].Text.length();

			if (prevLen > 0 && lines[lineIdx - 1].Text.back() == L'\n')
				prevLen--;

			cursorPosition_ = lines[lineIdx - 1].StartIndex + std::min(col, prevLen);
			applyCursorMove();
			return true;
		}

		case InputKey::DOWN:
		{
			if (!AcceptsReturn.Get() && TextWrapping.Get() == terminality::TextWrap::NoWrap)
			{
				PopFocus(Direction::Down, input.Modifier);
				return true;
			}

			auto lines = TextHelper::GetLines(currentText, arrangedRect_.Width, TextWrapping.Get());
			size_t lineIdx = getLineIndex(lines);

			if (lineIdx + 1 >= lines.size())
			{
				PopFocus(Direction::Down, input.Modifier);
				return true;
			}

			size_t col = cursorPosition_ - lines[lineIdx].StartIndex;
			size_t nextLen = lines[lineIdx + 1].Text.length();

			if (nextLen > 0 && lines[lineIdx + 1].Text.back() == L'\n')
				nextLen--;

			cursorPosition_ = lines[lineIdx + 1].StartIndex + std::min(col, nextLen);
			applyCursorMove();
			return true;
		}

		case InputKey::LEFT:
		{
			if (terminality::hasFlag(input.Modifier, InputModifier::LeftAlt) ||
				terminality::hasFlag(input.Modifier, InputModifier::RightAlt))
			{
				PopFocus(Direction::Left, input.Modifier);
				return true;
			}

			if (cursorPosition_ == 0)
				return true;

			cursorPosition_--;
			applyCursorMove();
			return true;
		}

		case InputKey::RIGHT:
		{
			if (terminality::hasFlag(input.Modifier, InputModifier::LeftAlt) ||
				terminality::hasFlag(input.Modifier, InputModifier::RightAlt))
			{
				PopFocus(Direction::Right, input.Modifier);
				return true;
			}

			if (cursorPosition_ >= currentText.length())
				return true;

			cursorPosition_++;
			applyCursorMove();
			return true;
		}

		case InputKey::BACK:
		{
			if (cursorPosition_ == 0)
				return true;

			currentText.erase(--cursorPosition_, 1);
			applyTextChange();
			return true;
		}

		case InputKey::DELETE:
		{
			if (cursorPosition_ >= currentText.length())
				return true;

			currentText.erase(cursorPosition_, 1);
			applyTextChange();
			return true;
		}

		case InputKey::HOME:
		{
			if (cursorPosition_ == 0)
				return true;

			cursorPosition_ = 0;
			applyCursorMove();
			return true;
		}

		case InputKey::END:
		{
			if (cursorPosition_ >= currentText.length())
				return true;

			cursorPosition_ = currentText.length();
			applyCursorMove();
			return true;
		}
	}

	return false;
}

bool TextBox::OnKeyUp(InputEvent input)
{
	return false;
}

void TextBox::OnGotFocus()
{
	focused_ = true;
	InvalidateVisual();
}

void TextBox::OnLostFocus()
{
	focused_ = false;
	InvalidateVisual();
}

bool TextBox::MoveFocusNext(Direction direction, InputModifier modifiers)
{
	if (!focusable_)
		return false;

	if (!focused_)
		return true;

	if (modifiers == InputModifier::None && (direction == Direction::Left || direction == Direction::Right))
		return true;

	return false;
}
// --- End Source: Controls/Interactable/TextBox.cpp ---

// --- Begin Source: Controls/Layout/Grid.cpp ---

// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <vector> (Moved to top)
// #include <limits> (Moved to top)
// #include <string> (Moved to top)
// #include <string_view> (Moved to top)
// #include <charconv> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

// ------------------------------------------------------------------
// String helpers
// ------------------------------------------------------------------

static constexpr std::string_view Trim(std::string_view str)
{
    const auto first = str.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};

    const auto last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static constexpr char ToLowerAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

static constexpr bool EqualsIgnoreCase(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;

    for (std::size_t i = 0; i < a.size(); ++i)
        if (ToLowerAscii(a[i]) != ToLowerAscii(b[i]))
            return false;

    return true;
}

static std::vector<GridLength> ParseGridLengths(std::string_view definitions)
{
    std::vector<GridLength> result;
    if (definitions.empty())
        return result;

    size_t start = 0;
    while (start < definitions.size())
    {
        size_t end = definitions.find(',', start);
        std::string_view token = Trim(definitions.substr(start, end - start));

        if (!token.empty())
        {
            if (EqualsIgnoreCase(token, "Auto"))
            {
                result.push_back(GridLength::Auto());
            }
            else if (token.back() == '*')
            {
                if (token.size() == 1)
                {
                    result.push_back(GridLength::Star(1.0f));
                }
                else
                {
                    float value = 1.0f;
                    auto numPart = token.substr(0, token.size() - 1);
                    std::from_chars(numPart.data(), numPart.data() + numPart.size(), value);
                    result.push_back(GridLength::Star(value));
                }
            }
            else
            {
                int value = 0;
                std::from_chars(token.data(), token.data() + token.size(), value);
                result.push_back(GridLength::Cell(value));
            }
        }

        if (end == std::string_view::npos)
            break;

        start = end + 1;
    }

    return result;
}

// ------------------------------------------------------------------
// GridLength
// ------------------------------------------------------------------

GridLength GridLength::Auto()
{
    return { 0.0f, GridUnitType::Auto };
}

GridLength GridLength::Cell(int32_t cells)
{
    return { static_cast<float>(cells), GridUnitType::Cell };
}

GridLength GridLength::Star(float weight)
{
    return { weight, GridUnitType::Star };
}

// ------------------------------------------------------------------
// Grid
// ------------------------------------------------------------------

void Grid::SetRowDefinitions(std::string_view definitions)
{
    rowDefs_.clear();
    auto lengths = ParseGridLengths(definitions);

    for (const auto& length : lengths)
        AddRow(RowDefinition{ length });
}

void Grid::SetColumnDefinitions(std::string_view definitions)
{
    colDefs_.clear();
    auto lengths = ParseGridLengths(definitions);

    for (const auto& length : lengths)
        AddColumn(ColumnDefinition{ length });
}

void Grid::AddRow(const RowDefinition& def)
{
    rowDefs_.push_back(def);
    InvalidateMeasure();
}

void Grid::AddColumn(const ColumnDefinition& def)
{
    colDefs_.push_back(def);
    InvalidateMeasure();
}

static void AttachChild(ControlBase& child, Grid* parent)
{
    child.SetParent(parent);
    if (!child.IsAttached())
        child.OnAttachedToTree();
}

void Grid::AddChild(int32_t row, int32_t column, int32_t rowSpan, int32_t colSpan, std::unique_ptr<ControlBase> child)
{
    if (!child)
        return;

    AttachChild(*child, this);
    children_.push_back({ std::move(child), row, column, rowSpan, colSpan });
    InvalidateMeasure();
}

void Grid::AddChild(int32_t row, int32_t column, std::unique_ptr<ControlBase> child)
{
    AddChild(row, column, 1, 1, std::move(child));
}

void Grid::EnsureGridDefinitions()
{
    if (rowDefs_.empty())
        rowDefs_.push_back(RowDefinition{});

    if (colDefs_.empty())
        colDefs_.push_back(ColumnDefinition{});
}

template <typename T>
static int32_t ClampIndex(int32_t index, const std::vector<T>& container)
{
    return std::clamp<int32_t>(index, 0, static_cast<int32_t>(container.size()) - 1);
}

template <typename T>
static int32_t SumSpan(const std::vector<T>& defs, int32_t startIndex, int32_t span,
                       int32_t T::* actualSize)
{
    int32_t sum = 0;
    for (int32_t i = 0; i < span && (startIndex + i) < static_cast<int32_t>(defs.size()); ++i)
        sum += defs[startIndex + i].*actualSize;

    return sum;
}

Size Grid::MeasureOverride(const Size& availableSize)
{
    EnsureGridDefinitions();

    const bool widthIsInfinite = availableSize.Width < 0;
    const bool heightIsInfinite = availableSize.Height < 0;

    for (auto& row : rowDefs_)
        row.ActualHeight = (row.Height.Type == GridUnitType::Cell) ? static_cast<int32_t>(row.Height.Value) : 0;

    for (auto& col : colDefs_)
        col.ActualWidth = (col.Width.Type == GridUnitType::Cell) ? static_cast<int32_t>(col.Width.Value) : 0;

    for (auto& childWrapper : children_)
    {
        int32_t rowIndex = ClampIndex(childWrapper.Row, rowDefs_);
        int32_t columnIndex = ClampIndex(childWrapper.Column, colDefs_);

        bool isRowAuto = rowDefs_[rowIndex].Height.Type == GridUnitType::Auto ||
            (rowDefs_[rowIndex].Height.Type == GridUnitType::Star && heightIsInfinite);

        bool isColAuto = colDefs_[columnIndex].Width.Type == GridUnitType::Auto ||
            (colDefs_[columnIndex].Width.Type == GridUnitType::Star && widthIsInfinite);

        if (isRowAuto || isColAuto)
        {
            Size childDesired = childWrapper.Control->Measure(availableSize);

            if (isColAuto)
                colDefs_[columnIndex].ActualWidth = std::max(colDefs_[columnIndex].ActualWidth, childDesired.Width);

            if (isRowAuto)
                rowDefs_[rowIndex].ActualHeight = std::max(rowDefs_[rowIndex].ActualHeight, childDesired.Height);
        }
    }

    int32_t fixedWidth = 0, fixedHeight = 0;
    float totalStarWidth = 0.0f, totalStarHeight = 0.0f;

    for (const auto& col : colDefs_)
    {
        if (col.Width.Type == GridUnitType::Star && !widthIsInfinite)
            totalStarWidth += col.Width.Value;
        else
            fixedWidth += col.ActualWidth;
    }

    for (const auto& row : rowDefs_)
    {
        if (row.Height.Type == GridUnitType::Star && !heightIsInfinite)
            totalStarHeight += row.Height.Value;
        else
            fixedHeight += row.ActualHeight;
    }

    if (totalStarWidth > 0 && !widthIsInfinite)
    {
        int32_t remainingWidth = std::max(0, availableSize.Width - fixedWidth);
        for (auto& col : colDefs_)
        {
            if (col.Width.Type == GridUnitType::Star)
                col.ActualWidth = static_cast<int32_t>((col.Width.Value / totalStarWidth) * remainingWidth);
        }
    }

    if (totalStarHeight > 0 && !heightIsInfinite)
    {
        int32_t remainingHeight = std::max(0, availableSize.Height - fixedHeight);
        for (auto& row : rowDefs_)
        {
            if (row.Height.Type == GridUnitType::Star)
                row.ActualHeight = static_cast<int32_t>((row.Height.Value / totalStarHeight) * remainingHeight);
        }
    }

    for (auto& childWrapper : children_)
    {
        int32_t rowIndex = ClampIndex(childWrapper.Row, rowDefs_);
        int32_t columnIndex = ClampIndex(childWrapper.Column, colDefs_);

        int32_t cellWidth = SumSpan(colDefs_, columnIndex, childWrapper.ColumnSpan, &ColumnDefinition::ActualWidth);
        int32_t cellHeight = SumSpan(rowDefs_, rowIndex, childWrapper.RowSpan, &RowDefinition::ActualHeight);

        childWrapper.Control->Measure(Size(cellWidth, cellHeight));
    }

    int32_t totalDesiredWidth = 0;
    int32_t totalDesiredHeight = 0;

    for (const auto& col : colDefs_)
        totalDesiredWidth += col.ActualWidth;

    for (const auto& row : rowDefs_)
        totalDesiredHeight += row.ActualHeight;

    return Size(totalDesiredWidth, totalDesiredHeight);
}

void Grid::ArrangeOverride(const Rect& contentRect)
{
    EnsureGridDefinitions();

    int32_t currentX = 0;
    for (auto& col : colDefs_)
    {
        col.OffsetX = currentX;
        currentX += col.ActualWidth;
    }

    int32_t currentY = 0;
    for (auto& row : rowDefs_)
    {
        row.OffsetY = currentY;
        currentY += row.ActualHeight;
    }

    for (auto& childWrapper : children_)
    {
        int32_t rowIndex = ClampIndex(childWrapper.Row, rowDefs_);
        int32_t columnIndex = ClampIndex(childWrapper.Column, colDefs_);

        int32_t cellWidth = SumSpan(colDefs_, columnIndex, childWrapper.ColumnSpan, &ColumnDefinition::ActualWidth);
        int32_t cellHeight = SumSpan(rowDefs_, rowIndex, childWrapper.RowSpan, &RowDefinition::ActualHeight);

        Rect cellRect(
            contentRect.X + colDefs_[columnIndex].OffsetX,
            contentRect.Y + rowDefs_[rowIndex].OffsetY,
            cellWidth,
            cellHeight);

        childWrapper.Control->Arrange(cellRect);
    }
}

void Grid::RenderOverride(RenderContext& context)
{
    for (const auto& childWrapper : children_)
    {
        Rect childRect = childWrapper.Control->GetArrangedRect();
        RenderContext childContext = context.CreateInner(childRect);
        childWrapper.Control->Render(childContext);
    }
}

size_t Grid::VisualChildrenCount() const
{
    return children_.size();
}

VisualTreeNode* Grid::GetVisualChild(std::size_t index) const
{
    return children_.at(index).Control.get();
}

void Grid::OnGotFocus()
{
    if (focusedIndex_ < children_.size())
    {
        VisualTreeNode* focusedControl = children_[focusedIndex_].Control.get();
        if (focusedControl->IsFocusable())
        {
            PushFocus(focusedControl);
            InvalidateVisual();
            return;
        }
    }

    for (std::size_t i = 0; i < children_.size(); ++i)
    {
        VisualTreeNode* focusedControl = children_[i].Control.get();
        if (focusedControl->IsFocusable())
        {
            focusedIndex_ = i;
            PushFocus(focusedControl);
            InvalidateVisual();
            return;
        }
    }

    InvalidateVisual();
}

void Grid::OnLostFocus()
{
    focused_ = false;
    InvalidateVisual();
}

bool Grid::MoveFocusNext(Direction direction, InputModifier modifiers)
{
    if (children_.empty())
        return false;

    if (direction == Direction::Next)
    {
        for (std::size_t i = focusedIndex_ + 1; i < children_.size(); ++i)
        {
            ControlBase* control = children_[i].Control.get();
            if (control->IsFocusable() && control->IsTabStop())
            {
                focusedIndex_ = i;
                PushFocus(control);
                return true;
            }
        }
        return false;
    }

    if (direction == Direction::Previous)
    {
        if (focusedIndex_ == 0)
            return false;

        for (std::size_t i = focusedIndex_ - 1; i < children_.size(); --i)
        {
            ControlBase* control = children_[i].Control.get();
            if (control->IsFocusable() && control->IsTabStop())
            {
                focusedIndex_ = i;
                PushFocus(control);
                return true;
            }
        }
        return false;
    }

    if (focusedIndex_ >= children_.size())
        return false;

    const auto& current = children_[focusedIndex_];

    auto GetCellBounds = [](const GridChild& child) -> std::tuple<int, int, int, int>
    {
        int r1 = child.Row;
        int r2 = child.Row + std::max(1, child.RowSpan) - 1;
        int c1 = child.Column;
        int c2 = child.Column + std::max(1, child.ColumnSpan) - 1;
        return { r1, r2, c1, c2 };
    };

    auto [r1, r2, c1, c2] = GetCellBounds(current);

    size_t bestIndex = std::numeric_limits<std::size_t>::max();
    int minPrimary = std::numeric_limits<int>::max();
    int minSecondary = std::numeric_limits<int>::max();

    for (std::size_t i = 0; i < children_.size(); ++i)
    {
        if (i == focusedIndex_)
            continue;

        const auto& candidate = children_[i];
        ControlBase* ctrl = candidate.Control.get();

        if (!ctrl->IsFocusable() || !ctrl->IsTabStop())
            continue;

        auto [cr1, cr2, cc1, cc2] = GetCellBounds(candidate);

        int primaryDist = -1;
        int secondaryDist = 0;

        switch (direction)
        {
            case Direction::Right:
                if (cc1 > c2)
                {
                    primaryDist = cc1 - c2;
                    secondaryDist = (cr2 >= r1 && cr1 <= r2) ? 0 : std::min(std::abs(cr1 - r2), std::abs(cr2 - r1));
                }
                break;

            case Direction::Left:
                if (cc2 < c1)
                {
                    primaryDist = c1 - cc2;
                    secondaryDist = (cr2 >= r1 && cr1 <= r2) ? 0 : std::min(std::abs(cr1 - r2), std::abs(cr2 - r1));
                }
                break;

            case Direction::Down:
                if (cr1 > r2)
                {
                    primaryDist = cr1 - r2;
                    secondaryDist = (cc2 >= c1 && cc1 <= c2) ? 0 : std::min(std::abs(cc1 - c2), std::abs(cc2 - c1));
                }
                break;

            case Direction::Up:
                if (cr2 < r1)
                {
                    primaryDist = r1 - cr2;
                    secondaryDist = (cc2 >= c1 && cc1 <= c2) ? 0 : std::min(std::abs(cc1 - c2), std::abs(cc2 - c1));
                }
                break;

            default:
                break;
        }

        if (primaryDist > 0)
        {
            if (primaryDist < minPrimary || (primaryDist == minPrimary && secondaryDist < minSecondary))
            {
                minPrimary = primaryDist;
                minSecondary = secondaryDist;
                bestIndex = i;
            }
        }
    }

    if (bestIndex != std::numeric_limits<std::size_t>::max())
    {
        focusedIndex_ = bestIndex;
        PushFocus(children_[bestIndex].Control.get());
        return true;
    }

    return false;
}
// --- End Source: Controls/Layout/Grid.cpp ---

// --- Begin Source: Controls/Layout/ScrollViewer.cpp ---

// #include <algorithm> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

Size ScrollViewer::MeasureOverride(const Size& availableSize)
{
    if (Content)
    {
        Size desired = Content->Measure(Size::Auto);
        return Size(
            std::min(availableSize.Width, desired.Width),
            std::min(availableSize.Height, desired.Height)
        );
    }

    return Size::Zero;
}

void ScrollViewer::ArrangeOverride(const Rect& finalRect)
{
    arrangedRect_ = finalRect;

    if (Content != nullptr)
    {
        Size extent = Content->GetActualSize();

        int maxScrollX = std::max(0, extent.Width - finalRect.Width);
        int maxScrollY = std::max(0, extent.Height - finalRect.Height);

        int cx = std::clamp(ScrollX.Get(), 0, maxScrollX);
        int cy = std::clamp(ScrollY.Get(), 0, maxScrollY);

        ScrollX = cx;
        ScrollY = cy;

        Rect contentRect(
            finalRect.X - cx,
            finalRect.Y - cy,
            extent.Width,
            extent.Height
        );

        Content->Arrange(contentRect);
    }
}

void ScrollViewer::RenderOverride(RenderContext& context)
{
    if (Content != nullptr)
    {
        RenderContext childContext = context.CreateInner(context.ContextRect());
        Content->Render(childContext);
    }

    int viewHeight = GetViewportHeight();
    int extentHeight = GetExtentHeight();

    if (extentHeight > viewHeight && viewHeight >= 3)
    {
        int maxScrollY = extentHeight - viewHeight;
        int cy = ScrollY.Get();

        float progress = (float)cy / maxScrollY;
        int scrollArea = viewHeight - 2;
        int indicatorPos = 1 + (int)(progress * (scrollArea - 1));

        int x = GetArrangedRect().Width - 1;
        context.SetCell(x, 0, L'^');
        context.SetCell(x, viewHeight - 1, L'v');

        for (int i = 1; i < viewHeight - 1; i++)
        {
            if (i == indicatorPos)
                context.SetCell(x, i, L'#');
            else
                context.SetCell(x, i, L'|');
        }
    }
}

bool ScrollViewer::OnKeyDown(InputEvent input)
{
    if (hasFlag(input.Modifier, InputModifier::LeftAlt) || hasFlag(input.Modifier, InputModifier::RightAlt))
        return false;

    int currentY = ScrollY.Get();
    if (input.Key == InputKey::UP)
    {
        ScrollY = std::max(0, currentY - 1);
        return true;
    }
    else if (input.Key == InputKey::DOWN)
    {
        int maxScrollY = std::max(0, GetExtentHeight() - GetViewportHeight());
        ScrollY = std::min(maxScrollY, currentY + 1);
        return true;
    }

    return false;
}

int ScrollViewer::GetExtentWidth() const
{
    return Content ? Content->GetActualSize().Width : 0;
}

int ScrollViewer::GetExtentHeight() const
{
    return Content ? Content->GetActualSize().Height : 0;
}

int ScrollViewer::GetViewportWidth() const
{
    return GetArrangedRect().Width;
}

int ScrollViewer::GetViewportHeight() const
{
    return GetArrangedRect().Height;
}
// --- End Source: Controls/Layout/ScrollViewer.cpp ---

// --- Begin Source: Controls/Layout/StackPanel.cpp ---
// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <functional> (Moved to top)
// #include <vector> (Moved to top)
// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

void StackPanel::AddChild(std::unique_ptr<ControlBase> child)
{
    if (child == nullptr)
        return;

    child->SetParent(this);
    if (!child->IsAttached())
        child->OnAttachedToTree();

    contents_.push_back(std::move(child));

    if (AutoScrollToEnd.Get())
    {
        forceScrollToEnd_ = true;
        focusedIndex_ = contents_.size() - 1;
    }

    InvalidateMeasure();
}

std::unique_ptr<ControlBase> StackPanel::RemoveChild(ControlPredicate predicate)
{
    const auto found = std::find_if(contents_.begin(), contents_.end(),
        [&](const std::unique_ptr<ControlBase>& control) { return predicate(control.get()); });

    if (found == contents_.end())
        return nullptr;

    std::unique_ptr<ControlBase> removed = std::move(*found);
    contents_.erase(found);

    if (removed)
        removed->SetParent(nullptr);

    if (focusedIndex_ >= contents_.size())
        focusedIndex_ = contents_.empty() ? 0 : contents_.size() - 1;

    InvalidateMeasure();
    return removed;
}

void StackPanel::Insert(std::size_t index, std::unique_ptr<ControlBase> child)
{
    if (child == nullptr)
        return;

    if (index > contents_.size())
        index = contents_.size();

    child->SetParent(this);
    if (!child->IsAttached())
        child->OnAttachedToTree();

    contents_.insert(contents_.begin() + index, std::move(child));

    if (focusedIndex_ >= index && focusedIndex_ < contents_.size() - 1)
        focusedIndex_++;
    else
        PushFocus(child.get());

    if (AutoScrollToEnd.Get())
    {
        forceScrollToEnd_ = true;
        focusedIndex_ = contents_.size() - 1;
    }

    InvalidateMeasure();
}

std::unique_ptr<ControlBase> StackPanel::RemoveAt(std::size_t index)
{
    if (index >= contents_.size())
        return nullptr;

    std::unique_ptr<ControlBase> removed = std::move(contents_[index]);
    contents_.erase(contents_.begin() + index);

    if (removed)
        removed->SetParent(nullptr);

    if (focusedIndex_ >= contents_.size())
        focusedIndex_ = contents_.empty() ? 0 : contents_.size() - 1;

    InvalidateMeasure();
    return removed;
}

void StackPanel::Clear()
{
    for (auto& child : contents_)
    {
        if (child)
            child->SetParent(nullptr);
    }

    contents_.clear();
    focusedIndex_ = 0;
    scrollOffset_ = 0;
    InvalidateMeasure();
}

void StackPanel::ScrollIntoView()
{
    if (!Scrollable || contents_.empty() || focusedIndex_ >= contents_.size())
        return;

    int32_t childStart = 0;
    int32_t childSize = 0;

    for (std::size_t i = 0; i <= focusedIndex_; ++i)
    {
        Size sz = contents_[i]->GetActualSize();
        if (i == focusedIndex_)
        {
            childSize = (ContentOrientation == Orientation::Vertical) ? sz.Height : sz.Width;
            break;
        }

        childStart += (ContentOrientation == Orientation::Vertical) ? sz.Height : sz.Width;
    }

    int32_t viewSize = (ContentOrientation == Orientation::Vertical) ? GetArrangedRect().Height : GetArrangedRect().Width;
    if (childStart < scrollOffset_)
    {
        scrollOffset_ = childStart;
        InvalidateArrange();
    }
    else if (childStart + childSize > scrollOffset_ + viewSize)
    {
        scrollOffset_ = childStart + childSize - viewSize;
        InvalidateArrange();
    }
}

void StackPanel::OnGotFocus()
{
    if (contents_.empty())
    {
        InvalidateVisual();
        return;
    }

    if (focusedIndex_ < contents_.size())
    {
        VisualTreeNode* focusedControl = contents_[focusedIndex_].get();
        if (focusedControl->IsFocusable())
        {
            PushFocus(focusedControl);
            ScrollIntoView();
            InvalidateVisual();
            return;
        }
    }

    for (std::size_t i = 0; i < contents_.size(); ++i)
    {
        VisualTreeNode* focusedControl = contents_[i].get();
        if (focusedControl->IsFocusable())
        {
            focusedIndex_ = i;
            PushFocus(focusedControl);
            ScrollIntoView();
            break;
        }
    }

    InvalidateVisual();
}

void StackPanel::OnLostFocus()
{
    focused_ = false;
    InvalidateVisual();
}

size_t StackPanel::VisualChildrenCount() const
{
    return contents_.size();
}

VisualTreeNode* StackPanel::GetVisualChild(std::size_t index) const
{
    return contents_.at(index).get();
}

void StackPanel::OnPropertyChanged(const char* propertyName)
{
    ControlBase::OnPropertyChanged(propertyName);
}

bool StackPanel::MoveFocusNext(Direction direction, InputModifier modifiers)
{
    if (contents_.empty())
        return false;

    bool goBack = false;
    bool goForward = false;

    switch (direction)
    {
        case Direction::Previous:
        {
            goBack = true;
            break;
        }
        case Direction::Next:
        {
            goForward = true;
            break;
        }
        case Direction::Up:
        {
            if (ContentOrientation == Orientation::Vertical)
                goBack = true;
            break;
        }
        case Direction::Down:
        {
            if (ContentOrientation == Orientation::Vertical)
                goForward = true;
            break;
        }
        case Direction::Left:
        {
            if (ContentOrientation == Orientation::Horizontal)
                goBack = true;
            break;
        }
        case Direction::Right:
        {
            if (ContentOrientation == Orientation::Horizontal)
                goForward = true;
            break;
        }
    }

    size_t focusedIndexPre = focusedIndex_;
    if (goBack)
    {
        if (focusedIndex_ == 0)
        {
            if (Looping)
            {
                if (hasFlag(modifiers, InputModifier::LeftAlt) || hasFlag(modifiers, InputModifier::RightAlt))
                    return false;
                focusedIndex_ = contents_.size() - 1;
            }
            else
            {
                return false;
            }
        }
        else
        {
            focusedIndex_ -= 1;
        }

        for (int i = static_cast<int>(focusedIndex_); i >= 0; i--)
        {
            ControlBase* control = contents_[i].get();
            if (control->IsFocusable() && control->IsTabStop())
            {
                focusedIndex_ = i;
                PushFocus(control);
                ScrollIntoView();
                return true;
            }
        }
    }
    else if (goForward)
    {
        if (focusedIndex_ == contents_.size() - 1)
        {
            if (Looping)
            {
                if (hasFlag(modifiers, InputModifier::LeftAlt) || hasFlag(modifiers, InputModifier::RightAlt))
                    return false;
                focusedIndex_ = 0;
            }
            else
            {
                return false;
            }
        }
        else
        {
            focusedIndex_ += 1;
        }

        for (std::size_t i = focusedIndex_; i < contents_.size(); i++)
        {
            ControlBase* control = contents_[i].get();
            if (control->IsFocusable() && control->IsTabStop())
            {
                focusedIndex_ = i;
                PushFocus(control);
                ScrollIntoView();
                return true;
            }
        }
    }

    focusedIndex_ = focusedIndexPre;
    return false;
}

Size StackPanel::MeasureOverride(const Size& availableSize)
{
    int32_t totalWidth = 0;
    int32_t totalHeight = 0;

    for (const auto& child : contents_)
    {
        Size childAvailable = availableSize;
        if (ContentOrientation == Orientation::Vertical)
            childAvailable.Height = -1;
        else
            childAvailable.Width = -1;

        const Size childSize = child->Measure(childAvailable);
        if (ContentOrientation == Orientation::Vertical)
        {
            totalWidth = std::max(childSize.Width, totalWidth);
            totalHeight += childSize.Height;
        }
        else
        {
            totalWidth += childSize.Width;
            totalHeight = std::max(childSize.Height, totalHeight);
        }
    }

    return Size(totalWidth, totalHeight);
}

void StackPanel::ArrangeOverride(const Rect& contentRect)
{
    int32_t totalContentWidth = 0;
    int32_t totalContentHeight = 0;

    for (const auto& child : contents_)
    {
        const Size childSize = child->GetActualSize();
        if (ContentOrientation == Orientation::Vertical)
        {
            totalContentWidth = std::max(totalContentWidth, childSize.Width);
            totalContentHeight += childSize.Height;
        }
        else
        {
            totalContentWidth += childSize.Width;
            totalContentHeight = std::max(totalContentHeight, childSize.Height);
        }
    }

    if (Scrollable)
    {
        int32_t maxScroll = std::max(0, (ContentOrientation == Orientation::Vertical)
            ? (totalContentHeight - contentRect.Height)
            : (totalContentWidth - contentRect.Width));

        if (AutoScrollToEnd.Get() && forceScrollToEnd_)
        {
            scrollOffset_ = maxScroll;
            forceScrollToEnd_ = false;
        }

        scrollOffset_ = std::clamp(scrollOffset_, 0, maxScroll);
    }
    else
    {
        scrollOffset_ = 0;
    }

    int32_t startX = 0;
    int32_t startY = 0;

    if (ContentOrientation == Orientation::Vertical)
    {
        if (VerticalContentAlignment == VerticalAlign::Bottom)
            startY = std::max(0, contentRect.Height - totalContentHeight);
        else if (VerticalContentAlignment == VerticalAlign::Center)
            startY = std::max(0, contentRect.Height - totalContentHeight) / 2;
    }
    else
    {
        if (HorizontalContentAlignment == HorizontalAlign::Right)
            startX = std::max(0, contentRect.Width - totalContentWidth);
        else if (HorizontalContentAlignment == HorizontalAlign::Center)
            startX = std::max(0, contentRect.Width - totalContentWidth) / 2;
    }

    int32_t currentX = startX - (ContentOrientation == Orientation::Horizontal ? scrollOffset_ : 0);
    int32_t currentY = startY - (ContentOrientation == Orientation::Vertical ? scrollOffset_ : 0);

    for (const std::unique_ptr<ControlBase>& child : contents_)
    {
        const Size childSize = child->GetActualSize();

        if (ContentOrientation == Orientation::Vertical)
        {
            const Rect childRect(
                contentRect.X,
                contentRect.Y + currentY,
                contentRect.Width,
                childSize.Height);

            child->Arrange(childRect);
            currentY += childSize.Height;
        }
        else
        {
            const Rect childRect(
                contentRect.X + currentX,
                contentRect.Y,
                childSize.Width,
                contentRect.Height);

            child->Arrange(childRect);
            currentX += childSize.Width;
        }
    }
}

void StackPanel::RenderOverride(RenderContext& context)
{
    Rect allowed = context.ContextRect();

    for (const std::unique_ptr<ControlBase>& child : contents_)
    {
        Rect childRect = child->GetArrangedRect();
        if (allowed.Intersects(childRect))
        {
            RenderContext childContext = context.CreateInner(childRect);
            child->Render(childContext);
        }
    }

    if (Scrollable)
    {
        int32_t totalContentWidth = 0;
        int32_t totalContentHeight = 0;

        for (const auto& child : contents_)
        {
            const Size childSize = child->GetActualSize();
            if (ContentOrientation == Orientation::Vertical)
                totalContentHeight += childSize.Height;
            else
                totalContentWidth += childSize.Width;
        }

        if (ContentOrientation == Orientation::Vertical)
        {
            int viewHeight = GetArrangedRect().Height;
            if (totalContentHeight > viewHeight && viewHeight >= 3)
            {
                int maxScroll = totalContentHeight - viewHeight;
                float progress = (float)scrollOffset_ / maxScroll;
                int scrollArea = viewHeight - 2;
                int indicatorPos = 1 + (int)(progress * (scrollArea - 1));

                int x = GetArrangedRect().Width - 1;
                context.SetCell(x, 0, L'^');
                context.SetCell(x, viewHeight - 1, L'v');

                for (int i = 1; i < viewHeight - 1; i++)
                {
                    if (i == indicatorPos)
                        context.SetCell(x, i, L'#');
                    else
                        context.SetCell(x, i, L'|');
                }
            }
        }
        else
        {
            int viewWidth = GetArrangedRect().Width;
            if (totalContentWidth > viewWidth && viewWidth >= 3)
            {
                int maxScroll = totalContentWidth - viewWidth;
                float progress = (float)scrollOffset_ / maxScroll;
                int scrollArea = viewWidth - 2;
                int indicatorPos = 1 + (int)(progress * (scrollArea - 1));

                int y = GetArrangedRect().Height - 1;
                context.SetCell(0, y, L'<');
                context.SetCell(viewWidth - 1, y, L'>');

                for (int i = 1; i < viewWidth - 1; i++)
                {
                    if (i == indicatorPos)
                        context.SetCell(i, y, L'#');
                    else
                        context.SetCell(i, y, L'-');
                }
            }
        }
    }
}
// --- End Source: Controls/Layout/StackPanel.cpp ---

// --- Begin Source: Controls/Layout/TabControl.cpp ---

// #include <algorithm> (Moved to top)
// #include <vector> (Moved to top)
// #include <string> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

static constexpr bool in_bounds(int index, const std::vector<TabItem>& tabs)
{
	return index >= 0 && index < tabs.size();
}

void TabControl::AddTab(const std::string& header, std::unique_ptr<ControlBase> content)
{
    if (content)
    {
        content->SetParent(this);
        if (!content->IsAttached())
            content->OnAttachedToTree();
    }

    tabs_.emplace_back(header, std::move(content));
    InvalidateMeasure();
}

void TabControl::RemoveTab(int index)
{
    if (index >= 0 && index < tabs_.size())
    {
        if (tabs_[index].Content)
            tabs_[index].Content->SetParent(nullptr);

        tabs_.erase(tabs_.begin() + index);
        if (SelectedIndex.Get() >= tabs_.size())
            SelectedIndex = std::max(0, (int)tabs_.size() - 1);

        InvalidateMeasure();
    }
}

void TabControl::ClearTabs()
{
    for (auto& tab : tabs_)
    {
        if (tab.Content)
            tab.Content->SetParent(nullptr);
    }

    tabs_.clear();
    SelectedIndex = 0;
    InvalidateMeasure();
}

size_t TabControl::GetTabCount() const
{
    return tabs_.size();
}

bool TabControl::OnKeyDown(InputEvent input)
{
    if (tabs_.empty())
        return false;

    if (input.Key == InputKey::LEFT)
    {
        int idx = SelectedIndex.Get();
        if (idx > 0)
        {
            SelectedIndex = idx - 1;
            return true;
        }
    }
    else if (input.Key == InputKey::RIGHT)
    {
        int idx = SelectedIndex.Get();
        if (idx < (int)tabs_.size() - 1)
        {
            SelectedIndex = idx + 1;
            return true;
        }
    }
    else if (input.Key == InputKey::DOWN || input.Key == InputKey::RETURN)
    {
        int idx = SelectedIndex.Get();
        if (in_bounds(idx, tabs_))
        {
            ControlBase* content = tabs_[idx].Content.get();
            if (content != nullptr && content->IsFocusable())
            {
                PushFocus(content);
                return true;
            }
        }
    }

    return ControlBase::OnKeyDown(input);
}

std::size_t TabControl::VisualChildrenCount() const
{
    int idx = SelectedIndex.Get();
    if (in_bounds(idx, tabs_) && tabs_[idx].Content != nullptr)
        return 1;

    return 0;
}

VisualTreeNode* TabControl::GetVisualChild(std::size_t index) const
{
    int idx = SelectedIndex.Get();
    if (in_bounds(idx, tabs_))
    {
        ControlBase* content = tabs_[idx].Content.get();
        if (content != nullptr && index == 0)
            return content;
    }

    return nullptr;
}

bool TabControl::MoveFocusNext(Direction direction, InputModifier modifiers)
{
    if (tabs_.empty())
        return false;

    int idx = SelectedIndex.Get();
    if (in_bounds(idx, tabs_))
    {
        if (direction == Direction::Down || direction == Direction::Next)
        {
            ControlBase* content = tabs_[idx].Content.get();
            if (content != nullptr && content->IsFocusable())
            {
                PushFocus(content);
                return true;
            }
        }
    }

    return false;
}

void TabControl::OnGotFocus()
{
    InvalidateVisual();
}

void TabControl::OnLostFocus()
{
    InvalidateVisual();
}

void TabControl::OnPropertyChanged(const char* propertyName)
{
    ControlBase::OnPropertyChanged(propertyName);
}

Size TabControl::MeasureOverride(const Size& availableSize)
{
    int32_t totalHeaderWidth = 0;
    for (const auto& tab : tabs_)
    {
        totalHeaderWidth += tab.Header.length() + 5; // "[*] Header "
    }

    Size contentAvailable = availableSize;
    if (contentAvailable.Height != -1)
        contentAvailable.Height = std::max(0, contentAvailable.Height - 1);

    int32_t maxContentWidth = 0;
    int32_t maxContentHeight = 0;

    int idx = SelectedIndex.Get();
    if (in_bounds(idx, tabs_))
    {
        ControlBase* content = tabs_[idx].Content.get();
        if (content != nullptr)
        {
            Size childSize = content->Measure(contentAvailable);
            maxContentWidth = childSize.Width;
            maxContentHeight = childSize.Height;
        }
    }

    return Size(
        std::max(totalHeaderWidth, maxContentWidth),
        maxContentHeight + 1
    );
}

void TabControl::ArrangeOverride(const Rect& finalRect)
{
    int idx = SelectedIndex.Get();
    if (in_bounds(idx, tabs_))
    {
        ControlBase* content = tabs_[idx].Content.get();
        if (content != nullptr)
        {
            Rect contentRect(
                finalRect.X,
                finalRect.Y + 1,
                finalRect.Width,
                std::max(0, finalRect.Height - 1));

            content->Arrange(contentRect);
        }
    }
}

void TabControl::RenderOverride(RenderContext& context)
{
    int idx = SelectedIndex.Get();
    Rect bounds = context.ContextRect();

    int32_t currentX = 0;
    for (int i = 0; i < tabs_.size(); ++i)
    {
        const auto& tab = tabs_[i];
        std::string headerText;
        if (i == idx)
        {
            headerText = focused_
                ? ("[>] " + tab.Header + " ")
                : ("[*] " + tab.Header + " ");
        }
        else
        {
            headerText = "[ ] " + tab.Header + " ";
        }

        int width = headerText.length();
        if (currentX + width > bounds.Width)
        {
            std::string trunc = headerText.substr(0, std::max(0, bounds.Width - currentX));
            if (!trunc.empty())
            {
                Color fg = (i == idx)
                    ? FocusedForegroundColor.Get()
                    : ForegroundColor.Get();

                Color bg = (i == idx)
                    ? FocusedBackgroundColor.Get()
                    : BackgroundColor.Get();

                context.RenderText(Point(currentX, 0), trunc, fg, bg);
            }

            break;
        }

        Color fg = (i == idx)
            ? FocusedForegroundColor.Get()
            : ForegroundColor.Get();

        Color bg = (i == idx)
            ? FocusedBackgroundColor.Get()
            : BackgroundColor.Get();

        context.RenderText(Point(currentX, 0), headerText, fg, bg);
        currentX += width;
    }

    if (in_bounds(idx, tabs_))
    {
        ControlBase* content = tabs_[idx].Content.get();
        if (content != nullptr)
        {
            Rect childRect = tabs_[idx].Content->GetArrangedRect();
            RenderContext childContext = context.CreateInner(childRect);
            tabs_[idx].Content->Render(childContext);
        }
    }
}
// --- End Source: Controls/Layout/TabControl.cpp ---

// --- Begin Source: Controls/Visual/Border.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

static wchar_t DefaultBorderStyle(const RectanglePos pos)
{
	/*

	for (int32_t x = 0; x < rect.Width; ++x)
	{
		context.SetCell(x, 0,				L'\x2500', renderColor, BackgroundColor);
		context.SetCell(x, rect.Height - 1,	L'\x2500', renderColor, BackgroundColor);
	}

	for (int32_t y = 0; y < rect.Height; ++y)
	{
		context.SetCell(0, y,				L'\x2502', renderColor, BackgroundColor);
		context.SetCell(rect.Width - 1, y,	L'\x2502', renderColor, BackgroundColor);
	}

	context.SetCell(0, 0,							 L'\x256D', renderColor, BackgroundColor);
	context.SetCell(rect.Width - 1, 0,				 L'\x256E', renderColor, BackgroundColor);
	context.SetCell(0, rect.Height - 1,				 L'\x2570', renderColor, BackgroundColor);
	context.SetCell(rect.Width - 1, rect.Height - 1, L'\x256F', renderColor, BackgroundColor);

	*/

	switch (pos)
	{
		case RectanglePos::TopHorizontalLine:
		case RectanglePos::BottomHorizontalLine:
			return L'\x2500';

		case RectanglePos::LeftVerticalLine:
		case RectanglePos::RightVerticalLine:
			return L'\x2502';

		case RectanglePos::LeftTopCorner:
			return L'\x256D';

		case RectanglePos::LeftBottomCorner:
			return L'\x2570';

		case RectanglePos::RightTopCorner:
			return L'\x256E';

		case RectanglePos::RightBottomCorner:
			return L'\x256F';

		default:
			return L'#';
	}
}

Border::Border()
{
	Style = std::move(DefaultBorderStyle);
	FocusedForegroundColor = Color::CYAN;
	FocusedBackgroundColor = Color::CYAN;
}

Border::Border(std::unique_ptr<ControlBase> content)
{
	Style = std::move(DefaultBorderStyle);
	FocusedForegroundColor = Color::CYAN;
	FocusedBackgroundColor = Color::CYAN;
	Content = std::move(content);
}

void Border::OnPropertyChanged(const char* propertyName)
{
	if (std::strcmp(propertyName, "Content") == 0)
	{
		if (Content.Get() != nullptr)
			Content.Get()->SetParent(this);

		InvalidateMeasure();
		InvalidateVisual();
		return;
	}

	ControlBase::OnPropertyChanged(propertyName);
}

bool Border::MoveFocusNext(Direction direction, InputModifier modifiers)
{
	return false;
}

void Border::OnGotFocus()
{
	if (Content == nullptr)
	{
		InvalidateVisual();
		return;
	}

	focused_ = true;
	VisualTreeNode* focusedControl = Content->get();
	PushFocus(focusedControl);
	InvalidateVisual();
}

void Border::OnLostFocus()
{
	focused_ = false;
	InvalidateVisual();
}

Size Border::MeasureOverride(const Size& availableSize)
{
	const int thickness = 2;
	Size desiredSize(thickness, thickness);

	if (Content != nullptr)
	{
		const Size innerSize(
			availableSize.Width >= 0 ? std::max(0, availableSize.Width - thickness) : -1,
			availableSize.Height >= 0 ? std::max(0, availableSize.Height - thickness) : -1
		);

		const Size childSize = Content.Get()->Measure(innerSize);

		desiredSize.Width += childSize.Width;
		desiredSize.Height += childSize.Height;
	}

	if (availableSize.Width >= 0)
		desiredSize.Width = std::min(desiredSize.Width, availableSize.Width);

	if (availableSize.Height >= 0)
		desiredSize.Height = std::min(desiredSize.Height, availableSize.Height);

	return desiredSize;
}

void Border::ArrangeOverride(const Rect& contentRect)
{
	if (Content == nullptr)
		return;

	Rect arrangedRect = GetArrangedRect();
	const Rect innerRect(
		contentRect.X + 1,
		contentRect.Y + 1,
		std::max(0, arrangedRect.Width - 2),
		std::max(0, arrangedRect.Height - 2));

	Content.Get()->Arrange(innerRect);
}

void Border::RenderOverride(RenderContext& context)
{
	const Rect rect = context.ContextRect();
	const Size size = rect.AsSize();
	const Color renderColor = focused_ ? FocusedBorderColor : BorderColor;
	const BorderStyle style = Style;

	for (int32_t x = 0; x < rect.Width; ++x)
	{
		context.SetCell(x, 0,				style(RectanglePos::TopHorizontalLine),    renderColor, BackgroundColor);
		context.SetCell(x, rect.Height - 1, style(RectanglePos::BottomHorizontalLine), renderColor, BackgroundColor);
	}

	for (int32_t y = 0; y < rect.Height; ++y)
	{
		context.SetCell(0, y,				style(RectanglePos::LeftVerticalLine),  renderColor, BackgroundColor);
		context.SetCell(rect.Width - 1, y,	style(RectanglePos::RightVerticalLine), renderColor, BackgroundColor);
	}

	context.SetCell(0, 0,							 style(RectanglePos::LeftTopCorner),	 renderColor, BackgroundColor);
	context.SetCell(0, rect.Height - 1,				 style(RectanglePos::LeftBottomCorner),  renderColor, BackgroundColor);
	context.SetCell(rect.Width - 1, 0,				 style(RectanglePos::RightTopCorner),    renderColor, BackgroundColor);
	context.SetCell(rect.Width - 1, rect.Height - 1, style(RectanglePos::RightBottomCorner), renderColor, BackgroundColor);

	if (HeaderText->size() != 0)
	{
		context.RenderText(Point(2, 0), HeaderText, FocusedForegroundColor, BackgroundColor);
	}

	if (rect.Width > 2 || rect.Height > 2)
	{
		if (Content != nullptr)
		{
			Rect childRect = Content.Get()->GetArrangedRect();
			RenderContext childContext = context.CreateInner(childRect);
			Content.Get()->Render(childContext);
		}
	}
}

size_t Border::VisualChildrenCount() const
{
	return 1;
}

VisualTreeNode* Border::GetVisualChild(std::size_t index) const
{
	return Content.Get().get();
}
// --- End Source: Controls/Visual/Border.cpp ---

// --- Begin Source: Controls/Visual/Label.cpp ---

// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)
// #include <algorithm> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

Label::Label()
{
	SetFocusable(false);
}

Label::Label(std::wstring& text)
{
	SetFocusable(false);
	Text = text;
}

void Label::OnPropertyChanged(const char* propertyName)
{
	if (std::strcmp(propertyName, "Text") == 0)
	{
		TextChanged.Emit();
	}

	ControlBase::OnPropertyChanged(propertyName);
}

Size Label::MeasureOverride(const Size& availableSize)
{
	std::vector<int32_t> lines = TextHelper::MeasureLines(Text, availableSize.Width, TextWrapping);

	int32_t maxWidth = 0;
	for (const auto& line : lines)
		maxWidth = std::max(maxWidth, line);

	int32_t width = availableSize.Width >= 0 ? std::clamp(maxWidth + 1, 0, availableSize.Width) : maxWidth + 1;
	int32_t desiredHeight = std::max<int32_t>(1, static_cast<int32_t>(lines.size()));
	int32_t height = availableSize.Height >= 0 ? std::min(desiredHeight, availableSize.Height) : desiredHeight;

	return Size(width, height);
}

void Label::ArrangeOverride(const Rect& contentRect)
{
	return;
}

void Label::RenderOverride(RenderContext& context)
{
	const Rect rect = context.ContextRect();

	Color fore = focused_ ? FocusedForegroundColor : ForegroundColor;
	Color back = focused_ ? FocusedBackgroundColor : BackgroundColor;

	std::vector<LineInfo> lines = TextHelper::GetLines(Text.Get(), rect.Width, TextWrapping.Get());
	for (const auto& line : lines)
		context.RenderText(Point::Zero, line.Text, fore, back, false);
}
// --- End Source: Controls/Visual/Label.cpp ---

// --- Begin Source: Controls/Visual/ProgressBar.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

Size ProgressBar::MeasureOverride(const Size& availableSize)
{
	int32_t width = availableSize.Width >= 0 ? std::max(availableSize.Width, 1) : 1;
	int32_t height = 1; // availableSize.Height >= 0 ? std::max(availableSize.Height, 1) : 1;
	return Size(width, height);
}

void ProgressBar::ArrangeOverride(const Rect& contentRect)
{
	// bleh U_U
	return;
}

void ProgressBar::RenderOverride(RenderContext& context)
{
	float minVal = Minimum.Get();
	float maxVal = Maximum.Get();
	float val = Value.Get();

	if (maxVal <= minVal)
		maxVal = minVal + 1.0f;

	Rect renderRect = context.ContextRect();
	float clampedVal = std::clamp(val, minVal, maxVal);
	float fraction = (clampedVal - minVal) / (maxVal - minVal);
	float exactWidth = fraction * renderRect.Width;

	int fullCells = static_cast<int>(exactWidth);
	int partialIndex = static_cast<int>((exactWidth - fullCells) * 2.0f);

	Color bar = BarColor.Get();
	Color track = TrackColor.Get();

	static const wchar_t partialBlocks[] = { L' ', L'\x258C' };
	for (int y = 0; y < renderRect.Height; ++y)
	{
		for (int x = 0; x < renderRect.Width; ++x)
		{
			if (x < fullCells)
			{
				context.SetCell(x, y, L'\x2588', bar, track);
			}
			else if (x == fullCells)
			{
				context.SetCell(x, y, partialBlocks[partialIndex], bar, track);
			}
			else
			{
				context.SetCell(x, y, L' ', track, track);
			}
		}
	}
}
// --- End Source: Controls/Visual/ProgressBar.cpp ---

// --- Begin Source: Controls/Visual/Spinner.cpp ---

// #include <string> (Moved to top)
// #include <cstdint> (Moved to top)
// #include <vector> (Moved to top)
// #include <functional> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

Spinner::Spinner()
{
    SetFocusable(false);
    MinSize = Size(1, 1);
    MaxSize = Size(1, 1);

    tickConnection_ = DispatchTimer::Current().TickEvent.Connect([this](float dt)
    {
        accumulator_ += dt;
        if (accumulator_ >= 0.1f)
        {
            accumulator_ -= 0.1f;
            frame_ = (frame_ + 1) % 4;
            InvalidateVisual();
        }
    });
}

Size Spinner::MeasureOverride(const Size& availableSize)
{
    const std::wstring* maxStr = nullptr;
    for (auto& frame : Frames.Get())
    {
        if (maxStr == nullptr)
        {
            maxStr = &frame;
            continue;
        }

        if (frame.size() > maxStr->size())
        {
            maxStr = &frame;
            continue;
        }
    }

    return Size(maxStr == nullptr ? 0 : static_cast<int32_t>(maxStr->size()), 1);
}

void Spinner::ArrangeOverride(const Rect& finalRect)
{
    return;
}

void Spinner::RenderOverride(RenderContext& context)
{
    std::vector<std::wstring> frames = Frames.Get();
    context.RenderText(Point::Zero, frames.at(frame_), ForegroundColor, BackgroundColor);
}
// --- End Source: Controls/Visual/Spinner.cpp ---

// --- Begin Source: Core/Geometry.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

const Point Point::Zero = { 0, 0 };

bool Point::operator==(const Point& other) const
{
	return X == other.X && Y == other.Y;
}

bool Point::operator!=(const Point& other) const
{
	return !(*this == other);
}

bool Vector::operator==(const Vector& other) const
{
	return From == other.From && To == other.To;
}

bool Vector::operator!=(const Vector& other) const
{
	return !(*this == other);
}

const Size Size::Zero = { 0, 0 };
const Size Size::Auto = { -1, -1 };

bool Size::operator==(const Size& other) const
{
	return Width == other.Width && Height == other.Height;
}

bool Size::operator!=(const Size& other) const
{
	return !(*this == other);
}

const Thickness Thickness::Zero = { 0, 0, 0, 0 };
const Thickness Thickness::Single = { 1, 1, 1, 1 };

bool Thickness::operator==(const Thickness& other) const
{
	return Left == other.Left && Top == other.Top && Right == other.Right && Bottom == other.Bottom;
}

bool Thickness::operator!=(const Thickness& other) const
{
	return !(*this == other);
}

bool Thickness::IsUniform() const
{
	return Left == Top && Left == Right && Left == Bottom;
}

bool Rect::operator==(const Rect& other) const
{
	return X == other.X && Y == other.Y && Width == other.Width && Height == other.Height;
}

bool Rect::operator!=(const Rect& other) const
{
	return !(*this == other);
}

int32_t Rect::Right() const
{
	return X + Width;
}

int32_t Rect::Bottom() const
{
	return Y + Height;
}

Size Rect::AsSize() const
{
	return terminality::Size(Width, Height);
}

bool Rect::Contains(Point point) const
{
	return (point.X >= X)
		&& (point.X < X + Width)
		&& (point.Y >= Y)
		&& (point.Y < Y + Height);
}

bool Rect::Contains(Rect inner) const
{
	return (X <= inner.X)
		&& (Y <= inner.Y)
		&& ((X + Width) >= (inner.X + inner.Width))
		&& ((Y + Height) >= (inner.Y + inner.Height));
}

bool Rect::Intersects(Rect other) const
{
	return !(other.X >= X + Width
		|| other.X + other.Width <= X
		|| other.Y >= Y + Height
		|| other.Y + other.Height <= Y);
}

bool Rect::IsEmpty() const
{
	return Width <= 0 || Height <= 0;
}

Rect Rect::Union(const Rect& a, const Rect& b)
{
	if (a.IsEmpty())
		return b;

	if (b.IsEmpty())
		return a;

	const int32_t left = std::min(a.X, b.X);
	const int32_t top = std::min(a.Y, b.Y);

	const int32_t right = std::max(a.Right(), b.Right());
	const int32_t bottom = std::max(a.Bottom(), b.Bottom());

	return Rect(left, top, right - left, bottom - top);
}

Rect Rect::Enclose(const Rect& into, const Rect& rect)
{
	const int32_t absLeft = std::max(into.X, into.X + rect.X);
	const int32_t absTop = std::max(into.Y, into.Y + rect.Y);

	const int32_t rectRight = into.X + rect.X + rect.Width;
	const int32_t rectBottom = into.Y + rect.Y + rect.Height;

	const int32_t absRight = std::min(into.X + into.Width, rectRight);
	const int32_t absBottom = std::min(into.Y + into.Height, rectBottom);

	const int32_t finalWidth = std::max(0, absRight - absLeft);
	const int32_t finalHeight = std::max(0, absBottom - absTop);

	return Rect(absLeft, absTop, finalWidth, finalHeight);
}

Rect Rect::Clip(const Rect& into, const Rect& rect)
{
	const int32_t left = std::max(into.X, rect.X);
	const int32_t top = std::max(into.Y, rect.Y);

	const int32_t right = std::min(into.X + into.Width, rect.X + rect.Width);
	const int32_t bottom = std::min(into.Y + into.Height, rect.Y + rect.Height);

	const int32_t width = std::max(0, right - left);
	const int32_t height = std::max(0, bottom - top);

	return Rect(left, top, width, height);
}
// --- End Source: Core/Geometry.cpp ---

// --- Begin Source: Core/TextHelper.cpp ---

// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <vector> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

std::vector<LineBounds> TextHelper::CalculateLineBounds(const std::wstring& text, int32_t availableWidth, TextWrap wrapping)
{
    std::vector<LineBounds> bounds;
    if (text.empty())
    {
        bounds.push_back({ 0, 0, 0 });
        return bounds;
    }

    const size_t len = text.length();
    size_t start = 0;

    while (start < len)
    {
        size_t end = start;
        int32_t width = 0;
        size_t lastSpace = std::wstring::npos;

        if (wrapping == TextWrap::NoWrap || availableWidth <= 0)
        {
            while (end < len && text[end] != L'\n')
                end++;

            bounds.push_back({ start, end, end < len ? end + 1 : len });
            start = end < len ? end + 1 : len;
            continue;
        }

        while (end < len && text[end] != L'\n' && width < availableWidth)
        {
            if (text[end] == L' ')
                lastSpace = end;

            end++;
            width++;
        }

        if (end == len || text[end] == L'\n')
        {
            bounds.push_back({ start, end, end < len ? end + 1 : len });
            start = end < len ? end + 1 : len;
            continue;
        }

        if (wrapping == TextWrap::WrapWholeWords)
        {
            if (end < len && (text[end] == L' ' || text[end] == L'\n'))
            {
                bounds.push_back({ start, end, text[end] == L'\n' ? end + 1 : end + 1 });
                start = end + 1;
                continue;
            }

            if (lastSpace != std::wstring::npos && lastSpace >= start)
            {
                bounds.push_back({ start, lastSpace, lastSpace + 1 });
                start = lastSpace + 1;
                continue;
            }
        }

        bounds.push_back({ start, end, end });
        start = end;
    }

    if (!text.empty() && text.back() == L'\n')
        bounds.push_back({ len, len, len });

    return bounds;
}

std::vector<LineInfo> TextHelper::GetLines(const std::wstring& text, int32_t availableWidth, TextWrap wrapping)
{
    std::vector<LineInfo> lines;
    auto bounds = CalculateLineBounds(text, availableWidth, wrapping);

    lines.reserve(bounds.size());
    for (const auto& b : bounds)
    {
        lines.push_back({ text.substr(b.Start, b.End - b.Start), b.Start });
    }

    return lines;
}

std::vector<int32_t> TextHelper::MeasureLines(const std::wstring& text, int32_t availableWidth, TextWrap wrapping)
{
    std::vector<int32_t> lineLengths;
    auto bounds = CalculateLineBounds(text, availableWidth, wrapping);

    lineLengths.reserve(bounds.size());
    for (const auto& b : bounds)
    {
        lineLengths.push_back(static_cast<int32_t>(b.End - b.Start));
    }

    return lineLengths;
}
// --- End Source: Core/TextHelper.cpp ---

// --- Begin Source: Dialogs/ContextMenu.cpp ---

// #include <functional> (Moved to top)
// #include <string> (Moved to top)
// #include <cstdint> (Moved to top)
// #include <atomic> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

void ContextMenu::AddItem(const std::wstring& text, std::function<void()> action)
{
	items_.push_back({ text, std::move(action) });
}

void ContextMenu::Clear()
{
	items_.clear();
}

void ContextMenu::Open(Point position)
{
	HostApplication& host = HostApplication::Current();
	VisualTree& tree = VisualTree::Current();

	std::atomic<bool>* running = nullptr;

	if (items_.empty())
		return;

	auto ctxMenuBody = init<Border>([&](Border* ctxMenuBody)
	{
		ctxMenuBody->HorizontalAlignment = HorizontalAlign::Left;
		ctxMenuBody->VerticalAlignment = VerticalAlign::Top;
		ctxMenuBody->Margin = Thickness(position.X, position.Y, 0, 0);
		ctxMenuBody->BorderColor = Color::YELLOW;
		ctxMenuBody->FocusedBorderColor = Color::YELLOW;

		ctxMenuBody->Content = init<StackPanel>([&](StackPanel* panel)
		{
			panel->HorizontalAlignment = HorizontalAlign::Stretch;
			panel->VerticalAlignment = VerticalAlign::Stretch;

			for (const ContextMenuItem& item : items_)
			{
				panel->AddChild(init<Button>([&](Button* itemButton)
				{
					itemButton->Text = item.Text;
					itemButton->HorizontalAlignment = HorizontalAlign::Stretch;
					itemButton->Clicked += [&running, item]()
					{
						if (item.Action != nullptr)
							item.Action();

						running->store(false);
					};
				}));
			}
		});
	});

	UILayer& layer = tree.PushLayer(std::move(ctxMenuBody));
	running = &layer.Running;

	host.NestUILoop(layer);
	tree.PopLayer();
}
// --- End Source: Dialogs/ContextMenu.cpp ---

// --- Begin Source: Dialogs/MessageBox.cpp ---

// #include <string> (Moved to top)
// #include <memory> (Moved to top)
// #include <functional> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

MessageBoxResult MessageBox::Show(const std::wstring& title, const std::wstring& message, MessageBoxButton buttons)
{
    MessageBoxResult result = MessageBoxResult::None;

    auto rootGrid = init<Grid>([&](Grid* rootGrid)
    {
        rootGrid->HorizontalAlignment = HorizontalAlign::Stretch;
        rootGrid->VerticalAlignment = VerticalAlign::Stretch;

        rootGrid->AddChild(0, 0, init<Border>([&](Border* dialogBorder)
        {
            dialogBorder->HorizontalAlignment = HorizontalAlign::Center;
            dialogBorder->VerticalAlignment = VerticalAlign::Center;

            if (!title.empty())
            {
                dialogBorder->HeaderText = title;
            }

            dialogBorder->Content = init<StackPanel>([&](StackPanel* dialogContent)
            {
                dialogContent->HorizontalAlignment = HorizontalAlign::Stretch;
                dialogContent->VerticalAlignment = VerticalAlign::Top;
                dialogContent->Margin = Thickness(2, 1, 2, 0);

                dialogContent->AddChild(init<Label>([&](Label* messageBox)
                {
                    messageBox->Text = message;
                    messageBox->SetFocusable(false);
                    messageBox->SetTabStop(false);
                    messageBox->TextWrapping = TextWrap::WrapWholeWords;
                }));

                dialogContent->AddChild(init<Grid>([&](Grid* buttonGrid)
                {
                    buttonGrid->Margin = Thickness(0, 1, 0, 0);
                    buttonGrid->HorizontalAlignment = HorizontalAlign::Right;

                    int colIndex = 0;
                    auto addButton = [&](const std::wstring& text, MessageBoxResult res)
                    {
                        buttonGrid->AddColumn(ColumnDefinition{ GridLength::Auto() });
                        buttonGrid->AddChild(0, colIndex++, init<Button>([&](Button* btn)
                        {
                            btn->Text = text;
                            btn->Clicked += [btn, res, &result]()
                            {
                                result = res;
                                btn->Close();
                            };
                        }));
                    };

                    switch (buttons)
                    {
                        case MessageBoxButton::YesNoCancel:
                        {
                            addButton(L"Yes", MessageBoxResult::Yes);
                            addButton(L"No", MessageBoxResult::No);
                            addButton(L"Cancel", MessageBoxResult::Cancel);
                            break;
                        }

                        case MessageBoxButton::YesNo:
                        {
                            addButton(L"Yes", MessageBoxResult::Yes);
                            addButton(L"No", MessageBoxResult::No);
                            break;
                        }

                        case MessageBoxButton::OkCancel:
                        {
                            addButton(L"OK", MessageBoxResult::Ok);
                            addButton(L"Cancel", MessageBoxResult::Cancel);
                            break;
                        }

                        case MessageBoxButton::Ok:
                        {
                            addButton(L"OK", MessageBoxResult::Ok);
                            break;
                        }
                    }
                }));
            });
        }));
    });

    Navigator::Current().Navigate(std::move(rootGrid));
    return result;
}
// --- End Source: Dialogs/MessageBox.cpp ---

// --- Begin Source: Dialogs/OpenFileDialog.cpp ---

// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)
// #include <string> (Moved to top)
// #include <functional> (Moved to top)
// #include <filesystem> (Moved to top)
// #include <vector> (Moved to top)
// #include <algorithm> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

struct FileSystemEntry
{
    std::wstring Name;
    bool IsDirectory;
    bool IsParentDir;
};

class FileSystemEntryView : public Button
{
public:
    FileSystemEntry Model;

    FileSystemEntryView()
    {
        HorizontalAlignment = HorizontalAlign::Stretch;
    }

    Size MeasureOverride(const Size& availableSize) override
    {
        return Size(static_cast<int>(Model.Name.size()) + 4, 1);
    }

    void RenderOverride(RenderContext& context) override
    {
        auto rin = context.BeginText();

        // Используем состояние focused_, которое менеджит Button/ControlBase
        if (focused_)
        {
            rin << SetBack(Color::WHITE) << SetFore(Color::BLACK);
        }
        else
        {
            rin << SetBack(Color::BLACK) << SetFore(Color::WHITE);
        }

        rin << L"[ ";
        if (!focused_)
        {
            if (Model.IsParentDir)
                rin << SetFore(Color::YELLOW);
            else if (Model.IsDirectory)
                rin << SetFore(Color::CYAN);
            else
                rin << SetFore(Color::LIGHT_GRAY);
        }

        rin << Model.Name;

        if (!focused_)
            rin << SetFore(Color::WHITE);

        rin << L" ]";
    }
};

std::optional<std::filesystem::path> OpenFileDialog::Show(const std::wstring& title, const std::filesystem::path& initialDireinity)
{
    // WIP
    //*
    HostApplication& host = HostApplication::Current();
    VisualTree& tree = VisualTree::Current();

    std::optional<std::filesystem::path> result = std::nullopt;
	std::atomic<bool>* running = nullptr;

    std::filesystem::path currentDir = initialDireinity.empty()
        ? std::filesystem::current_path()
        : initialDireinity;

    ObservableCollection<FileSystemEntry> currentFiles;
    TextBox* pathBoxPtr = nullptr;

    auto loadDirectory = [&]()
    {
        currentFiles.clear();

        if (pathBoxPtr != nullptr)
            pathBoxPtr->Text = currentDir.wstring();

        // Добавляем ".."
        if (currentDir.has_parent_path() && currentDir != currentDir.parent_path())
        {
            currentFiles.push_back(FileSystemEntry{ L"..", true, true });
        }

        try
        {
            std::vector<FileSystemEntry> dirs;
            std::vector<FileSystemEntry> files;

            for (const auto& entry : std::filesystem::directory_iterator(currentDir))
            {
                if (entry.is_directory())
                    dirs.push_back(FileSystemEntry{ entry.path().filename().wstring(), true, false });
                else
                    files.push_back(FileSystemEntry{ entry.path().filename().wstring(), false, false });
            }

            auto sortAlphabetically = [](const FileSystemEntry& a, const FileSystemEntry& b)
            {
                return a.Name < b.Name;
            };

            std::sort(dirs.begin(), dirs.end(), sortAlphabetically);
            std::sort(files.begin(), files.end(), sortAlphabetically);

            for (const auto& d : dirs)
                currentFiles.push_back(d);

            for (const auto& f : files)
                currentFiles.push_back(f);
        }
        catch (...)
        {

        }
    };

    auto rootGrid = init<Grid>([&](Grid* root)
    {
        root->HorizontalAlignment = HorizontalAlign::Stretch;
        root->VerticalAlignment = VerticalAlign::Stretch;

        root->AddChild(0, 0, init<Border>([&](Border* dialogBorder)
        {
            dialogBorder->HorizontalAlignment = HorizontalAlign::Center;
            dialogBorder->VerticalAlignment = VerticalAlign::Center;
            dialogBorder->BorderColor = Color::WHITE;
            dialogBorder->MaxSize = Size(60, 20);
            dialogBorder->MinSize = Size(40, 10);

            dialogBorder->Content = init<Grid>([&](Grid* dialogContent)
            {
                dialogContent->AddRow(RowDefinition{ GridLength::Auto() });
                dialogContent->AddRow(RowDefinition{ GridLength::Star(1.0f) });
                dialogContent->AddRow(RowDefinition{ GridLength::Auto() });

                dialogContent->AddChild(0, 0, init<TextBox>([&](TextBox* pathBox)
                {
                    pathBoxPtr = pathBox;
                    pathBox->Text = currentDir.wstring();
                    pathBox->SetFocusable(false);
                    pathBox->SetTabStop(false);
                    pathBox->HorizontalAlignment = HorizontalAlign::Stretch;
                }));

                dialogContent->AddChild(1, 0, init<ItemsControl<FileSystemEntry>>([&](ItemsControl<FileSystemEntry>* items)
                {
                    items->HorizontalAlignment = HorizontalAlign::Stretch;
                    items->VerticalAlignment = VerticalAlign::Stretch;
                    items->SetItemsSource(&currentFiles);

                    items->SetItemTemplate([&](const FileSystemEntry& entryModel) -> std::unique_ptr<ControlBase>
                    {
                        return init<FileSystemEntryView>([&, entryModel](FileSystemEntryView* view)
                        {
                            view->Model = entryModel;
                            view->HorizontalAlignment = HorizontalAlign::Stretch;

                            view->Clicked += [&currentDir, &result, &running, &loadDirectory, entryModel]()
                            {
                                if (entryModel.IsParentDir)
                                {
                                    currentDir = currentDir.parent_path();
                                    loadDirectory();
                                }
                                else if (entryModel.IsDirectory)
                                {
                                    currentDir /= entryModel.Name;
                                    loadDirectory();
                                }
                                else
                                {
                                    result = currentDir / entryModel.Name;
                                    running->store(false);
                                }
                            };
                        });
                    });
                }));

                dialogContent->AddChild(2, 0, init<Button>([&](Button* cancelBtn)
                {
                    cancelBtn->Text = L"Cancel";
                    cancelBtn->HorizontalAlignment = HorizontalAlign::Right;
                    cancelBtn->Clicked += [&running]()
                    {
                        running->store(false);
                    };
                }));
            });
        }));
    });

    loadDirectory();
    UILayer& layer = tree.PushLayer(std::move(rootGrid));
    running = &layer.Running;

    host.NestUILoop(layer);
    tree.PopLayer();
    return result;
    //*/

    return std::nullopt;
}
// --- End Source: Dialogs/OpenFileDialog.cpp ---

// --- Begin Source: Engine/DispatchTimer.cpp ---

// #include <cstdint> (Moved to top)
// #include <chrono> (Moved to top)
// #include <stdexcept> (Moved to top)
// #include <thread> (Moved to top)
// #include <mutex> (Moved to top)
// #include <functional> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

DispatchTimer& DispatchTimer::Current()
{
	static DispatchTimer dispatcher;
	return dispatcher;
}

void DispatchTimer::SetUIThread()
{
	uiThreadId_ = std::this_thread::get_id();
}

bool DispatchTimer::CheckAccess() const
{
	return std::this_thread::get_id() == uiThreadId_;
}

void DispatchTimer::VerifyAccess() const
{
	if (!CheckAccess())
	{
		throw std::logic_error("Invalid cross-thread operation. You must use DispatchTimer::InvokeAsync to access this object.");
	}
}

void DispatchTimer::InvokeAsync(std::function<void()> task)
{
	if (!task)
		return;

	std::lock_guard<std::mutex> lock(mutex_);
	tasks_.push_back(std::move(task));
}

void DispatchTimer::ProcessTasks()
{
	std::vector<std::function<void()>> currentTasks;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (tasks_.empty())
			return;

		currentTasks = std::move(tasks_);
	}

	for (const auto& task : currentTasks)
	{
		try
		{
			task();
		}
		catch (const std::exception& e)
		{
			// ...
		}
	}
}

bool DispatchTimer::IsRunning() const
{
	return running_.load();
}

bool DispatchTimer::IsResizing() const
{
	return isResizing_;
}

void DispatchTimer::Start()
{
	running_.store(true);
	lastTime_ = std::chrono::high_resolution_clock::now();
}

void DispatchTimer::Stop()
{
	running_.store(false);
}

void DispatchTimer::Tick()
{
	if (!running_.load())
		return;

	frameStart_ = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> dt = frameStart_ - lastTime_;
	lastTime_ = frameStart_;

	deltaTime_ = dt.count();
	totalTime_ += deltaTime_;

	TickEvent.Emit(deltaTime_);

	if (isResizing_)
	{
		resizeDebounceTimer_ -= deltaTime_;
		if (resizeDebounceTimer_ <= 0.0f)
		{
			isResizing_ = false;
			ResizeFinishedEvent.Emit();
		}
	}
}

void DispatchTimer::BeginResize()
{
	isResizing_ = true;
	resizeDebounceTimer_ = RESIZE_DELAY;
}

std::chrono::milliseconds DispatchTimer::GetRemainingFrameTime(int targetFPS)
{
	const auto targetFrameTime = std::chrono::milliseconds(1000 / targetFPS);
	auto workTime = std::chrono::high_resolution_clock::now() - frameStart_;
	auto waitTime = targetFrameTime - std::chrono::duration_cast<std::chrono::milliseconds>(workTime);

	if (waitTime.count() < 0)
		return std::chrono::milliseconds(0);

	return waitTime;
}
// --- End Source: Engine/DispatchTimer.cpp ---

// --- Begin Source: Engine/FocusManager.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <stdexcept> (Moved to top)
// #include <stack> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

FocusManager& FocusManager::Current()
{
	if (!DispatchTimer::Current().CheckAccess())
		throw std::runtime_error("Cannot get FocusManager within running UI thread or Before UI thread was started.");

	return VisualTree::Current().GetFocusManager();
}

VisualTreeNode* FocusManager::GetFocused() const
{
	if (focusStack.empty())
		return nullptr;

	return focusStack.back();
}

bool FocusManager::SetFocused(VisualTreeNode* node)
{
	if (node == nullptr || !node->IsFocusable())
		return false;

	VisualTreeNode* old = nullptr;
	if (!focusStack.empty())
	{
		old = focusStack.back();
		if (old == node)
			return false;
	}

	VisualTreeNode* parent = node->GetParent();
	if (old == parent)
	{
		focusStack.push_back(node);
	}
	else
	{
		focusStack.clear();
		while (parent != nullptr)
		{
			parent->OnGotFocus();
			parent = parent->GetParent();
		}
	}

	FocusChanged.Emit(old, node);
	node->OnGotFocus();
	return true;
}

bool FocusManager::MoveNext(Direction direction, InputModifier modifiers)
{
	if (focusStack.empty())
		return false;

	std::vector<VisualTreeNode*> focusStackCopy = focusStack;
	for (auto i = focusStackCopy.rbegin(); i != focusStackCopy.rend(); i++)
	{
		// Check if node can move focus
		VisualTreeNode* searchNode = *i;
		if (!searchNode->MoveFocusNext(direction, modifiers))
		{
			// Cannot move, continue
			focusStack.pop_back();
			continue;
		}

		// Can move, disfocussing all popped nodes
		for (auto i = focusStackCopy.rbegin(); i != focusStackCopy.rend(); i++)
		{
			VisualTreeNode* disfocusNode = *i;
			if (disfocusNode == searchNode)
				break; // Cutting edge

			disfocusNode->OnLostFocus();
		}

		// Success
		return true;
	}

	focusStack = focusStackCopy;
	return false;
}

void FocusManager::ClearFocus(VisualTreeNode* node)
{
	if (focusStack.empty() || node == nullptr)
		return;

	auto it = std::find(focusStack.begin(), focusStack.end(), node);
	if (it != focusStack.end())
	{
		VisualTreeNode* oldFocused = focusStack.back();
		for (auto& dropIt = it; dropIt != focusStack.end(); ++dropIt)
			(*dropIt)->OnLostFocus();

		focusStack.erase(it, focusStack.end());
		VisualTreeNode* newFocused = focusStack.empty() ? nullptr : focusStack.back();

		if (oldFocused != newFocused)
			FocusChanged.Emit(oldFocused, newFocused);
	}
}
// --- End Source: Engine/FocusManager.cpp ---

// --- Begin Source: Engine/Navigator.cpp ---

// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)
// #include <functional> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

Navigator& Navigator::Current()
{
	static Navigator navigator;
	return navigator;
}

void Navigator::Navigate(std::unique_ptr<VisualTreeNode> page)
{
	if (!page)
		return;

	HostApplication& host = HostApplication::Current();
	VisualTree& tree = VisualTree::Current();

	UILayer& layer = tree.PushLayer(std::move(page));
	host.NestUILoop(layer);

	if (tree.Root())
	{
		tree.Root()->InvalidateMeasure();
		tree.Root()->InvalidateVisual();
	}

	tree.PopLayer();
}

bool Navigator::CanGoBack() const
{
	return VisualTree::Current().LayerCount() > 1;
}

bool Navigator::GoBack()
{
	if (!CanGoBack())
		return false;

	VisualTree& tree = VisualTree::Current();
	tree.PopLayer();

	if (tree.PeekLayer() != nullptr)
	{
		tree.PeekLayer()->InvalidateMeasure();
		tree.PeekLayer()->InvalidateVisual();
	}

	return true;
}

void Navigator::GoHome()
{
	VisualTree& tree = VisualTree::Current();
	while (tree.LayerCount() > 1)
		tree.PopLayer();

	if (tree.Root())
	{
		tree.Root()->InvalidateMeasure();
		tree.Root()->InvalidateVisual();
	}
}
// --- End Source: Engine/Navigator.cpp ---

// --- Begin Source: Engine/RenderBuffer.cpp ---

// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <iostream> (Moved to top)
// #include <optional> (Moved to top)
// #include <stdexcept> (Moved to top)
// #include <mutex> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

const wchar_t* RenderBuffer::GetAsniBg(Color color)
{
	switch (color)
	{
		// default
		case Color::BLACK:          return L"\x1b[40m";
		case Color::DARK_RED:       return L"\x1b[41m";
		case Color::DARK_GREEN:     return L"\x1b[42m";
		case Color::DARK_YELLOW:    return L"\x1b[43m";
		case Color::DARK_BLUE:      return L"\x1b[44m";
		case Color::DARK_MAGENTA:   return L"\x1b[45m";
		case Color::DARK_CYAN:      return L"\x1b[46m";
		case Color::LIGHT_GRAY:     return L"\x1b[47m";

		// light
		case Color::DARK_GRAY:      return L"\x1b[100m";
		case Color::RED:            return L"\x1b[101m";
		case Color::GREEN:          return L"\x1b[102m";
		case Color::YELLOW:         return L"\x1b[103m";
		case Color::BLUE:           return L"\x1b[104m";
		case Color::MAGENTA:        return L"\x1b[105m";
		case Color::CYAN:           return L"\x1b[106m";
		case Color::WHITE:          return L"\x1b[107m";

		case Color::TRANSPARENT:    return L"";
		default:                    return L"\x1b[40m";
	}
}

const wchar_t* RenderBuffer::GetAsniFg(Color color)
{
	switch (color)
	{
		// default
		case Color::BLACK:          return L"\x1b[30m";
		case Color::DARK_RED:       return L"\x1b[31m";
		case Color::DARK_GREEN:     return L"\x1b[32m";
		case Color::DARK_YELLOW:    return L"\x1b[33m";
		case Color::DARK_BLUE:      return L"\x1b[34m";
		case Color::DARK_MAGENTA:   return L"\x1b[35m";
		case Color::DARK_CYAN:      return L"\x1b[36m";
		case Color::LIGHT_GRAY:     return L"\x1b[37m";

		// light
		case Color::DARK_GRAY:      return L"\x1b[90m";
		case Color::RED:            return L"\x1b[91m";
		case Color::GREEN:          return L"\x1b[92m";
		case Color::YELLOW:         return L"\x1b[93m";
		case Color::BLUE:           return L"\x1b[94m";
		case Color::MAGENTA:        return L"\x1b[95m";
		case Color::CYAN:           return L"\x1b[96m";
		case Color::WHITE:          return L"\x1b[97m";

		case Color::TRANSPARENT:    return L"";
		default:                    return L"\x1b[37m";
	}
}

RenderBuffer::RenderBuffer(uint32_t initialWidth, uint32_t initialHeight)
{
	std::lock_guard<std::recursive_mutex> guard(renderMutex);

	buffer.assign(MAX_WIDTH * MAX_HEIGHT, CellInfo(' ', static_cast<Color>(-1), static_cast<Color>(-1)));
	snapshotBuffer.assign(MAX_WIDTH * MAX_HEIGHT, CellInfo(' ', static_cast<Color>(-1), static_cast<Color>(-1)));

	width = std::min(initialWidth, static_cast<uint32_t>(MAX_WIDTH));
	height = std::min(initialHeight, static_cast<uint32_t>(MAX_HEIGHT));

	snapshotWidth = width;
	snapshotHeight = height;
	dirtyRect = Rect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
}

void RenderBuffer::Resize(uint32_t newWidth, uint32_t newHeight)
{
	std::lock_guard<std::recursive_mutex> guard(renderMutex);

	width = std::min(newWidth, static_cast<uint32_t>(MAX_WIDTH));
	height = std::min(newHeight, static_cast<uint32_t>(MAX_HEIGHT));

	dirtyRect = Rect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
	hasDirtyRect = true;
}

void RenderBuffer::Clear(const CellInfo& cell)
{
	std::lock_guard<std::recursive_mutex> guard(renderMutex);
	std::fill(buffer.begin(), buffer.end(), cell);
	MarkDirty(Rect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height)));
}

void RenderBuffer::SetCell(uint32_t x, uint32_t y, const CellInfo& cell)
{
	std::lock_guard<std::recursive_mutex> guard(renderMutex);
	if (x >= width || y >= height)
		return;

	CellInfo& target = buffer[GetIndex(x, y)];
	if (target != cell)
	{
		if (cell.Back != Color::TRANSPARENT)
			target.Back = cell.Back;

		if (cell.Fore != Color::TRANSPARENT)
			target.Fore = cell.Fore;

		if (cell.Symbol != L'\0')
			target.Symbol = cell.Symbol;

		//target = cell;
		MarkDirty(Rect(static_cast<int32_t>(x), static_cast<int32_t>(y), 1, 1));
	}
}

const CellInfo& RenderBuffer::GetCell(uint32_t x, uint32_t y) const
{
	if (x >= width || y >= height)
		throw std::out_of_range("RenderBuffer::GetCell coordinates out of range");

	return buffer.at(GetIndex(x, y));
}

void RenderBuffer::Snapshot()
{
	std::lock_guard<std::recursive_mutex> guard(renderMutex);
	snapshotBuffer = buffer;
	snapshotWidth = width;
	snapshotHeight = height;
	hasDirtyRect = false;
	dirtyRect = Rect();
}

void RenderBuffer::BulkRender(std::wostream& out)
{
	std::lock_guard<std::recursive_mutex> guard(renderMutex);

	if (!hasDirtyRect)
		return;

	std::wstring output;
	output.reserve(width * height * 24);

	output += L"\x1b[H";
	std::optional<Color> currentFore;
	std::optional<Color> currentBack;

	for (uint32_t y = 0; y < height; ++y)
	{
		output += L"\x1b[";
		output += std::to_wstring(y + 1);
		output += L";1H";

		for (uint32_t x = 0; x < width; ++x)
		{
			const size_t idx = GetIndex(x, y);
			const CellInfo& cell = buffer[idx];

			if (!currentFore.has_value() || *currentFore != cell.Fore)
			{
				output += GetAsniFg(cell.Fore);
				currentFore = cell.Fore;
			}

			if (!currentBack.has_value() || *currentBack != cell.Back)
			{
				output += GetAsniBg(cell.Back);
				currentBack = cell.Back;
			}

			output += cell.Symbol;
		}

		if (y < height - 1)
			output += L"\r\n";
	}

	out.write(output.data(), output.size());
	out.flush();

	Snapshot();
	hasDirtyRect = false;
	dirtyRect = Rect();
}

void RenderBuffer::DiffRender(std::wostream& out)
{
	std::lock_guard<std::recursive_mutex> guard(renderMutex);
	if (snapshotWidth != width || snapshotHeight != height)
	{
		BulkRender(out);
		return;
	}

	if (!hasDirtyRect)
		return;

	const uint32_t startX = (hasDirtyRect) ? static_cast<uint32_t>(std::max(0, dirtyRect.X)) : 0U;
	const uint32_t startY = (hasDirtyRect) ? static_cast<uint32_t>(std::max(0, dirtyRect.Y)) : 0U;
	const uint32_t endX = (hasDirtyRect) ? static_cast<uint32_t>(std::min<int32_t>(width, dirtyRect.Right())) : width;
	const uint32_t endY = (hasDirtyRect) ? static_cast<uint32_t>(std::min<int32_t>(height, dirtyRect.Bottom())) : height;

	std::wstring output;
	output.reserve(8192);

	std::optional<Color> currentFore;
	std::optional<Color> currentBack;
	uint32_t expectedX = static_cast<uint32_t>(-1);
	uint32_t expectedY = static_cast<uint32_t>(-1);

	for (uint32_t y = startY; y < endY; ++y)
	{
		for (uint32_t x = startX; x < endX; ++x)
		{
			const size_t idx = GetIndex(x, y);
			const CellInfo& cell = buffer[idx];
			if (cell == snapshotBuffer[idx])
				continue;

			if (expectedX != x || expectedY != y)
			{
				output += L"\x1b[";
				output += std::to_wstring(y + 1);
				output += L";";
				output += std::to_wstring(x + 1);
				output += L"H";
			}

			if (!currentFore.has_value() || *currentFore != cell.Fore)
			{
				output += GetAsniFg(cell.Fore);
				currentFore = cell.Fore;
			}

			if (!currentBack.has_value() || *currentBack != cell.Back)
			{
				output += GetAsniBg(cell.Back);
				currentBack = cell.Back;
			}

			output += cell.Symbol;

			expectedX = x + 1;
			expectedY = y;
		}
	}

	out << std::nounitbuf;
	out.write(output.data(), output.size());
	out.flush();

	Snapshot();
	hasDirtyRect = false;
	dirtyRect = Rect();
}

size_t RenderBuffer::GetIndex(uint32_t x, uint32_t y) const
{
	return static_cast<std::size_t>(y) * MAX_WIDTH + x;
}

void RenderBuffer::MarkDirty(const Rect& rect)
{
	if (!hasDirtyRect)
	{
		dirtyRect = rect;
		hasDirtyRect = true;
		return;
	}

	dirtyRect = Rect::Union(dirtyRect, rect);
}
// --- End Source: Engine/RenderBuffer.cpp ---

// --- Begin Source: Engine/RenderContext.cpp ---

// #include <cstdint> (Moved to top)
// #include <mutex> (Moved to top)
// #include <algorithm> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

RenderContext::RenderContext(RenderBuffer& buffer, Rect targetRect)
    : buffer_(buffer), rect_(targetRect) {}

RenderContext RenderContext::CreateInner(Rect targetRect)
{
	int32_t x = targetRect.X;
	int32_t y = targetRect.Y;

	int32_t right = std::clamp<int32_t>(x + targetRect.Width, rect_.X, rect_.Right());
    int32_t bottom = std::clamp<int32_t>(y + targetRect.Height, rect_.Y, rect_.Bottom());

	int32_t width = std::max(0, right - x);
	int32_t height = std::max(0, bottom - y);

	return RenderContext(buffer_, Rect(x, y, width, height));
}

Rect RenderContext::ContextRect()
{
    return rect_;
}

void RenderContext::SetCell(uint32_t x, uint32_t y, const CellInfo& cell)
{
    if (x >= static_cast<uint32_t>(rect_.Width))
        return;

    if (y >= static_cast<uint32_t>(rect_.Height))
        return;

    int32_t absX = rect_.X + x;
    int32_t absY = rect_.Y + y;

    buffer_.SetCell(absX, absY, cell);
    buffer_.MarkDirty(Rect(absX, absY, 1, 1));
}

void RenderContext::SetCell(uint32_t x, uint32_t y, const wchar_t puts, Color fg, Color bg)
{
    if (x >= static_cast<uint32_t>(rect_.Width))
        return;

    if (y >= static_cast<uint32_t>(rect_.Height))
        return;

    int32_t absX = rect_.X + x;
    int32_t absY = rect_.Y + y;

    buffer_.SetCell(absX, absY, CellInfo{ puts, fg, bg });
    buffer_.MarkDirty(Rect(absX, absY, 1, 1));
}

const CellInfo& RenderContext::GetCell(uint32_t x, uint32_t y) const
{
    static CellInfo empty;
    if (x >= static_cast<uint32_t>(rect_.Width))
        return empty;

    if (y >= static_cast<uint32_t>(rect_.Height))
        return empty;

    int32_t absX = rect_.X + x;
    int32_t absY = rect_.Y + y;

    return buffer_.GetCell(absX, absY);
}

void RenderContext::RenderRaw(const Point& point, const std::string& rawData)
{
    std::lock_guard<std::recursive_mutex> guard(buffer_.renderMutex);

    uint32_t absX = rect_.X + point.X;
    uint32_t absY = rect_.Y + point.Y;

    for (int32_t i = 0; i < static_cast<int32_t>(rawData.size()); ++i)
    {
        if (point.X + i < rect_.Width)
        {
            buffer_.SetCell(absX + i, absY, CellInfo{ (wchar_t)rawData[i] });
        }
    }
}

RenderStream RenderContext::BeginText(Point startPos)
{
    return RenderStream(*this, startPos);
}

void RenderContext::RenderText(const Point& point, const std::wstring& text, Color fg, Color bg, bool wrap)
{
    if (text.empty() || point.Y >= rect_.Height || point.X >= rect_.Width) return;

    std::lock_guard<std::recursive_mutex> guard(buffer_.renderMutex);

    uint32_t x = point.X;
    uint32_t y = point.Y;

    for (wchar_t ch : text)
    {
        if (x >= static_cast<uint32_t>(rect_.Width))
        {
            if (!wrap)
                break;

            x = 0;
            y++;
        }

        if (y >= static_cast<uint32_t>(rect_.Height))
            break;

        buffer_.SetCell(rect_.X + x, rect_.Y + y, CellInfo{ ch, fg, bg });
        x++;
    }

    buffer_.MarkDirty(Rect(rect_.X + point.X, rect_.Y + point.Y, (int32_t)text.size(), 1));
}

void RenderContext::RenderText(const Point& point, const std::string& text, Color fg, Color bg, bool wrap)
{
    std::wstring wtext(text.begin(), text.end());
    RenderText(point, wtext, fg, bg, wrap);
}

void RenderContext::RenderText(const Point& point, const char* text, Color fg, Color bg, bool wrap)
{
    RenderText(point, std::string(text), fg, bg, wrap);
}

void RenderContext::RenderText(const Point& point, const wchar_t* text, Color fg, Color bg, bool wrap)
{
    RenderText(point, std::wstring(text), fg, bg, wrap);
}

void RenderContext::RenderRectangle(const Point& point, const Size& size, RectangleStyle style)
{
    std::lock_guard<std::recursive_mutex> guard(buffer_.renderMutex);

    for (int32_t y = 0; y < size.Height; ++y)
    {
        for (int32_t x = 0; x < size.Width; ++x)
        {
            wchar_t symbol = style(Point(x, y), size);
            if (symbol != L'\0')
                SetCell(point.X + x, point.Y + y, CellInfo(symbol));
        }
    }
}

void RenderContext::RenderRectangle(const Point& point, const Size& size, Color fg, Color bg, RectangleStyle style)
{
    std::lock_guard<std::recursive_mutex> guard(buffer_.renderMutex);

    for (int32_t y = 0; y < size.Height; ++y)
    {
        for (int32_t x = 0; x < size.Width; ++x)
        {
            wchar_t symbol = style(Point(x, y), size);
            if (symbol != L'\0')
                SetCell(point.X + x, point.Y + y, CellInfo(symbol, fg, bg));
        }
    }
}

static wchar_t DefaultRectangleStyle(const Point& point, const Size& size)
{
    return L'#';
}

void RenderContext::RenderRectangle(const Point& point, const Size& size)
{
    RenderRectangle(point, size, DefaultRectangleStyle);
}

void RenderContext::RenderRectangle(const Point& point, const Size& size, Color fg, Color bg)
{
    RenderRectangle(point, size, fg, bg, DefaultRectangleStyle);
}

void RenderContext::RenderRectangle(const Point& point, const int32_t width, const int32_t height)
{
    RenderRectangle(point, Size(width, height));
}

void RenderContext::RenderRectangle(const Point& point, const int32_t width, const int32_t height, Color fg, Color bg)
{
    RenderRectangle(point, Size(width, height), fg, bg);
}

void RenderContext::RenderRectangle(const Point& point, const int32_t width, const int32_t height, RectangleStyle style)
{
    RenderRectangle(point, Size(width, height), style);
}

void RenderContext::RenderRectangle(const Point& point, const int32_t width, const int32_t height, Color fg, Color bg, RectangleStyle style)
{
    RenderRectangle(point, Size(width, height), fg, bg, style);
}

void RenderContext::RenderLine(const Vector& vector)
{
    RenderLine(vector.From, vector.To);
}

void RenderContext::RenderLine(const Vector& vector, VectorStyle style)
{
    RenderLine(vector.From, vector.To, style);
}

void RenderContext::RenderLine(const Point& fromPoint, const Point& toPoint)
{
    RenderLine(fromPoint, toPoint, [](const Point& current, const Vector& v)
        {
            int32_t dx = std::abs(v.To.X - v.From.X);
            int32_t dy = std::abs(v.To.Y - v.From.Y);
            return (dx > dy) ? L'─' : L'│';
        });
}

void RenderContext::RenderLine(const Point& fromPoint, const Point& toPoint, VectorStyle style)
{
    std::lock_guard<std::recursive_mutex> guard(buffer_.renderMutex);

    int32_t x1 = fromPoint.X;
    int32_t y1 = fromPoint.Y;
    int32_t x2 = toPoint.X;
    int32_t y2 = toPoint.Y;

    int32_t dx = std::abs(x2 - x1);
    int32_t dy = std::abs(y2 - y1);
    int32_t sx = (x1 < x2) ? 1 : -1;
    int32_t sy = (y1 < y2) ? 1 : -1;
    int32_t err = dx - dy;

    Vector v(fromPoint, toPoint);

    while (true)
    {
        wchar_t symbol = style(Point(x1, y1), v);
        if (symbol != L'\0')
        {
            SetCell(x1, y1, CellInfo{ symbol });
        }

        if (x1 == x2 && y1 == y2)
            break;

        int32_t e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

void RenderContext::RenderLine(const Point& point, const int32_t length, short direction)
{
    Point to = point;
    if (direction == 0)
    {
        to.X += (length > 0 ? length - 1 : 0);
    }
    else
    {
        to.Y += (length > 0 ? length - 1 : 0);
    }

    RenderLine(point, to);
}

void RenderContext::RenderLine(const Point& point, const int32_t length, VectorStyle style)
{
    Point to(point.X + (length > 0 ? length - 1 : 0), point.Y);
    RenderLine(point, to, style);
}
// --- End Source: Engine/RenderContext.cpp ---

// --- Begin Source: Engine/RenderStream.cpp ---

// #include <cstdint> (Moved to top)
// #include <string> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

RenderStream& RenderStream::operator<<(const std::wstring& text)
{
    if (text.empty())
        return *this;

    context_.RenderText(pos_, text, fg_, bg_, wrap_);
    pos_.X += static_cast<int32_t>(text.length());
    return *this;
}

RenderStream& RenderStream::operator<<(const std::string& text)
{
    return *this << std::wstring(text.begin(), text.end());
}

RenderStream& RenderStream::operator<<(const wchar_t* text)
{
    return *this << std::wstring(text);
}

RenderStream& RenderStream::operator<<(const char* text)
{
    return *this << std::string(text);
}

RenderStream& RenderStream::operator<<(int32_t value)
{
    return *this << std::to_wstring(value);
}
RenderStream& RenderStream::operator<<(uint32_t value)
{
    return *this << std::to_wstring(value);
}
RenderStream& RenderStream::operator<<(float value)
{
    return *this << std::to_wstring(value);
}
RenderStream& RenderStream::operator<<(double value)
{
    return *this << std::to_wstring(value);
}

RenderStream& RenderStream::operator<<(RenderStream& (*manipulator)(RenderStream&))
{
    return manipulator(*this);
}

void RenderStream::NewLine()
{
    pos_.X = 0; pos_.Y += 1;
}
// --- End Source: Engine/RenderStream.cpp ---

// --- Begin Source: Framework/ControlBase.cpp ---

// #include <cstdint> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <memory> (Moved to top)
// #include <functional> (Moved to top)
// #include <unordered_map> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

static std::unordered_map<InputKey, bool> hotkeyExecutionState;

void ControlBase::ResetHotkeyExecutionState()
{
	hotkeyExecutionState.clear();
}

void ControlBase::SetFocusable(bool value)
{
	if (focusable_ == value)
		return;

	focusable_ = value;
	OnPropertyChanged("Focusable");
}

void ControlBase::SetTabStop(bool value)
{
	if (isTabStop_ == value)
		return;

	isTabStop_ = value;
	OnPropertyChanged("TabStop");
}

void ControlBase::SetTabIndex(int value)
{
	if (tabIndex_ == value)
		return;

	tabIndex_ = value;
	OnPropertyChanged("TabIndex");
}

Size ControlBase::Measure(const Size& availableSize)
{
	int32_t innerWidth  = availableSize.Width;
	int32_t innerHeight = availableSize.Height;

	if (MaxSize->Width >= 0)
		innerWidth  = innerWidth  < 0 ? MaxSize->Width  : std::min(innerWidth,  MaxSize->Width);

	if (MaxSize->Height >= 0)
		innerHeight = innerHeight < 0 ? MaxSize->Height : std::min(innerHeight, MaxSize->Height);

	if (innerWidth >= 0)
		innerWidth  = std::max(0, innerWidth  - Margin->Left - Margin->Right);

	if (innerHeight >= 0)
		innerHeight = std::max(0, innerHeight - Margin->Top  - Margin->Bottom);

	if (MinSize->Width  >= 0 && innerWidth  >= 0)
		innerWidth  = std::max(innerWidth,  MinSize->Width);

	if (MinSize->Height >= 0 && innerHeight >= 0)
		innerHeight = std::max(innerHeight, MinSize->Height);

	Size innerSize(innerWidth, innerHeight);
	if (innerSize == Size::Zero)
	{
		actualSize_ = Size(Margin->Left + Margin->Right, Margin->Top + Margin->Bottom);
		return actualSize_;
	}

	Size contentSize = MeasureOverride(innerSize);
	actualSize_ = Size(
		contentSize.Width  >= 0 ? contentSize.Width  + Margin->Left + Margin->Right  : -1,
		contentSize.Height >= 0 ? contentSize.Height + Margin->Top  + Margin->Bottom : -1
	);

	return actualSize_;
}

void ControlBase::Arrange(const Rect& finalRect)
{
	int32_t slotX      = finalRect.X + Margin->Left;
	int32_t slotY      = finalRect.Y + Margin->Top;
	int32_t slotWidth  = std::max(0, finalRect.Width  - Margin->Left - Margin->Right);
	int32_t slotHeight = std::max(0, finalRect.Height - Margin->Top  - Margin->Bottom);

	int32_t contentWidth  = actualSize_.Width  >= 0 ? std::max(0, actualSize_.Width  - Margin->Left - Margin->Right)  : -1;
	int32_t contentHeight = actualSize_.Height >= 0 ? std::max(0, actualSize_.Height - Margin->Top  - Margin->Bottom) : -1;

	int32_t x      = slotX;
	int32_t y      = slotY;
	int32_t width  = contentWidth;
	int32_t height = contentHeight;

	switch (HorizontalAlignment)
	{
		case HorizontalAlign::Left:
		{
			width = width < 0 ? slotWidth : std::min(width, slotWidth);
			break;
		}

		case HorizontalAlign::Center:
		{
			width = width < 0 ? slotWidth : std::min(width, slotWidth);
			x += (slotWidth - width) / 2;
			break;
		}

		case HorizontalAlign::Right:
		{
			width = width < 0 ? slotWidth : std::min(width, slotWidth);
			x += slotWidth - width;
			break;
		}

		case HorizontalAlign::Stretch:
		{
			width = std::clamp<int32_t>(slotWidth,
				MinSize->Width  < 0 ? 0         : MinSize->Width,
				MaxSize->Width  < 0 ? slotWidth : MaxSize->Width);

			x += (slotWidth - width) / 2;
			break;
		}
	}

	switch (VerticalAlignment)
	{
		case VerticalAlign::Top:
		{
			height = height < 0 ? slotHeight : std::min(height, slotHeight);
			break;
		}

		case VerticalAlign::Center:
		{
			height = height < 0 ? slotHeight : std::min(height, slotHeight);
			y += (slotHeight - height) / 2;
			break;
		}

		case VerticalAlign::Bottom:
		{
			height = height < 0 ? slotHeight : std::min(height, slotHeight);
			y += slotHeight - height;
			break;
		}

		case VerticalAlign::Stretch:
		{
			height = std::clamp<int32_t>(slotHeight,
				MinSize->Height < 0 ? 0          : MinSize->Height,
				MaxSize->Height < 0 ? slotHeight : MaxSize->Height);

			y += (slotHeight - height) / 2;
			break;
		}
	}

	arrangeDirty_ = false;
	arrangedRect_ = Rect(x, y, width, height);
	ArrangeOverride(arrangedRect_);
}

static wchar_t EmptyRectangleStyle(const Point& point, const Size& size)
{
	return L' ';
}

void ControlBase::Render(RenderContext& context)
{
	Rect rect = context.ContextRect();
	context.RenderRectangle(Point::Zero, rect.AsSize(), ForegroundColor, BackgroundColor, EmptyRectangleStyle);

	visualDirty_ = false;
	RenderOverride(context);
}

void ControlBase::SetParent(VisualTreeNode* parent)
{
	if (parent_ == parent)
		return;

	if (parent == nullptr)
	{
		OnDettachedFromTree();
		parent_ = nullptr;
		return;
	}

	parent_ = parent;
	OnPropertyChanged("Parent");
}

void ControlBase::SetLayer(UILayer* layer)
{
	if (layer_ == layer)
		return;

	if (layer == nullptr)
	{
		layer_ = nullptr;
		return;
	}

	layer_ = layer;
	OnPropertyChanged("Layer");

	for (auto it = child_begin(); it != child_end(); ++it)
	{
		VisualTreeNode* child = *it;
		child->SetLayer(layer);
	}
}

const ChildIterator ControlBase::child_begin() const
{
	return ChildIterator(this, 0);
}

const ChildIterator ControlBase::child_end() const
{
	return ChildIterator(this, VisualChildrenCount());
}

void ControlBase::Close()
{
	layer_->Running.store(false);
}

void ControlBase::OpenContextMenu()
{
	if (CtxMenu.Get() != nullptr)
	{
		Rect rect = GetArrangedRect();
		CtxMenu.Get()->Open(Point(rect.X, rect.Y + rect.Height));
	}
}

size_t ControlBase::VisualChildrenCount() const
{
	return 0;
}

VisualTreeNode* ControlBase::GetVisualChild(std::size_t index) const
{
	return nullptr;
}

void ControlBase::ApplyInvalidation(InvalidationKind invalidation)
{
	if (hasFlag(invalidation, InvalidationKind::Visual))
		InvalidateVisual();

	if (hasFlag(invalidation, InvalidationKind::Arrange))
		InvalidateArrange();

	if (hasFlag(invalidation, InvalidationKind::Measure))
		InvalidateMeasure();
}

bool ControlBase::OnKeyDown(InputEvent input)
{
	KeyDown.Emit(input);

    if (input.Pressed && hotkeyExecutionState[input.Key])
        return true;

	constexpr InputModifier EssentialModifiers =
		InputModifier::LeftAlt | InputModifier::RightAlt |
		InputModifier::LeftCtrl | InputModifier::RightCtrl |
		InputModifier::Shift;

	InputModifier cleanModifier = input.Modifier & EssentialModifiers;
	for (const auto& pair : hotkeys_)
	{
		const InputEvent& event = pair.first;
		if (cleanModifier == event.Modifier && input.Key == event.Key && input.Pressed == event.Pressed)
		{
            hotkeyExecutionState[input.Key] = input.Pressed;

			HotkeyCallback callback = pair.second;
			callback(this);
			return true;
		}
	}

	switch (input.Key)
	{
		case InputKey::None:
		{
			break;
		}

		case InputKey::UP:
		{
			PopFocus(Direction::Up, input.Modifier);
			return true;
		}

		case InputKey::DOWN:
		{
			PopFocus(Direction::Down, input.Modifier);
			return true;
		}

		case InputKey::LEFT:
		{
			PopFocus(Direction::Left, input.Modifier);
			return true;
		}

		case InputKey::RIGHT:
		{
			PopFocus(Direction::Right, input.Modifier);
			return true;
		}

		case InputKey::TAB:
		{
			PopFocus(hasFlag(input.Modifier, InputModifier::Shift) ? Direction::Previous : Direction::Next, input.Modifier);
			return true;
		}
	}

	return false;
}

bool ControlBase::OnKeyUp(InputEvent input)
{
	KeyUp.Emit(input);

    hotkeyExecutionState[input.Key] = false;

	constexpr InputModifier EssentialModifiers =
		InputModifier::LeftAlt | InputModifier::RightAlt |
		InputModifier::LeftCtrl | InputModifier::RightCtrl |
		InputModifier::Shift;

	InputModifier cleanModifier = input.Modifier & EssentialModifiers;
	for (const auto& pair : hotkeys_)
	{
		const InputEvent& event = pair.first;
		if (cleanModifier == event.Modifier && input.Key == event.Key && input.Pressed == event.Pressed)
		{
			HotkeyCallback callback = pair.second;
			callback(this);
			return true;
		}
	}

	return false;
}

void ControlBase::OnHotkey(InputModifier modifier, InputKey key, HotkeyCallback callback)
{
	InputEvent event(modifier, key, true);
	if (hotkeys_.find(event) != hotkeys_.end())
		throw std::runtime_error("Combination is already registered.");

	hotkeys_[event] = std::move(callback);
}

void ControlBase::OnPropertyChanged(const char* propertyName)
{
	if (std::strcmp(propertyName, "ExpSize") == 0)
	{
		Size newSize = ExpSize.Get();
		MinSize.Set(std::move(newSize));
		MaxSize.Set(std::move(newSize));
	}
}
// --- End Source: Framework/ControlBase.cpp ---

// --- Begin Source: Framework/HostApplication.cpp ---

// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)
// #include <string> (Moved to top)
// #include <stdexcept> (Moved to top)
// #include <optional> (Moved to top)
// #include <iostream> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

HostApplication& HostApplication::Current()
{
	static HostApplication app;
	return app;
}

void HostApplication::RunUILoop(std::unique_ptr<VisualTreeNode> root)
{
	static bool isRunning = false;
	if (root == nullptr)
		throw std::runtime_error("UI root widget cannot be null or empty");

	if (isRunning)
		throw std::runtime_error("UI loop is already running. Nesting RunUILoop is not allowed. Use NestUILoop instead.");

	DispatchTimer& timer = DispatchTimer::Current();
	/*
	if (timer.CheckAccess())
		throw std::runtime_error("UI loop thread was already start somewhere else");
	*/

	isRunning = true;
	timer.SetUIThread();
	VisualTree& tree = VisualTree::Current();
	UILayer& layer = tree.PushLayer(std::move(root));

	const Size initViewport = HostBackend::QueryViewportSize();
	renderBuffer_.Resize(static_cast<uint32_t>(initViewport.Width), static_cast<uint32_t>(initViewport.Height));

	auto resizeConn = timer.ResizeFinishedEvent.Connect([&tree]()
	{
		if (tree.Root() != nullptr)
		{
			tree.Root()->InvalidateMeasure();
			tree.Root()->InvalidateVisual();
		}
	});

	timer.Start();
	NestUILoop(layer);
	timer.Stop();
	isRunning = false;
}

void HostApplication::NestUILoop(UILayer& layer)
{
	DispatchTimer& timer = DispatchTimer::Current();
	VisualTree& tree = VisualTree::Current();

	layer.Running.store(true);
	while (layer.Running.load() && timer.IsRunning())
	{
		timer.Tick();
		timer.ProcessTasks();

		const Size viewport = HostBackend::QueryViewportSize();
		if (viewport.Height != renderBuffer_.Height() || viewport.Width != renderBuffer_.Width())
		{
			renderBuffer_.Resize(static_cast<uint32_t>(viewport.Width), static_cast<uint32_t>(viewport.Height));
			timer.BeginResize();
		}

		if (!timer.IsResizing())
		{
			tree.RunLayout(viewport);
			if (tree.HasDirtyVisual())
			{
				tree.Render(renderBuffer_);
				renderBuffer_.DiffRender(std::wcout);
			}
		}

		const InputEvent evt = HostBackend::PollInput(timer.GetRemainingFrameTime(60));
		if (evt.Key != InputKey::None)
		{
			FocusManager& focus = tree.GetFocusManager();
			VisualTreeNode* focused = focus.GetFocused();

			bool success = false;
			while (!success && focused != nullptr)
			{
				success = evt.Pressed
					? focused->OnKeyDown(evt)
					: focused->OnKeyUp(evt);

				focused = focused->GetParent();
			}

			if (!timer.IsResizing())
			{
				tree.RunLayout(viewport);
				if (tree.HasDirtyVisual())
				{
					tree.Render(renderBuffer_);
					renderBuffer_.DiffRender(std::wcout);
				}
			}
		}
		else
		{
			ControlBase::ResetHotkeyExecutionState();
		}
	}
}

void HostApplication::RequestStop()
{
	DispatchTimer& timer = DispatchTimer::Current();
	timer.Stop();
}
// --- End Source: Framework/HostApplication.cpp ---

// --- Begin Source: Framework/VisualTree.cpp ---

// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)
// #include <functional> (Moved to top)
// #include <algorithm> (Moved to top)
// #include <stdexcept> (Moved to top)
// #include <stack> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

VisualTree::VisualTree()
{
	layers_.reserve(100);
}

VisualTree& VisualTree::Current()
{
	if (!DispatchTimer::Current().CheckAccess())
		throw std::runtime_error("Cannot get FocusManager within running UI thread or Before UI thread was started.");

	static VisualTree visualTree;
	return visualTree;
}

size_t VisualTree::LayerCount()
{
	return layers_.size();
}

VisualTreeNode* VisualTree::Root() const
{
	if (layers_.empty())
		return nullptr;

	return layers_.at(0)->RootNode.get();
}

VisualTreeNode* VisualTree::PeekLayer() const
{
	if (layers_.empty())
		return nullptr;

	return layers_.back()->RootNode.get();
}

/*
void VisualTree::SetRoot(std::unique_ptr<VisualTreeNode> rootNode)
{
	if (layers_.empty())
	{
		layers_.push_back(UILayer{ std::move(rootNode), FocusManager() });
	}
	else
	{
		layers_[0] = UILayer{ std::move(rootNode), FocusManager() };
	}

	hasDirtyVisual_ = true;
	dirtyRect_ = Rect();
}
*/

UILayer& VisualTree::PushLayer(std::unique_ptr<VisualTreeNode> layerRoot)
{
	if (layers_.size() == 100)
		throw std::runtime_error("UI layer stack overflow.");

	layers_.emplace_back(std::make_unique<UILayer>(std::move(layerRoot)));
	hasDirtyVisual_ = true;
	dirtyRect_ = Rect();

	UILayer& layer = *layers_.back().get();
	layer.Focus.SetFocused(layer.RootNode.get());
	return layer;
}

void VisualTree::PopLayer()
{
	if (layers_.size() > 1)
	{
		UILayer& layer = *layers_.back().get();
		layer.Running.store(false);
		layers_.pop_back();

		hasDirtyVisual_ = true;
		dirtyRect_ = Rect();
	}
}

FocusManager& VisualTree::GetFocusManager()
{
	if (layers_.empty())
		throw std::runtime_error("No layers in VisualTree");

	return layers_.back()->Focus;
}

void VisualTree::Invalidate(const Rect& dirtyRect)
{
	hasDirtyVisual_ = true;
	dirtyRect_ = Rect::Union(dirtyRect_, dirtyRect);
}

void VisualTree::CollectDirtyNodeRect(const VisualTreeNode& node)
{
	if (node.IsVisualDirty())
	{
		hasDirtyVisual_ = true;
	}
}

void VisualTree::RunLayout(const Size& viewportSize)
{
	for (auto& layer : layers_)
	{
		if (layer->RootNode == nullptr)
			continue;

		Size desiredSize = layer->RootNode->Measure(viewportSize);
		layer->RootNode->Arrange(Rect(0, 0, viewportSize.Width, viewportSize.Height));
		CollectDirtyNodeRect(*layer->RootNode);
	}
}

void VisualTree::Render(RenderBuffer& buffer)
{
	if (layers_.empty())
		return;

	if (!hasDirtyVisual_)
		return;

	UILayer& topLayer = *layers_.back();
	if (topLayer.RootNode == nullptr)
		return;

	Rect nodeRect = topLayer.RootNode.get()->GetArrangedRect();
	RenderContext context(buffer, nodeRect);

	topLayer.RootNode->Render(context);
	hasDirtyVisual_ = false;
	dirtyRect_ = Rect();
}
// --- End Source: Framework/VisualTree.cpp ---

// --- Begin Source: Framework/VisualTreeNode.cpp ---

// #include <cstdint> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

void VisualTreeNode::PopFocus(Direction direction, InputModifier modifiers)
{
	FocusManager::Current().MoveNext(direction, modifiers);
}

void VisualTreeNode::PushFocus(VisualTreeNode* focused)
{
	FocusManager::Current().SetFocused(focused);
}

VisualTreeNode* VisualTreeNode::GetParent() const
{
	return parent_;
}

void VisualTreeNode::InvalidateMeasure()
{
	DispatchTimer::Current().VerifyAccess();
	measureDirty_ = true;
	arrangeDirty_ = true;
	visualDirty_ = true;

	if (parent_ != nullptr)
	{
		parent_->OnChildInvalidated(*this);
		parent_->InvalidateMeasure();
	}
}

void VisualTreeNode::InvalidateArrange()
{
	arrangeDirty_ = true;
	visualDirty_ = true;

	if (parent_ != nullptr)
	{
		parent_->OnChildInvalidated(*this);
		parent_->InvalidateArrange();
	}
}

void VisualTreeNode::InvalidateVisual()
{
	visualDirty_ = true;

	if (parent_ != nullptr)
	{
		//parent_->OnChildInvalidated(*this);
		parent_->InvalidateVisual();
	}
}

bool VisualTreeNode::IsAttached() const
{
	return attached_;
}

bool VisualTreeNode::IsMeasureDirty() const
{
	return measureDirty_;
}

bool VisualTreeNode::IsArrangeDirty() const
{
	return arrangeDirty_;
}

bool VisualTreeNode::IsVisualDirty() const
{
	return visualDirty_;
}

bool VisualTreeNode::IsFocusable() const
{
	return focusable_;
}

bool VisualTreeNode::IsTabStop() const
{
	return isTabStop_ || true;
}

int VisualTreeNode::GetTabIndex() const
{
	return tabIndex_;
}

bool VisualTreeNode::OnKeyDown(InputEvent input)
{
	return false;
}

bool VisualTreeNode::OnKeyUp(InputEvent input)
{
	return false;
}

bool VisualTreeNode::MoveFocusNext(Direction direction, InputModifier modifiers)
{
	if (!IsFocusable())
		return false;

	if (!focused_)
		return true;

	return false;
}

void VisualTreeNode::OnChildInvalidated(VisualTreeNode& child)
{
	InvalidateMeasure();
	InvalidateVisual();
}

void VisualTreeNode::OnAttachedToTree()
{
	attached_ = true;
	InvalidateMeasure();
	InvalidateVisual();
}

void VisualTreeNode::OnDettachedFromTree()
{
	attached_ = false;
	FocusManager::Current().ClearFocus(this);
	InvalidateMeasure();
	InvalidateVisual();
}

void VisualTreeNode::OnGotFocus()
{
	focused_ = true;
	InvalidateVisual();
}

void VisualTreeNode::OnLostFocus()
{
	focused_ = false;
	InvalidateVisual();
}

Size VisualTreeNode::GetActualSize() const
{
	return actualSize_;
}

Rect VisualTreeNode::GetArrangedRect() const
{
	return arrangedRect_;
}
// --- End Source: Framework/VisualTreeNode.cpp ---

// --- Begin Source: Framework/PlatformSpecific/HostApplication.Linux.cpp ---
#if defined(__linux__) || defined(__APPLE__)

// #include <cstdlib> (Moved to top)
// #include <iostream> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)

using namespace terminality;

#include <sys/ioctl.h>
// #include <clocale> (Moved to top)
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <termios.h>

static struct termios original_termios;

void HostApplication::EnterTerminal()
{
	std::setlocale(LC_ALL, "");

	try
	{
		std::locale loc("");
		std::locale::global(loc);
		std::cout.imbue(loc);
		std::cerr.imbue(loc);
		std::wcout.imbue(loc);
		std::wcin.imbue(loc);
	}
	catch (...)
	{
		std::cerr << "Warning: Failed to set UTF-8 locale. UI may render incorrectly.\n";
	}

	tcgetattr(STDIN_FILENO, &original_termios);

	struct termios raw = original_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	std::ios_base::sync_with_stdio(false);
	std::wcout.tie(nullptr);

	std::wcout << L"\x1b[?1049h\x1b[0m\x1b[40m\x1b[?7l\x1b[2J\x1b[?25l";
	std::wcout.flush();
}

void HostApplication::ExitTerminal()
{
	std::wcout << L"\x1b[?1049l\x1b[?7h\x1b[?25h";
	std::wcout.flush();

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

Size HostBackend::QueryViewportSize()
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return Size(w.ws_col, w.ws_row);
}

InputEvent HostBackend::PollInput(std::chrono::milliseconds timeout)
{
	struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
	int ret = poll(&pfd, 1, timeout.count());

	if (ret <= 0 || !(pfd.revents & POLLIN))
		return InputEvent(InputModifier::None, InputKey::None, false);

	char buffer[32];
	ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

	if (bytesRead <= 0)
		return InputEvent(InputModifier::None, InputKey::None, false);

	buffer[bytesRead] = '\0';

	if (buffer[0] == '\x1b')
	{
		if (bytesRead == 1)
			return InputEvent(InputModifier::None, InputKey::ESCAPE, true);

		if (bytesRead >= 3 && buffer[1] == '[')
		{
			switch (buffer[2])
			{
				case 'A': return InputEvent(InputModifier::None, InputKey::UP, true);
				case 'B': return InputEvent(InputModifier::None, InputKey::DOWN, true);
				case 'C': return InputEvent(InputModifier::None, InputKey::RIGHT, true);
				case 'D': return InputEvent(InputModifier::None, InputKey::LEFT, true);
			}
		}
		return InputEvent(InputModifier::None, InputKey::None, false);
	}

	if (bytesRead == 1)
	{
		char c = buffer[0];
		if (c == '\t') return InputEvent(InputModifier::None, InputKey::TAB, true);
		if (c == '\n' || c == '\r') return InputEvent(InputModifier::None, InputKey::RETURN, true);
		if (c == 127 || c == '\b') return InputEvent(InputModifier::None, InputKey::BACK, true);
		if (c == ' ') return InputEvent(InputModifier::None, InputKey::SPACE, true);
	}

	wchar_t wc = 0;
	if (std::mbtowc(&wc, buffer, bytesRead) > 0 && wc >= 32)
		return InputEvent(wc, true);

	return InputEvent(InputModifier::None, InputKey::None, false);
}

#endif // __linux__ || __APPLE__
// --- End Source: Framework/PlatformSpecific/HostApplication.Linux.cpp ---

// --- Begin Source: Framework/PlatformSpecific/HostApplication.Windows.cpp ---
#ifdef _WIN32

// #include <chrono> (Moved to top)
// #include <clocale> (Moved to top)
// #include <cstdint> (Moved to top)
// #include <iostream> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)
#include <Windows.h>

using namespace terminality;

void HostApplication::EnterTerminal()
{
    std::setlocale(LC_ALL, ".UTF-8");
    try
    {
        std::locale::global(std::locale(".UTF-8"));
        std::wcout.imbue(std::locale());
        std::wcerr.imbue(std::locale());
    }
    catch (...)
    {
        // Locale not available; keep process defaults.
    }

    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD outMode = 0;
    if (GetConsoleMode(hOutput, &outMode))
    {
        outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOutput, outMode);
    }

    CONSOLE_CURSOR_INFO cursorInfo;
    if (GetConsoleCursorInfo(hOutput, &cursorInfo))
    {
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hOutput, &cursorInfo);
    }

    DWORD inMode = 0;
    if (GetConsoleMode(hInput, &inMode))
        SetConsoleMode(hInput, inMode | ENABLE_WINDOW_INPUT);

    std::ios_base::sync_with_stdio(false);
    std::wcout.tie(nullptr);
    std::wcout << L"\x1b[?1049h\x1b[0m\x1b[40m\x1b[?7l\x1b[2J\x1b[?25l";
}

void HostApplication::ExitTerminal()
{
    std::wcout << L"\x1b[?1049l\x1b[?7h\x1b[?25h";
	std::wcout.flush();
}

Size HostBackend::QueryViewportSize()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    return Size(columns, rows);
}

InputEvent HostBackend::PollInput(std::chrono::milliseconds timeout)
{
    static HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD waitResult = WaitForSingleObject(hInput, static_cast<DWORD>(timeout.count()));

    if (waitResult != WAIT_OBJECT_0)
        return InputEvent(InputModifier::None, InputKey::None, false);

    INPUT_RECORD record;
    DWORD read;

    if (!ReadConsoleInputW(hInput, &record, 1, &read) || read == 0)
        return InputEvent(InputModifier::None, InputKey::None, record.Event.KeyEvent.bKeyDown);

    if (record.EventType != KEY_EVENT)
        return InputEvent(InputModifier::None, InputKey::None, record.Event.KeyEvent.bKeyDown);

    const auto& keyEvent = record.Event.KeyEvent;
    const InputKey keyCode = static_cast<InputKey>(keyEvent.wVirtualKeyCode);
    const InputModifier modifiers = static_cast<InputModifier>(keyEvent.dwControlKeyState);
    const wchar_t unicodeChar = keyEvent.uChar.UnicodeChar;

    switch (keyCode)
    {
        case InputKey::UP:
        case InputKey::DOWN:
        case InputKey::LEFT:
        case InputKey::RIGHT:
        case InputKey::TAB:
        case InputKey::BACK:
        case InputKey::RETURN:
        case InputKey::SPACE:
        case InputKey::ESCAPE:
            return InputEvent(modifiers, keyCode, record.Event.KeyEvent.bKeyDown);
    }

    if (unicodeChar >= 32)
        return InputEvent(modifiers, keyCode, unicodeChar, record.Event.KeyEvent.bKeyDown);

    return InputEvent(InputModifier::None, InputKey::None, record.Event.KeyEvent.bKeyDown);
}

#endif // _WIN32
// --- End Source: Framework/PlatformSpecific/HostApplication.Windows.cpp ---

// --- Begin Source: Framework/PlatformSpecific/Windows.cpp ---
#ifdef _WIN32

// #include <string> (Moved to top)
// #include <thread> (Moved to top)
// #include <memory> (Moved to top)

// #include "terminality/Terminality.hpp" (Merged)
#include <Windows.h>

using namespace terminality;

void terminality::AlertAsync(const std::wstring& text, const std::wstring& title)
{
    std::thread([text, title]() { MessageBoxW(nullptr, text.c_str(), title.size() == 0 ? nullptr : title.c_str(), MB_OK | MB_ICONINFORMATION); }).detach();
}

#endif // _WIN32
// --- End Source: Framework/PlatformSpecific/Windows.cpp ---

#endif // TERMINALITY_IMPLEMENTATION
