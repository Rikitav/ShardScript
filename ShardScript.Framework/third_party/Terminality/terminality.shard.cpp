#define NOMINMAX
#include <ShardScript.hpp>

#undef MessageBox
#undef DELETE
#undef TRANSPARENT
#undef XBUTTON1
#undef XBUTTON2

#define TERMINALITY_IMPLEMENTATION
#include "Terminality.hpp"

#include <stdexcept>
#include <string>
#include <thread>
#include <chrono>

using namespace shard;
using namespace terminality;

static TypeSymbol* gHorizontalAlignmentEnum = nullptr;
static TypeSymbol* gVerticalAlignmentEnum = nullptr;
static TypeSymbol* gOrientationEnum = nullptr;

// -----------------------------------------------------------------------------
// Geometry / primitive type symbols
// -----------------------------------------------------------------------------

static StructSymbol* g_PointStruct = nullptr;
static FieldSymbol* g_Point_X = nullptr;
static FieldSymbol* g_Point_Y = nullptr;
static ConstructorSymbol* g_PointStruct_init = nullptr;

static StructSymbol* g_SizeStruct = nullptr;
static FieldSymbol* g_Size_Width = nullptr;
static FieldSymbol* g_Size_Height = nullptr;
static ConstructorSymbol* g_SizeStruct_init = nullptr;

static StructSymbol* g_RectStruct = nullptr;
static FieldSymbol* g_Rect_X = nullptr;
static FieldSymbol* g_Rect_Y = nullptr;
static FieldSymbol* g_Rect_Width = nullptr;
static FieldSymbol* g_Rect_Height = nullptr;
static ConstructorSymbol* g_RectStruct_init = nullptr;

static StructSymbol* g_ThicknessStruct = nullptr;
static FieldSymbol* g_Thickness_Left = nullptr;
static FieldSymbol* g_Thickness_Top = nullptr;
static FieldSymbol* g_Thickness_Right = nullptr;
static FieldSymbol* g_Thickness_Bottom = nullptr;
static ConstructorSymbol* g_ThicknessStruct_init = nullptr;

static StructSymbol* g_InputEventStruct = nullptr;
static FieldSymbol* g_InputEvent_Modifier = nullptr;
static FieldSymbol* g_InputEvent_Key = nullptr;
static FieldSymbol* g_InputEvent_Char = nullptr;
static FieldSymbol* g_InputEvent_Pressed = nullptr;
static ConstructorSymbol* g_InputEventStruct_init = nullptr;

static TypeSymbol* g_ColorEnum = nullptr;
static TypeSymbol* g_InputKeyEnum = nullptr;
static TypeSymbol* g_InputModifierEnum = nullptr;

// -----------------------------------------------------------------------------
// IWidget / IRenderContext interface method handles
// -----------------------------------------------------------------------------

static InterfaceSymbol* g_IWidget = nullptr;
static MethodSymbol* g_IWidget_Measure = nullptr;
static MethodSymbol* g_IWidget_Arrange = nullptr;
static MethodSymbol* g_IWidget_Render = nullptr;
static MethodSymbol* g_IWidget_OnKeyDown = nullptr;
static MethodSymbol* g_IWidget_OnKeyUp = nullptr;

static InterfaceSymbol* g_IRenderContext = nullptr;
static MethodSymbol* g_IRenderContext_ContextRect = nullptr;
static MethodSymbol* g_IRenderContext_SetCell = nullptr;
static MethodSymbol* g_IRenderContext_RenderText = nullptr;
static MethodSymbol* g_IRenderContext_RenderRectangle = nullptr;
static MethodSymbol* g_IRenderContext_CreateInner = nullptr;

static ClassSymbol* g_RenderContextClass = nullptr;
static FieldSymbol* g_RenderContext_PtrField = nullptr;

static ClassSymbol* g_ControlClass = nullptr;
static FieldSymbol* g_Control_PtrField = nullptr;
static ClassSymbol* g_ControlFactoryClass = nullptr;
static MethodSymbol* g_ControlFactory_FromWidget = nullptr;

static ClassSymbol* g_StackPanelClass = nullptr;
static ClassSymbol* g_LabelClass = nullptr;
static ClassSymbol* g_ButtonClass = nullptr;

namespace
{
	static ObjectInstance* MakeEnumValue(GarbageCollector& collector, TypeSymbol* enumType, std::int64_t value)
	{
		ObjectInstance* instance = collector.AllocateInstance(enumType);
		instance->WriteInteger(value);
		return instance;
	}

	// -----------------------------------------------------------------------------
	// Geometry helpers: create/read Point, Size, Rect, Thickness
	// -----------------------------------------------------------------------------

	static Point GetPoint(ObjectInstance* instance)
	{
		if (instance == nullptr)
			return Point();

		int32_t x = static_cast<int32_t>(instance->GetField(g_Point_X->SlotIndex)->AsInteger());
		int32_t y = static_cast<int32_t>(instance->GetField(g_Point_Y->SlotIndex)->AsInteger());
		return Point(x, y);
	}

	static Size GetSize(ObjectInstance* instance)
	{
		if (instance == nullptr)
			return Size::Zero;

		int32_t width = static_cast<int32_t>(instance->GetField(g_Size_Width->SlotIndex)->AsInteger());
		int32_t height = static_cast<int32_t>(instance->GetField(g_Size_Height->SlotIndex)->AsInteger());
		return Size(width, height);
	}

	static Rect GetRect(ObjectInstance* instance)
	{
		if (instance == nullptr)
			return Rect();

		int32_t x = static_cast<int32_t>(instance->GetField(g_Rect_X->SlotIndex)->AsInteger());
		int32_t y = static_cast<int32_t>(instance->GetField(g_Rect_Y->SlotIndex)->AsInteger());
		int32_t width = static_cast<int32_t>(instance->GetField(g_Rect_Width->SlotIndex)->AsInteger());
		int32_t height = static_cast<int32_t>(instance->GetField(g_Rect_Height->SlotIndex)->AsInteger());
		return Rect(x, y, width, height);
	}

	static Thickness GetThickness(ObjectInstance* instance)
	{
		if (instance == nullptr)
			return Thickness::Zero;

		int32_t left = static_cast<int32_t>(instance->GetField(g_Thickness_Left->SlotIndex)->AsInteger());
		int32_t top = static_cast<int32_t>(instance->GetField(g_Thickness_Top->SlotIndex)->AsInteger());
		int32_t right = static_cast<int32_t>(instance->GetField(g_Thickness_Right->SlotIndex)->AsInteger());
		int32_t bottom = static_cast<int32_t>(instance->GetField(g_Thickness_Bottom->SlotIndex)->AsInteger());
		return Thickness(left, top, right, bottom);
	}

	static InputEvent GetInputEvent(ObjectInstance* instance)
	{
		if (instance == nullptr)
			return InputEvent(InputModifier::None, InputKey::None, false);

		auto modifier = static_cast<InputModifier>(instance->GetField(g_InputEvent_Modifier->SlotIndex)->AsInteger());
		auto key = static_cast<InputKey>(instance->GetField(g_InputEvent_Key->SlotIndex)->AsInteger());
		wchar_t ch = static_cast<wchar_t>(instance->GetField(g_InputEvent_Char->SlotIndex)->AsInteger());
		bool pressed = instance->GetField(g_InputEvent_Pressed->SlotIndex)->AsBoolean();
		return InputEvent(modifier, key, ch, pressed);
	}

	// -----------------------------------------------------------------------------
	// Static factory callbacks for geometry types
	// -----------------------------------------------------------------------------

	static ObjectInstance* shard_Point_Create(const CallState& context) noexcept(false)
	{
		auto [self, x, y] = GetArgs<ObjectInstance*, std::int64_t, std::int64_t>(context);
		self->SetField(g_Point_X->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(x)));
		self->SetField(g_Point_Y->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(y)));
		return nullptr;
	}

	static ObjectInstance* shard_Size_Create(const CallState& context) noexcept(false)
	{
		auto [self, width, height] = GetArgs<ObjectInstance*, std::int64_t, std::int64_t>(context);
		self->SetField(g_Size_Width->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(width)));
		self->SetField(g_Size_Height->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(height)));
		return nullptr;
	}

	static ObjectInstance* shard_Rect_Create(const CallState& context) noexcept(false)
	{
		auto [self, x, y, width, height] = GetArgs<ObjectInstance*, std::int64_t, std::int64_t, std::int64_t, std::int64_t>(context);
		self->SetField(g_Rect_X->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(x)));
		self->SetField(g_Rect_Y->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(y)));
		self->SetField(g_Rect_Width->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(width)));
		self->SetField(g_Rect_Height->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(height)));
		return nullptr;
	}

	static ObjectInstance* shard_Thickness_Create(const CallState& context) noexcept(false)
	{
		auto [self, left, top, right, bottom] = GetArgs<ObjectInstance*, std::int64_t, std::int64_t, std::int64_t, std::int64_t>(context);
		self->SetField(g_Thickness_Left->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(left)));
		self->SetField(g_Thickness_Top->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(top)));
		self->SetField(g_Thickness_Right->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(right)));
		self->SetField(g_Thickness_Bottom->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(bottom)));
		return nullptr;
	}

	// ============================================================================
	// Native control lifetime management.
	//
	// `_ptr` points to a small native `ControlHolder` struct (allocated with `new`).
	// The holder owns the actual Terminality control until the control is handed
	// over to a parent container or to HostApplication. After the transfer the
	// holder stops owning the control, so the GC can safely delete the holder when
	// the script wrapper is collected without double-freeing the control.
	// ============================================================================

	struct ControlHolder
	{
		bool Owned = true;
		ControlBase* Control = nullptr;
		DelegateRef ClickedDelegate;

		ControlHolder() = default;
		ControlHolder(ControlBase* control, bool owned)
			: Control(control), Owned(owned) {
		}

		~ControlHolder()
		{
			if (Owned && Control != nullptr)
				delete Control;

			// ClickedDelegate's destructor releases its ObjectRef automatically.
		}
	};

	static ControlHolder* GetHolder(ObjectInstance* instance)
	{
		TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
		FieldSymbol* ptrField = SemanticModel::FindFieldByName(type, L"_ptr");
		if (ptrField == nullptr)
			throw std::runtime_error("Terminality wrapper is missing the `_ptr` field");

		ObjectInstance* ptrInstance = instance->GetField(ptrField->SlotIndex);
		if (ptrInstance == nullptr || ptrInstance == GarbageCollector::NullInstance)
			return nullptr;

		if (ptrInstance->getInfo()->Inlining == TypeInlining::ByValue)
			return *static_cast<ControlHolder**>(ptrInstance->getMemory());

		return static_cast<ControlHolder*>(ptrInstance->AsNint());
	}

	static ControlBase* ExtractControlPtr(ObjectInstance* instance)
	{
		ControlHolder* holder = GetHolder(instance);
		if (holder == nullptr)
			return nullptr;

		return holder->Control;
	}

	static ControlBase* ReleaseControlPtr(ObjectInstance* instance)
	{
		ControlHolder* holder = GetHolder(instance);
		if (holder == nullptr || holder->Control == nullptr)
			return nullptr;

		ControlBase* native = holder->Control;
		holder->Owned = false;

		return native;
	}

	static void RegisterControlPtr(const CallState& context, ObjectInstance* instance, ControlBase* native)
	{
		TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
		FieldSymbol* ptrField = SemanticModel::FindFieldByName(type, L"_ptr");
		if (ptrField == nullptr)
			throw std::runtime_error("Terminality wrapper is missing the `_ptr` field");

		ControlHolder* holder = new ControlHolder{ native, true };
		ObjectInstance* wrapper = context.Collector.FromNint(holder, false);
		instance->SetField(ptrField->SlotIndex, wrapper);
	}

	// ============================================================================
	// RenderContext wrapper
	//
	// RenderContext objects are created on-demand during a render pass and must
	// not outlive the current RenderOverride call. The script-side RenderContext
	// simply stores the native RenderContext* as a native-integer field.
	// ============================================================================

	static RenderContext* GetRenderContext(ObjectInstance* instance)
	{
		TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
		FieldSymbol* ptrField = SemanticModel::FindFieldByName(type, L"_ptr");
		if (ptrField == nullptr)
			throw std::runtime_error("RenderContext wrapper is missing the `_ptr` field");

		ObjectInstance* ptrInstance = instance->GetField(ptrField->SlotIndex);
		if (ptrInstance == nullptr || ptrInstance == GarbageCollector::NullInstance)
			return nullptr;

		return static_cast<RenderContext*>(ptrInstance->AsNint());
	}

	static ObjectInstance* CreateRenderContextObject(GarbageCollector& collector, RenderContext* native)
	{
		ObjectInstance* instance = collector.AllocateInstance(g_RenderContextClass);
		instance->SetField(g_RenderContext_PtrField->SlotIndex, collector.FromNint(native, false));
		return instance;
	}
}

static ObjectInstance* shard_RenderContext_ContextRect(const CallState& context) noexcept(false)
{
	auto [self] = GetArgs<ObjectInstance*>(context);
	RenderContext* ctx = GetRenderContext(self);

	if (ctx == nullptr)
	{
		return NewObject(context, g_RectStruct, g_RectStruct_init,
		{
			context.Collector.FromValue(0ll),
			context.Collector.FromValue(0ll),
			context.Collector.FromValue(0ll),
			context.Collector.FromValue(0ll)
		});
	}

	Rect rect = ctx->ContextRect();
	return NewObject(context, g_RectStruct, g_RectStruct_init,
	{
		context.Collector.FromValue(static_cast<std::int64_t>(rect.X)),
		context.Collector.FromValue(static_cast<std::int64_t>(rect.Y)),
		context.Collector.FromValue(static_cast<std::int64_t>(rect.Width)),
		context.Collector.FromValue(static_cast<std::int64_t>(rect.Height))
	});
}

static ObjectInstance* shard_RenderContext_SetCell(const CallState& context) noexcept(false)
{
	auto [self, x, y, ch, fg, bg] = GetArgs<ObjectInstance*, std::int64_t, std::int64_t, std::int64_t, ObjectInstance*, ObjectInstance*>(context);
	RenderContext* ctx = GetRenderContext(self);
	if (ctx != nullptr)
	{
		Color fgColor = static_cast<Color>(fg != nullptr ? fg->AsInteger() : static_cast<std::int64_t>(Color::WHITE));
		Color bgColor = static_cast<Color>(bg != nullptr ? bg->AsInteger() : static_cast<std::int64_t>(Color::BLACK));
		ctx->SetCell(static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<wchar_t>(ch), fgColor, bgColor);
	}

	return nullptr;
}

static ObjectInstance* shard_RenderContext_RenderText(const CallState& context) noexcept(false)
{
	auto [self, point, text, fg, bg] = GetArgs<ObjectInstance*, ObjectInstance*, ObjectInstance*, ObjectInstance*, ObjectInstance*>(context);
	RenderContext* ctx = GetRenderContext(self);
	if (ctx == nullptr)
		return nullptr;

	Color fgColor = static_cast<Color>(fg != nullptr ? fg->AsInteger() : static_cast<std::int64_t>(Color::WHITE));
	Color bgColor = static_cast<Color>(bg != nullptr ? bg->AsInteger() : static_cast<std::int64_t>(Color::BLACK));
	ctx->RenderText(GetPoint(point), text != nullptr ? text->AsString() : L"", fgColor, bgColor, false);
	return nullptr;
}

static ObjectInstance* shard_RenderContext_RenderRectangle(const CallState& context) noexcept(false)
{
	auto [self, point, size, fg, bg] = GetArgs<ObjectInstance*, ObjectInstance*, ObjectInstance*, ObjectInstance*, ObjectInstance*>(context);
	RenderContext* ctx = GetRenderContext(self);
	if (ctx == nullptr)
		return nullptr;

	Color fgColor = static_cast<Color>(fg != nullptr ? fg->AsInteger() : static_cast<std::int64_t>(Color::WHITE));
	Color bgColor = static_cast<Color>(bg != nullptr ? bg->AsInteger() : static_cast<std::int64_t>(Color::BLACK));
	ctx->RenderRectangle(GetPoint(point), GetSize(size), fgColor, bgColor);
	return nullptr;
}

static ObjectInstance* shard_RenderContext_CreateInner(const CallState& context) noexcept(false)
{
	auto [self, rect] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
	RenderContext* ctx = GetRenderContext(self);
	if (ctx == nullptr)
		return nullptr;

	RenderContext inner = ctx->CreateInner(GetRect(rect));
	return CreateRenderContextObject(context.Collector, &inner);
}

// ============================================================================
// ScriptControl proxy
//
// A native Terminality control that forwards its layout/render lifecycle to a
// ShardScript object implementing IWidget.
// ============================================================================

class ScriptControl : public ControlBase
{
    ObjectRef m_widget;
    VirtualMachine* m_runtime = nullptr;

public:
    ScriptControl(VirtualMachine& runtime, ObjectInstance* widget)
        : m_widget(widget), m_runtime(&runtime) { }

    Size MeasureOverride(const Size& availableSize) override
    {
        if (m_runtime == nullptr || m_widget.Instance == nullptr)
            return Size::Zero;

        TypeSymbol* widgetType = const_cast<TypeSymbol*>(m_widget.Instance->getInfo());
        MethodSymbol* measureMethod = widgetType->FindInterfaceImplementation(g_IWidget_Measure);
        if (measureMethod == nullptr)
            return Size::Zero;

        ObjectInstance* sizeArg = nullptr;
        {
            // Create a temporary CallState-like context for invocation
            // We need a GarbageCollector reference; use the widget's type info to locate it is not possible,
            // so we pass creation through the VM's current frame if available.
            // Simpler: create the Size object using a small helper that needs the VM/collector.
            // We'll rely on the widget already being associated with a collector via ObjectRef.
        }

        // Avoid the above complexity: we cannot easily allocate here without a collector reference.
        // Use the VM's garbage collector directly.
        GarbageCollector& collector = m_runtime->GetGarbageCollector();
        sizeArg = CreateSizeInternal(collector, availableSize.Width, availableSize.Height);

        std::vector<ObjectInstance*> args = { m_widget.Instance, sizeArg };
        ObjectInstance* result = m_runtime->InvokeMethod(measureMethod, args.data(), args.size());
        if (result == nullptr)
            return Size::Zero;

        return GetSize(result);
    }

    void ArrangeOverride(const Rect& contentRect) override
    {
        if (m_runtime == nullptr || m_widget.Instance == nullptr)
            return;

        TypeSymbol* widgetType = const_cast<TypeSymbol*>(m_widget.Instance->getInfo());
        MethodSymbol* arrangeMethod = widgetType->FindInterfaceImplementation(g_IWidget_Arrange);
        if (arrangeMethod == nullptr)
            return;

        GarbageCollector& collector = m_runtime->GetGarbageCollector();
        ObjectInstance* rectArg = CreateRectInternal(collector, contentRect.X, contentRect.Y, contentRect.Width, contentRect.Height);
        m_runtime->InvokeMethod(arrangeMethod, { m_widget.Instance, rectArg });
    }

    void RenderOverride(RenderContext& context) override
    {
        if (m_runtime == nullptr || m_widget.Instance == nullptr)
            return;

        TypeSymbol* widgetType = const_cast<TypeSymbol*>(m_widget.Instance->getInfo());
        MethodSymbol* renderMethod = widgetType->FindInterfaceImplementation(g_IWidget_Render);
        if (renderMethod == nullptr)
            return;

        ObjectInstance* contextArg = CreateRenderContextObject(m_runtime->GetGarbageCollector(), &context);
        m_runtime->InvokeMethod(renderMethod, { m_widget.Instance, contextArg });
    }

    bool OnKeyDown(InputEvent input) override
    {
        return HandleKeyEvent(input, g_IWidget_OnKeyDown);
    }

    bool OnKeyUp(InputEvent input) override
    {
        return HandleKeyEvent(input, g_IWidget_OnKeyUp);
    }

private:
	static ObjectInstance* CreateInputEventInternal(const CallState& context, const InputEvent& input)
	{
		ObjectInstance* instance = context.Collector.AllocateInstance(g_InputEventStruct);
		instance->SetField(g_InputEvent_Modifier->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(input.Modifier)));
		instance->SetField(g_InputEvent_Key->SlotIndex, context.Collector.FromValue(static_cast<std::int64_t>(input.Key)));
		instance->SetField(g_InputEvent_Char->SlotIndex, context.Collector.FromValue(input.Char));
		instance->SetField(g_InputEvent_Pressed->SlotIndex, context.Collector.FromValue(input.Pressed));
		return instance;
	}

    static ObjectInstance* CreateSizeInternal(GarbageCollector& collector, int32_t width, int32_t height)
    {
        ObjectInstance* instance = collector.AllocateInstance(g_SizeStruct);
        instance->SetField(g_Size_Width->SlotIndex, collector.FromValue(static_cast<std::int64_t>(width)));
        instance->SetField(g_Size_Height->SlotIndex, collector.FromValue(static_cast<std::int64_t>(height)));
        return instance;
    }

    static ObjectInstance* CreateRectInternal(GarbageCollector& collector, int32_t x, int32_t y, int32_t width, int32_t height)
    {
        ObjectInstance* instance = collector.AllocateInstance(g_RectStruct);
        instance->SetField(g_Rect_X->SlotIndex, collector.FromValue(static_cast<std::int64_t>(x)));
        instance->SetField(g_Rect_Y->SlotIndex, collector.FromValue(static_cast<std::int64_t>(y)));
        instance->SetField(g_Rect_Width->SlotIndex, collector.FromValue(static_cast<std::int64_t>(width)));
        instance->SetField(g_Rect_Height->SlotIndex, collector.FromValue(static_cast<std::int64_t>(height)));
        return instance;
    }

    bool HandleKeyEvent(InputEvent input, MethodSymbol* interfaceMethod)
    {
        if (m_runtime == nullptr || m_widget.Instance == nullptr || interfaceMethod == nullptr)
            return false;

        TypeSymbol* widgetType = const_cast<TypeSymbol*>(m_widget.Instance->getInfo());
        MethodSymbol* method = widgetType->FindInterfaceImplementation(interfaceMethod);
        if (method == nullptr)
            return false;

        GarbageCollector& collector = m_runtime->GetGarbageCollector();
        ObjectInstance* inputArg = CreateInputEventInternal(collector, input);
        std::vector<ObjectInstance*> args = { m_widget.Instance, inputArg };

        ObjectInstance* result = m_runtime->InvokeMethod(method, args.data(), args.size());
        if (result == nullptr)
            return false;

        return result->AsBoolean();
    }

    static ObjectInstance* CreateInputEventInternal(GarbageCollector& collector, const InputEvent& input)
    {
        ObjectInstance* instance = collector.AllocateInstance(g_InputEventStruct);
        instance->SetField(g_InputEvent_Modifier->SlotIndex, collector.FromValue(static_cast<std::int64_t>(input.Modifier)));
        instance->SetField(g_InputEvent_Key->SlotIndex, collector.FromValue(static_cast<std::int64_t>(input.Key)));
        instance->SetField(g_InputEvent_Char->SlotIndex, collector.FromValue(input.Char));
        instance->SetField(g_InputEvent_Pressed->SlotIndex, collector.FromValue(input.Pressed));
        return instance;
    }
};

// ============================================================================
// ControlFactory
// ============================================================================

static ObjectInstance* shard_ControlFactory_FromWidget(const CallState& context) noexcept(false)
{
	ObjectInstance* widget = context.Args[0];
	if (widget == nullptr || widget == GarbageCollector::NullInstance)
		throw std::runtime_error("FromWidget: widget is null");

	TypeSymbol* widgetType = const_cast<TypeSymbol*>(widget->getInfo());
	if (!widgetType->Interfaces.empty())
	{
		bool implementsIWidget = false;
		for (TypeSymbol* iface : widgetType->Interfaces)
		{
			if (iface == g_IWidget)
			{
				implementsIWidget = true;
				break;
			}
		}

		if (!implementsIWidget)
			throw std::runtime_error("FromWidget: object does not implement IWidget");
	}

	ControlBase* native = new ScriptControl(context.Runtimer, widget);
	ObjectInstance* instance = context.Collector.AllocateInstance(g_ControlClass);
	RegisterControlPtr(context, instance, native);
	return instance;
}

// ============================================================================
// HostApplication wrappers (singleton, all methods exposed as static)
// ============================================================================

static ObjectInstance* shard_HostApplication_EnterTerminal(const CallState& context) noexcept(false)
{
	HostApplication::Current().EnterTerminal();
	DispatchTimer::Current().SetUIThread();
	return nullptr;
}

static ObjectInstance* shard_HostApplication_ExitTerminal(const CallState& context) noexcept(false)
{
	HostApplication::Current().ExitTerminal();
	return nullptr;
}

static ObjectInstance* shard_HostApplication_RequestStop(const CallState& context) noexcept(false)
{
	HostApplication::Current().RequestStop();
	return nullptr;
}

static ObjectInstance* shard_HostApplication_RequestStopAfter(const CallState& context) noexcept(false)
{
	std::int64_t milliseconds = context.Args[0]->AsInteger();
	std::thread([milliseconds]()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
		DispatchTimer::Current().InvokeAsync([]()
		{
			HostApplication::Current().RequestStop();
		});
	}).detach();

	return nullptr;
}

static ObjectInstance* shard_HostApplication_RunUILoop(const CallState& context) noexcept(false)
{
	ObjectInstance* rootInstance = context.Args[0];
	ControlBase* root = ReleaseControlPtr(rootInstance);
	if (root == nullptr)
		throw std::runtime_error("RunUILoop root is null");

	HostApplication::Current().RunUILoop(std::unique_ptr<VisualTreeNode>(root));
	VisualTree::Current().PopLayer();
	return nullptr;
}

// ============================================================================
// StackPanel wrapper
// ============================================================================

static ObjectInstance* shard_StackPanel_init(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* panel = new StackPanel();
	RegisterControlPtr(context, instance, panel);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_AddChild(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	ObjectInstance* childInstance = context.Args[1];

	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(instance));
	ControlBase* child = ReleaseControlPtr(childInstance);
	if (panel == nullptr || child == nullptr)
		throw std::runtime_error("StackPanel.AddChild received a null control");

	child->SetParent(panel);

	panel->AddChild(std::unique_ptr<ControlBase>(child));
	return instance;
}

static ObjectInstance* shard_StackPanel_Clear(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(instance));
	if (panel != nullptr)
		panel->Clear();
	return nullptr;
}

static HorizontalAlign ToHorizontalAlign(std::int64_t value)
{
	return static_cast<HorizontalAlign>(value);
}

static VerticalAlign ToVerticalAlign(std::int64_t value)
{
	return static_cast<VerticalAlign>(value);
}

static Orientation ToOrientation(std::int64_t value)
{
	return static_cast<Orientation>(value);
}

static ObjectInstance* shard_StackPanel_HorizontalContentAlignment_get(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel == nullptr)
		return MakeEnumValue(context.Collector, gHorizontalAlignmentEnum, 0);

	return MakeEnumValue(context.Collector, gHorizontalAlignmentEnum, static_cast<std::int64_t>(panel->HorizontalContentAlignment.Get()));
}

static ObjectInstance* shard_StackPanel_HorizontalContentAlignment_set(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->HorizontalContentAlignment.Set(ToHorizontalAlign(context.Args[1]->AsInteger()));
	return nullptr;
}

static ObjectInstance* shard_StackPanel_VerticalContentAlignment_get(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel == nullptr)
		return MakeEnumValue(context.Collector, gVerticalAlignmentEnum, 0);

	return MakeEnumValue(context.Collector, gVerticalAlignmentEnum, static_cast<std::int64_t>(panel->VerticalContentAlignment.Get()));
}

static ObjectInstance* shard_StackPanel_VerticalContentAlignment_set(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->VerticalContentAlignment.Set(ToVerticalAlign(context.Args[1]->AsInteger()));
	return nullptr;
}

static ObjectInstance* shard_StackPanel_ContentOrientation_get(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel == nullptr)
		return MakeEnumValue(context.Collector, gOrientationEnum, 0);

	return MakeEnumValue(context.Collector, gOrientationEnum, static_cast<std::int64_t>(panel->ContentOrientation.Get()));
}

static ObjectInstance* shard_StackPanel_ContentOrientation_set(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->ContentOrientation.Set(ToOrientation(context.Args[1]->AsInteger()));
	return nullptr;
}

static ObjectInstance* shard_Control_HorizontalAlignment_get(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control == nullptr)
		return MakeEnumValue(context.Collector, gHorizontalAlignmentEnum, 0);

	return MakeEnumValue(context.Collector, gHorizontalAlignmentEnum, static_cast<std::int64_t>(control->HorizontalAlignment.Get()));
}

static ObjectInstance* shard_Control_HorizontalAlignment_set(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(ToHorizontalAlign(context.Args[1]->AsInteger()));
	return nullptr;
}

static ObjectInstance* shard_Control_VerticalAlignment_get(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control == nullptr)
		return MakeEnumValue(context.Collector, gVerticalAlignmentEnum, 0);

	return MakeEnumValue(context.Collector, gVerticalAlignmentEnum, static_cast<std::int64_t>(control->VerticalAlignment.Get()));
}

static ObjectInstance* shard_Control_VerticalAlignment_set(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(ToVerticalAlign(context.Args[1]->AsInteger()));
	return nullptr;
}

// ============================================================================
// Label wrapper
// ============================================================================

static ObjectInstance* shard_Label_init(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* label = new Label();
	RegisterControlPtr(context, instance, label);
	return nullptr;
}

static ObjectInstance* shard_Label_Text_get(const CallState& context) noexcept(false)
{
	auto* label = static_cast<Label*>(ExtractControlPtr(context.Args[0]));
	if (label == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(label->Text.Get());
}

static ObjectInstance* shard_Label_Text_set(const CallState& context) noexcept(false)
{
	auto* label = static_cast<Label*>(ExtractControlPtr(context.Args[0]));
	if (label != nullptr)
		label->Text.Set(context.Args[1]->AsString());
	return nullptr;
}

// ============================================================================
// Button wrapper
// ============================================================================

static ObjectInstance* shard_Button_init(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* button = new Button();

	RegisterControlPtr(context, instance, button);

	button->Clicked += [instance]()
	{
		ControlHolder* holder = GetHolder(instance);
		if (holder == nullptr || !holder->ClickedDelegate.IsValid())
			return;

		holder->ClickedDelegate();
	};

	return nullptr;
}

static ObjectInstance* shard_Button_SetClicked(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	ObjectInstance* delegateInstance = context.Args[1];

	ControlHolder* holder = GetHolder(instance);
	if (holder == nullptr)
		throw std::runtime_error("Button wrapper has no native holder");

	if (delegateInstance == nullptr || delegateInstance == GarbageCollector::NullInstance)
	{
		holder->ClickedDelegate = DelegateRef();
		return instance;
	}

	holder->ClickedDelegate = WrapDelegate(context, delegateInstance);
	return instance;
}

static ObjectInstance* shard_Button_Text_get(const CallState& context) noexcept(false)
{
	auto* button = static_cast<Button*>(ExtractControlPtr(context.Args[0]));
	if (button == nullptr)
		return context.Collector.FromValue(std::wstring());

	return context.Collector.FromValue(button->Text.Get());
}

static ObjectInstance* shard_Button_Text_set(const CallState& context) noexcept(false)
{
	auto* button = static_cast<Button*>(ExtractControlPtr(context.Args[0]));
	if (button != nullptr)
		button->Text.Set(context.Args[1]->AsString());
	return nullptr;
}

// -----------------------------------------------------------------------------
// Fluent builder-style setters
// -----------------------------------------------------------------------------

static ObjectInstance* shard_StackPanel_SetOrientation(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(instance));
	if (panel != nullptr)
		panel->ContentOrientation.Set(ToOrientation(context.Args[1]->AsInteger()));
	
	return instance;
}

static ObjectInstance* shard_StackPanel_SetHorizontalContentAlignment(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(instance));
	if (panel != nullptr)
		panel->HorizontalContentAlignment.Set(ToHorizontalAlign(context.Args[1]->AsInteger()));
	
	return instance;
}

static ObjectInstance* shard_StackPanel_SetVerticalContentAlignment(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(instance));
	if (panel != nullptr)
		panel->VerticalContentAlignment.Set(ToVerticalAlign(context.Args[1]->AsInteger()));
	
	return instance;
}

static ObjectInstance* shard_Label_SetText(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* label = static_cast<Label*>(ExtractControlPtr(instance));
	if (label != nullptr)
		label->Text.Set(context.Args[1]->AsString());
	
	return instance;
}

static ObjectInstance* shard_Button_SetText(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* button = static_cast<Button*>(ExtractControlPtr(instance));
	if (button != nullptr)
		button->Text.Set(context.Args[1]->AsString());
	return instance;
}

static ObjectInstance* shard_Control_SetHorizontalAlignment(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* control = ExtractControlPtr(instance);
	if (control != nullptr)
		control->HorizontalAlignment.Set(ToHorizontalAlign(context.Args[1]->AsInteger()));
	
	return instance;
}

static ObjectInstance* shard_Control_SetVerticalAlignment(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* control = ExtractControlPtr(instance);
	if (control != nullptr)
		control->VerticalAlignment.Set(ToVerticalAlign(context.Args[1]->AsInteger()));
	
	return instance;
}

// ============================================================================
// ShardScript library metadata and entry point
// ============================================================================

SHARDLIB_GETMETADATA
{
	lib.Name = L"shard.terminality";
	lib.Description = L"Terminality TUI framework bindings for ShardScript";
	lib.Version = L"0.1.0";
}

SHARDLIB_ENTRYPOINT
{
	SymbolBuilder<NamespaceSymbol> terminality(context, L"terminality");

	// ------------------------------------------------------------------------
	// Enums
	// ------------------------------------------------------------------------
	gHorizontalAlignmentEnum = terminality.AddEnum(L"HorizontalAlignment")
		.AddValue(L"Left", static_cast<std::int64_t>(HorizontalAlign::Left))
		.AddValue(L"Center", static_cast<std::int64_t>(HorizontalAlign::Center))
		.AddValue(L"Right", static_cast<std::int64_t>(HorizontalAlign::Right))
		.AddValue(L"Stretch", static_cast<std::int64_t>(HorizontalAlign::Stretch));

	gVerticalAlignmentEnum = terminality.AddEnum(L"VerticalAlignment")
		.AddValue(L"Top", static_cast<std::int64_t>(VerticalAlign::Top))
		.AddValue(L"Center", static_cast<std::int64_t>(VerticalAlign::Center))
		.AddValue(L"Bottom", static_cast<std::int64_t>(VerticalAlign::Bottom))
		.AddValue(L"Stretch", static_cast<std::int64_t>(VerticalAlign::Stretch));

	gOrientationEnum = terminality.AddEnum(L"Orientation")
		.AddValue(L"Vertical", static_cast<std::int64_t>(Orientation::Vertical))
		.AddValue(L"Horizontal", static_cast<std::int64_t>(Orientation::Horizontal));

	// ------------------------------------------------------------------------
	// Color enum
	// ------------------------------------------------------------------------
	g_ColorEnum = terminality.AddEnum(L"Color")
		.AddValue(L"Transparent", static_cast<std::int64_t>(Color::TRANSPARENT))
		.AddValue(L"Black", static_cast<std::int64_t>(Color::BLACK))
		.AddValue(L"DarkBlue", static_cast<std::int64_t>(Color::DARK_BLUE))
		.AddValue(L"DarkGreen", static_cast<std::int64_t>(Color::DARK_GREEN))
		.AddValue(L"DarkCyan", static_cast<std::int64_t>(Color::DARK_CYAN))
		.AddValue(L"DarkRed", static_cast<std::int64_t>(Color::DARK_RED))
		.AddValue(L"DarkMagenta", static_cast<std::int64_t>(Color::DARK_MAGENTA))
		.AddValue(L"DarkYellow", static_cast<std::int64_t>(Color::DARK_YELLOW))
		.AddValue(L"LightGray", static_cast<std::int64_t>(Color::LIGHT_GRAY))
		.AddValue(L"DarkGray", static_cast<std::int64_t>(Color::DARK_GRAY))
		.AddValue(L"Blue", static_cast<std::int64_t>(Color::BLUE))
		.AddValue(L"Green", static_cast<std::int64_t>(Color::GREEN))
		.AddValue(L"Cyan", static_cast<std::int64_t>(Color::CYAN))
		.AddValue(L"Red", static_cast<std::int64_t>(Color::RED))
		.AddValue(L"Magenta", static_cast<std::int64_t>(Color::MAGENTA))
		.AddValue(L"Yellow", static_cast<std::int64_t>(Color::YELLOW))
		.AddValue(L"White", static_cast<std::int64_t>(Color::WHITE));

	// ------------------------------------------------------------------------
	// InputKey enum (subset of commonly used keys)
	// ------------------------------------------------------------------------
	g_InputKeyEnum = terminality.AddEnum(L"InputKey")
		.AddValue(L"None", static_cast<std::int64_t>(InputKey::None))
		.AddValue(L"Char", static_cast<std::int64_t>(InputKey::CHAR))
		.AddValue(L"Return", static_cast<std::int64_t>(InputKey::RETURN))
		.AddValue(L"Escape", static_cast<std::int64_t>(InputKey::ESCAPE))
		.AddValue(L"Back", static_cast<std::int64_t>(InputKey::BACK))
		.AddValue(L"Tab", static_cast<std::int64_t>(InputKey::TAB))
		.AddValue(L"Space", static_cast<std::int64_t>(InputKey::SPACE))
		.AddValue(L"Left", static_cast<std::int64_t>(InputKey::LEFT))
		.AddValue(L"Up", static_cast<std::int64_t>(InputKey::UP))
		.AddValue(L"Right", static_cast<std::int64_t>(InputKey::RIGHT))
		.AddValue(L"Down", static_cast<std::int64_t>(InputKey::DOWN))
		.AddValue(L"F1", static_cast<std::int64_t>(InputKey::F1))
		.AddValue(L"F2", static_cast<std::int64_t>(InputKey::F2))
		.AddValue(L"F3", static_cast<std::int64_t>(InputKey::F3))
		.AddValue(L"F4", static_cast<std::int64_t>(InputKey::F4))
		.AddValue(L"F5", static_cast<std::int64_t>(InputKey::F5))
		.AddValue(L"F6", static_cast<std::int64_t>(InputKey::F6))
		.AddValue(L"F7", static_cast<std::int64_t>(InputKey::F7))
		.AddValue(L"F8", static_cast<std::int64_t>(InputKey::F8))
		.AddValue(L"F9", static_cast<std::int64_t>(InputKey::F9))
		.AddValue(L"F10", static_cast<std::int64_t>(InputKey::F10))
		.AddValue(L"F11", static_cast<std::int64_t>(InputKey::F11))
		.AddValue(L"F12", static_cast<std::int64_t>(InputKey::F12));

	// ------------------------------------------------------------------------
	// InputModifier flags enum
	// ------------------------------------------------------------------------
	g_InputModifierEnum = terminality.AddEnum(L"InputModifier", true)
		.AddValue(L"None", static_cast<std::int64_t>(InputModifier::None))
		.AddValue(L"LeftAlt", static_cast<std::int64_t>(InputModifier::LeftAlt))
		.AddValue(L"RightAlt", static_cast<std::int64_t>(InputModifier::RightAlt))
		.AddValue(L"Alt", static_cast<std::int64_t>(InputModifier::Alt))
		.AddValue(L"LeftCtrl", static_cast<std::int64_t>(InputModifier::LeftCtrl))
		.AddValue(L"RightCtrl", static_cast<std::int64_t>(InputModifier::RightCtrl))
		.AddValue(L"Ctrl", static_cast<std::int64_t>(InputModifier::Ctrl))
		.AddValue(L"Shift", static_cast<std::int64_t>(InputModifier::Shift))
		.AddValue(L"NumLockOn", static_cast<std::int64_t>(InputModifier::NumLockOn))
		.AddValue(L"ScrollLockOn", static_cast<std::int64_t>(InputModifier::ScrollLockOn))
		.AddValue(L"CapsLockOn", static_cast<std::int64_t>(InputModifier::CapsLockOn))
		.AddValue(L"Special", static_cast<std::int64_t>(InputModifier::Special));

	// ------------------------------------------------------------------------
	// Geometry structs
	// ------------------------------------------------------------------------
	{
		SymbolBuilder<StructSymbol> pointStruct = terminality.AddStruct(L"Point");
		g_PointStruct = pointStruct.Get();
		g_Point_X = pointStruct.AddField(L"X", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Point_Y = pointStruct.AddField(L"Y", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();

		g_PointStruct_init = pointStruct.AddInit(ACS_PUBLIC)
			.AddParameter(L"x", TYPE_INT)
			.AddParameter(L"y", TYPE_INT)
			.SetCallback(&shard_Point_Create);
	}

	{
		SymbolBuilder<StructSymbol> sizeStruct = terminality.AddStruct(L"Size");
		g_SizeStruct = sizeStruct.Get();
		g_Size_Width = sizeStruct.AddField(L"Width", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Size_Height = sizeStruct.AddField(L"Height", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();

		g_SizeStruct_init = sizeStruct.AddInit(ACS_PUBLIC)
			.AddParameter(L"width", TYPE_INT)
			.AddParameter(L"height", TYPE_INT)
			.SetCallback(&shard_Size_Create);
	}

	{
		SymbolBuilder<StructSymbol> rectStruct = terminality.AddStruct(L"Rect");
		g_RectStruct = rectStruct.Get();
		g_Rect_X = rectStruct.AddField(L"X", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Rect_Y = rectStruct.AddField(L"Y", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Rect_Width = rectStruct.AddField(L"Width", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Rect_Height = rectStruct.AddField(L"Height", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();

		g_RectStruct_init = rectStruct.AddInit(ACS_PUBLIC)
			.AddParameter(L"x", TYPE_INT)
			.AddParameter(L"y", TYPE_INT)
			.AddParameter(L"width", TYPE_INT)
			.AddParameter(L"height", TYPE_INT)
			.SetCallback(&shard_Rect_Create);
	}

	{
		SymbolBuilder<StructSymbol> thicknessStruct = terminality.AddStruct(L"Thickness");
		g_ThicknessStruct = thicknessStruct.Get();
		g_Thickness_Left = thicknessStruct.AddField(L"Left", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Thickness_Top = thicknessStruct.AddField(L"Top", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Thickness_Right = thicknessStruct.AddField(L"Right", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_Thickness_Bottom = thicknessStruct.AddField(L"Bottom", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC).Get();

		g_ThicknessStruct_init = thicknessStruct.AddInit(ACS_PUBLIC)
			.AddParameter(L"left", TYPE_INT)
			.AddParameter(L"top", TYPE_INT)
			.AddParameter(L"right", TYPE_INT)
			.AddParameter(L"bottom", TYPE_INT)
			.SetCallback(&shard_Thickness_Create);
	}

	{
		SymbolBuilder<StructSymbol> inputEventStruct = terminality.AddStruct(L"InputEvent");
		g_InputEventStruct = inputEventStruct.Get();
		g_InputEvent_Modifier = inputEventStruct.AddField(L"Modifier", g_InputModifierEnum, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_InputEvent_Key = inputEventStruct.AddField(L"Key", g_InputKeyEnum, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_InputEvent_Char = inputEventStruct.AddField(L"Char", TYPE_CHAR, LINK_INSTANCE, ACS_PUBLIC).Get();
		g_InputEvent_Pressed = inputEventStruct.AddField(L"Pressed", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC).Get();
	}

	// ------------------------------------------------------------------------
	// IRenderContext interface and RenderContext class
	// ------------------------------------------------------------------------
	{
		SymbolBuilder<InterfaceSymbol> renderContextInterface = terminality.AddInterface(L"IRenderContext");
		g_IRenderContext = renderContextInterface.Get();

		g_IRenderContext_ContextRect = renderContextInterface
			.AddMethod(L"ContextRect", g_RectStruct, LINK_INSTANCE, ACS_PUBLIC)
			.Get();
		g_IRenderContext_ContextRect->IsAbstract = true;

		g_IRenderContext_SetCell = renderContextInterface
			.AddMethod(L"SetCell", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"x", TYPE_INT)
			.AddParameter(L"y", TYPE_INT)
			.AddParameter(L"symbol", TYPE_CHAR)
			.AddParameter(L"fg", g_ColorEnum)
			.AddParameter(L"bg", g_ColorEnum)
			.Get();
		g_IRenderContext_SetCell->IsAbstract = true;

		g_IRenderContext_RenderText = renderContextInterface
			.AddMethod(L"RenderText", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"point", g_PointStruct)
			.AddParameter(L"text", TYPE_STRING)
			.AddParameter(L"fg", g_ColorEnum)
			.AddParameter(L"bg", g_ColorEnum)
			.Get();
		g_IRenderContext_RenderText->IsAbstract = true;

		g_IRenderContext_RenderRectangle = renderContextInterface
			.AddMethod(L"RenderRectangle", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"point", g_PointStruct)
			.AddParameter(L"size", g_SizeStruct)
			.AddParameter(L"fg", g_ColorEnum)
			.AddParameter(L"bg", g_ColorEnum)
			.Get();
		g_IRenderContext_RenderRectangle->IsAbstract = true;

		g_IRenderContext_CreateInner = renderContextInterface
			.AddMethod(L"CreateInner", g_IRenderContext, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"rect", g_RectStruct)
			.Get();
		g_IRenderContext_CreateInner->IsAbstract = true;
	}

	{
		SymbolBuilder<ClassSymbol> renderContextClass = terminality.AddClass(L"RenderContext");
		g_RenderContextClass = renderContextClass.Get();
		g_RenderContext_PtrField = renderContextClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE).Get();

		renderContextClass.Implements(g_IRenderContext);

		renderContextClass.AddMethod(L"ContextRect", g_RectStruct, LINK_INSTANCE, ACS_PUBLIC)
			.IsImplementationOf(g_IRenderContext_ContextRect)
			.SetCallback(&shard_RenderContext_ContextRect);

		renderContextClass.AddMethod(L"SetCell", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"x", TYPE_INT)
			.AddParameter(L"y", TYPE_INT)
			.AddParameter(L"symbol", TYPE_CHAR)
			.AddParameter(L"fg", g_ColorEnum)
			.AddParameter(L"bg", g_ColorEnum)
			.IsImplementationOf(g_IRenderContext_SetCell)
			.SetCallback(&shard_RenderContext_SetCell);

		renderContextClass.AddMethod(L"RenderText", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"point", g_PointStruct)
			.AddParameter(L"text", TYPE_STRING)
			.AddParameter(L"fg", g_ColorEnum)
			.AddParameter(L"bg", g_ColorEnum)
			.IsImplementationOf(g_IRenderContext_RenderText)
			.SetCallback(&shard_RenderContext_RenderText);

		renderContextClass.AddMethod(L"RenderRectangle", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"point", g_PointStruct)
			.AddParameter(L"size", g_SizeStruct)
			.AddParameter(L"fg", g_ColorEnum)
			.AddParameter(L"bg", g_ColorEnum)
			.IsImplementationOf(g_IRenderContext_RenderRectangle)
			.SetCallback(&shard_RenderContext_RenderRectangle);

		renderContextClass.AddMethod(L"CreateInner", g_IRenderContext, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"rect", g_RectStruct)
			.IsImplementationOf(g_IRenderContext_CreateInner)
			.SetCallback(&shard_RenderContext_CreateInner);
	}

	// ------------------------------------------------------------------------
	// IWidget interface
	// ------------------------------------------------------------------------
	{
		SymbolBuilder<InterfaceSymbol> widgetInterface = terminality.AddInterface(L"IWidget");
		g_IWidget = widgetInterface.Get();

		g_IWidget_Measure = widgetInterface
			.AddMethod(L"Measure", g_SizeStruct, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"availableSize", g_SizeStruct)
			.Get();
		g_IWidget_Measure->IsAbstract = true;

		g_IWidget_Arrange = widgetInterface
			.AddMethod(L"Arrange", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"contentRect", g_RectStruct)
			.Get();
		g_IWidget_Arrange->IsAbstract = true;

		g_IWidget_Render = widgetInterface
			.AddMethod(L"Render", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"context", g_IRenderContext)
			.Get();
		g_IWidget_Render->IsAbstract = true;

		g_IWidget_OnKeyDown = widgetInterface
			.AddMethod(L"OnKeyDown", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"input", g_InputEventStruct)
			.Get();
		g_IWidget_OnKeyDown->IsAbstract = true;

		g_IWidget_OnKeyUp = widgetInterface
			.AddMethod(L"OnKeyUp", TYPE_BOOL, LINK_INSTANCE, ACS_PUBLIC)
			.AddParameter(L"input", g_InputEventStruct)
			.Get();
		g_IWidget_OnKeyUp->IsAbstract = true;
	}

	// ------------------------------------------------------------------------
	// Control base class and ControlFactory
	// ------------------------------------------------------------------------
	{
		SymbolBuilder<ClassSymbol> controlClass = terminality.AddClass(L"Control");
		g_ControlClass = controlClass.Get();
		g_Control_PtrField = controlClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE).Get();
	}

	{
		SymbolBuilder<ClassSymbol> factoryClass = terminality.AddClass(L"ControlFactory");
		g_ControlFactoryClass = factoryClass.Get();

		factoryClass.AddMethod(L"FromWidget", g_ControlClass, LINK_STATIC, ACS_PUBLIC)
			.AddParameter(L"widget", g_IWidget)
			.SetCallback(&shard_ControlFactory_FromWidget);
	}

	// ------------------------------------------------------------------------
	// HostApplication (singleton exposed as a static class)
	// ------------------------------------------------------------------------
	SymbolBuilder<ClassSymbol> hostAppClass = terminality.AddClass(L"HostApplication");

	hostAppClass.AddMethod(L"EnterTerminal", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		.SetCallback(&shard_HostApplication_EnterTerminal);

	hostAppClass.AddMethod(L"ExitTerminal", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		.SetCallback(&shard_HostApplication_ExitTerminal);

	hostAppClass.AddMethod(L"RequestStop", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		.SetCallback(&shard_HostApplication_RequestStop);

	hostAppClass.AddMethod(L"RequestStopAfter", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		.AddParameter(L"milliseconds", TYPE_INT)
		.SetCallback(&shard_HostApplication_RequestStopAfter);

	hostAppClass.AddMethod(L"RunUILoop", TYPE_VOID, LINK_STATIC, ACS_PUBLIC)
		.AddParameter(L"root", TYPE_ANY)
		.SetCallback(&shard_HostApplication_RunUILoop);

	// ------------------------------------------------------------------------
	// StackPanel
	// ------------------------------------------------------------------------
	SymbolBuilder<ClassSymbol> stackPanelClass = terminality.AddClass(L"StackPanel");
	g_StackPanelClass = stackPanelClass.Get();

	stackPanelClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE);

	stackPanelClass.AddInit(ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_init);

	stackPanelClass.AddMethod(L"AddChild", g_StackPanelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"child", TYPE_ANY)
		.SetCallback(&shard_StackPanel_AddChild);

	stackPanelClass.AddMethod(L"Clear", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Clear);

	{
		auto prop = stackPanelClass.AddProperty(L"HorizontalAlignment", gHorizontalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_Control_HorizontalAlignment_get);
		prop.AddSetter().SetCallback(&shard_Control_HorizontalAlignment_set);
	}
	{
		auto prop = stackPanelClass.AddProperty(L"VerticalAlignment", gVerticalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_Control_VerticalAlignment_get);
		prop.AddSetter().SetCallback(&shard_Control_VerticalAlignment_set);
	}
	{
		auto prop = stackPanelClass.AddProperty(L"HorizontalContentAlignment", gHorizontalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_StackPanel_HorizontalContentAlignment_get);
		prop.AddSetter().SetCallback(&shard_StackPanel_HorizontalContentAlignment_set);
	}
	{
		auto prop = stackPanelClass.AddProperty(L"VerticalContentAlignment", gVerticalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_StackPanel_VerticalContentAlignment_get);
		prop.AddSetter().SetCallback(&shard_StackPanel_VerticalContentAlignment_set);
	}
	{
		auto prop = stackPanelClass.AddProperty(L"ContentOrientation", gOrientationEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_StackPanel_ContentOrientation_get);
		prop.AddSetter().SetCallback(&shard_StackPanel_ContentOrientation_set);
	}

	stackPanelClass.AddMethod(L"SetOrientation", g_StackPanelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"orientation", gOrientationEnum)
		.SetCallback(&shard_StackPanel_SetOrientation);

	stackPanelClass.AddMethod(L"SetHorizontalContentAlignment", g_StackPanelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gHorizontalAlignmentEnum)
		.SetCallback(&shard_StackPanel_SetHorizontalContentAlignment);

	stackPanelClass.AddMethod(L"SetVerticalContentAlignment", g_StackPanelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gVerticalAlignmentEnum)
		.SetCallback(&shard_StackPanel_SetVerticalContentAlignment);

	stackPanelClass.AddMethod(L"SetHorizontalAlignment", g_StackPanelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gHorizontalAlignmentEnum)
		.SetCallback(&shard_Control_SetHorizontalAlignment);

	stackPanelClass.AddMethod(L"SetVerticalAlignment", g_StackPanelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gVerticalAlignmentEnum)
		.SetCallback(&shard_Control_SetVerticalAlignment);

	// ------------------------------------------------------------------------
	// Label
	// ------------------------------------------------------------------------
	SymbolBuilder<ClassSymbol> labelClass = terminality.AddClass(L"Label");
	g_LabelClass = labelClass.Get();

	labelClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE);

	labelClass.AddInit(ACS_PUBLIC)
		.SetCallback(&shard_Label_init);

	{
		auto textProp = labelClass.AddProperty(L"Text", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);
		textProp.AddGetter().SetCallback(&shard_Label_Text_get);
		textProp.AddSetter().SetCallback(&shard_Label_Text_set);
	}

	{
		auto prop = labelClass.AddProperty(L"HorizontalAlignment", gHorizontalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_Control_HorizontalAlignment_get);
		prop.AddSetter().SetCallback(&shard_Control_HorizontalAlignment_set);
	}
	{
		auto prop = labelClass.AddProperty(L"VerticalAlignment", gVerticalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_Control_VerticalAlignment_get);
		prop.AddSetter().SetCallback(&shard_Control_VerticalAlignment_set);
	}

	labelClass.AddMethod(L"SetText", g_LabelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"text", TYPE_STRING)
		.SetCallback(&shard_Label_SetText);

	labelClass.AddMethod(L"SetHorizontalAlignment", g_LabelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gHorizontalAlignmentEnum)
		.SetCallback(&shard_Control_SetHorizontalAlignment);

	labelClass.AddMethod(L"SetVerticalAlignment", g_LabelClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gVerticalAlignmentEnum)
		.SetCallback(&shard_Control_SetVerticalAlignment);

	// ------------------------------------------------------------------------
	// Button
	// ------------------------------------------------------------------------
	SymbolBuilder<ClassSymbol> buttonClass = terminality.AddClass(L"Button");
	g_ButtonClass = buttonClass.Get();

	buttonClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE);

	buttonClass.AddInit(ACS_PUBLIC)
		.SetCallback(&shard_Button_init);

	{
		auto textProp = buttonClass.AddProperty(L"Text", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);
		textProp.AddGetter().SetCallback(&shard_Button_Text_get);
		textProp.AddSetter().SetCallback(&shard_Button_Text_set);
	}

	buttonClass.AddMethod(L"SetClicked", g_ButtonClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"action", TYPE_ANY)
		.SetCallback(&shard_Button_SetClicked);

	buttonClass.AddMethod(L"SetText", g_ButtonClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"text", TYPE_STRING)
		.SetCallback(&shard_Button_SetText);

	buttonClass.AddMethod(L"SetHorizontalAlignment", g_ButtonClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gHorizontalAlignmentEnum)
		.SetCallback(&shard_Control_SetHorizontalAlignment);

	buttonClass.AddMethod(L"SetVerticalAlignment", g_ButtonClass, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"alignment", gVerticalAlignmentEnum)
		.SetCallback(&shard_Control_SetVerticalAlignment);

	{
		auto prop = buttonClass.AddProperty(L"HorizontalAlignment", gHorizontalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_Control_HorizontalAlignment_get);
		prop.AddSetter().SetCallback(&shard_Control_HorizontalAlignment_set);
	}
	{
		auto prop = buttonClass.AddProperty(L"VerticalAlignment", gVerticalAlignmentEnum, LINK_INSTANCE, ACS_PUBLIC);
		prop.AddGetter().SetCallback(&shard_Control_VerticalAlignment_get);
		prop.AddSetter().SetCallback(&shard_Control_VerticalAlignment_set);
	}
}
