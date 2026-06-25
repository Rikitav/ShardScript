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
	ObjectInstance* ClickedDelegate = nullptr;

	ControlHolder() = default;
	ControlHolder(ControlBase* control, bool owned)
		: Control(control), Owned(owned) { }

	~ControlHolder()
	{
		if (Owned && Control != nullptr)
			delete Control;

		if (ClickedDelegate != nullptr && ClickedDelegate != GarbageCollector::NullInstance)
			ClickedDelegate->DecrementReference();
	}
};

static FieldSymbol* FindFieldByName(TypeSymbol* type, const std::wstring& name)
{
	for (FieldSymbol* field : type->Fields)
		if (field->Name == name)
			return field;

	return nullptr;
}

static ControlHolder* GetHolder(ObjectInstance* instance)
{
	TypeSymbol* type = const_cast<TypeSymbol*>(instance->getInfo());
	FieldSymbol* ptrField = FindFieldByName(type, L"_ptr");
	if (ptrField == nullptr)
		throw std::runtime_error("Terminality wrapper is missing the `_ptr` field");

	ObjectInstance* ptrInstance = instance->GetField(ptrField);
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
	FieldSymbol* ptrField = FindFieldByName(type, L"_ptr");
	if (ptrField == nullptr)
		throw std::runtime_error("Terminality wrapper is missing the `_ptr` field");

	ControlHolder* holder = new ControlHolder{ native, true };
	ObjectInstance* wrapper = context.Collector.FromNint(holder, false);
	instance->SetField(ptrField, wrapper);
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
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Clear(const CallState& context) noexcept(false)
{
	ObjectInstance* instance = context.Args[0];
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(instance));
	if (panel != nullptr)
		panel->Clear();
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_HorizontalContentAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->HorizontalContentAlignment.Set(HorizontalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_HorizontalContentAlignment_Center(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->HorizontalContentAlignment.Set(HorizontalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_HorizontalContentAlignment_Left(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->HorizontalContentAlignment.Set(HorizontalAlign::Left);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_HorizontalContentAlignment_Right(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->HorizontalContentAlignment.Set(HorizontalAlign::Right);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_VerticalContentAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->VerticalContentAlignment.Set(VerticalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_VerticalContentAlignment_Center(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->VerticalContentAlignment.Set(VerticalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_VerticalContentAlignment_Top(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->VerticalContentAlignment.Set(VerticalAlign::Top);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_VerticalContentAlignment_Bottom(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->VerticalContentAlignment.Set(VerticalAlign::Bottom);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_ContentOrientation_Vertical(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->ContentOrientation.Set(Orientation::Vertical);
	return nullptr;
}

static ObjectInstance* shard_StackPanel_Set_ContentOrientation_Horizontal(const CallState& context) noexcept(false)
{
	auto* panel = static_cast<StackPanel*>(ExtractControlPtr(context.Args[0]));
	if (panel != nullptr) panel->ContentOrientation.Set(Orientation::Horizontal);
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

static ObjectInstance* shard_Label_Set_HorizontalAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_Label_Set_HorizontalAlignment_Center(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_Label_Set_HorizontalAlignment_Left(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Left);
	return nullptr;
}

static ObjectInstance* shard_Label_Set_HorizontalAlignment_Right(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Right);
	return nullptr;
}

static ObjectInstance* shard_Label_Set_VerticalAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_Label_Set_VerticalAlignment_Center(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_Label_Set_VerticalAlignment_Top(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Top);
	return nullptr;
}

static ObjectInstance* shard_Label_Set_VerticalAlignment_Bottom(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Bottom);
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

	ControlHolder* holder = GetHolder(instance);
	if (holder != nullptr)
		holder->ClickedDelegate = GarbageCollector::NullInstance;

	button->Clicked += [instance, &runtimer = context.Runtimer]()
	{
		ControlHolder* holder = GetHolder(instance);
		if (holder == nullptr || holder->ClickedDelegate == nullptr || holder->ClickedDelegate == GarbageCollector::NullInstance)
			return;

		MethodSymbol* target = holder->ClickedDelegate->DelegateTarget;
		if (target == nullptr)
			return;

		runtimer.InvokeMethod(target, nullptr, 0);
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

	if (holder->ClickedDelegate != nullptr && holder->ClickedDelegate != GarbageCollector::NullInstance)
		holder->ClickedDelegate->DecrementReference();

	holder->ClickedDelegate = delegateInstance;

	if (delegateInstance != nullptr && delegateInstance != GarbageCollector::NullInstance)
		delegateInstance->IncrementReference();

	return nullptr;
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

static ObjectInstance* shard_Button_Set_HorizontalAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_Button_Set_HorizontalAlignment_Center(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_Button_Set_HorizontalAlignment_Left(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Left);
	return nullptr;
}

static ObjectInstance* shard_Button_Set_HorizontalAlignment_Right(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Right);
	return nullptr;
}

static ObjectInstance* shard_Button_Set_VerticalAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_Button_Set_VerticalAlignment_Center(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_Button_Set_VerticalAlignment_Top(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Top);
	return nullptr;
}

static ObjectInstance* shard_Button_Set_VerticalAlignment_Bottom(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Bottom);
	return nullptr;
}

// ============================================================================
// Shared alignment helpers (used by StackPanel too for its own alignment)
// ============================================================================

static ObjectInstance* shard_Control_Set_HorizontalAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_Control_Set_HorizontalAlignment_Center(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_Control_Set_HorizontalAlignment_Left(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Left);
	return nullptr;
}

static ObjectInstance* shard_Control_Set_HorizontalAlignment_Right(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->HorizontalAlignment.Set(HorizontalAlign::Right);
	return nullptr;
}

static ObjectInstance* shard_Control_Set_VerticalAlignment_Stretch(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Stretch);
	return nullptr;
}

static ObjectInstance* shard_Control_Set_VerticalAlignment_Center(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Center);
	return nullptr;
}

static ObjectInstance* shard_Control_Set_VerticalAlignment_Top(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Top);
	return nullptr;
}

static ObjectInstance* shard_Control_Set_VerticalAlignment_Bottom(const CallState& context) noexcept(false)
{
	auto* control = ExtractControlPtr(context.Args[0]);
	if (control != nullptr) control->VerticalAlignment.Set(VerticalAlign::Bottom);
	return nullptr;
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

	stackPanelClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE);

	stackPanelClass.AddInit(ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_init);

	stackPanelClass.AddMethod(L"AddChild", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"child", TYPE_ANY)
		.SetCallback(&shard_StackPanel_AddChild);

	stackPanelClass.AddMethod(L"Clear", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Clear);

	stackPanelClass.AddMethod(L"Set_HorizontalAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_HorizontalAlignment_Stretch);
	stackPanelClass.AddMethod(L"Set_HorizontalAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_HorizontalAlignment_Center);
	stackPanelClass.AddMethod(L"Set_HorizontalAlignment_Left", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_HorizontalAlignment_Left);
	stackPanelClass.AddMethod(L"Set_HorizontalAlignment_Right", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_HorizontalAlignment_Right);

	stackPanelClass.AddMethod(L"Set_VerticalAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_VerticalAlignment_Stretch);
	stackPanelClass.AddMethod(L"Set_VerticalAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_VerticalAlignment_Center);
	stackPanelClass.AddMethod(L"Set_VerticalAlignment_Top", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_VerticalAlignment_Top);
	stackPanelClass.AddMethod(L"Set_VerticalAlignment_Bottom", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Control_Set_VerticalAlignment_Bottom);

	stackPanelClass.AddMethod(L"Set_HorizontalContentAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_HorizontalContentAlignment_Stretch);
	stackPanelClass.AddMethod(L"Set_HorizontalContentAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_HorizontalContentAlignment_Center);
	stackPanelClass.AddMethod(L"Set_HorizontalContentAlignment_Left", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_HorizontalContentAlignment_Left);
	stackPanelClass.AddMethod(L"Set_HorizontalContentAlignment_Right", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_HorizontalContentAlignment_Right);

	stackPanelClass.AddMethod(L"Set_VerticalContentAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_VerticalContentAlignment_Stretch);
	stackPanelClass.AddMethod(L"Set_VerticalContentAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_VerticalContentAlignment_Center);
	stackPanelClass.AddMethod(L"Set_VerticalContentAlignment_Top", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_VerticalContentAlignment_Top);
	stackPanelClass.AddMethod(L"Set_VerticalContentAlignment_Bottom", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_VerticalContentAlignment_Bottom);

	stackPanelClass.AddMethod(L"Set_ContentOrientation_Vertical", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_ContentOrientation_Vertical);
	stackPanelClass.AddMethod(L"Set_ContentOrientation_Horizontal", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_StackPanel_Set_ContentOrientation_Horizontal);

	// ------------------------------------------------------------------------
	// Label
	// ------------------------------------------------------------------------
	SymbolBuilder<ClassSymbol> labelClass = terminality.AddClass(L"Label");

	labelClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE);

	labelClass.AddInit(ACS_PUBLIC)
		.SetCallback(&shard_Label_init);

	{
		auto textProp = labelClass.AddProperty(L"Text", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);
		textProp.AddGetter().SetCallback(&shard_Label_Text_get);
		textProp.AddSetter().SetCallback(&shard_Label_Text_set);
	}

	labelClass.AddMethod(L"Set_HorizontalAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_HorizontalAlignment_Stretch);
	labelClass.AddMethod(L"Set_HorizontalAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_HorizontalAlignment_Center);
	labelClass.AddMethod(L"Set_HorizontalAlignment_Left", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_HorizontalAlignment_Left);
	labelClass.AddMethod(L"Set_HorizontalAlignment_Right", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_HorizontalAlignment_Right);

	labelClass.AddMethod(L"Set_VerticalAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_VerticalAlignment_Stretch);
	labelClass.AddMethod(L"Set_VerticalAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_VerticalAlignment_Center);
	labelClass.AddMethod(L"Set_VerticalAlignment_Top", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_VerticalAlignment_Top);
	labelClass.AddMethod(L"Set_VerticalAlignment_Bottom", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Label_Set_VerticalAlignment_Bottom);

	// ------------------------------------------------------------------------
	// Button
	// ------------------------------------------------------------------------
	SymbolBuilder<ClassSymbol> buttonClass = terminality.AddClass(L"Button");

	buttonClass.AddField(L"_ptr", SymbolTable::Primitives::NativeInteger, LINK_INSTANCE, ACS_PRIVATE);

	buttonClass.AddInit(ACS_PUBLIC)
		.SetCallback(&shard_Button_init);

	{
		auto textProp = buttonClass.AddProperty(L"Text", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC);
		textProp.AddGetter().SetCallback(&shard_Button_Text_get);
		textProp.AddSetter().SetCallback(&shard_Button_Text_set);
	}

	buttonClass.AddMethod(L"SetClicked", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.AddParameter(L"action", TYPE_ANY)
		.SetCallback(&shard_Button_SetClicked);

	buttonClass.AddMethod(L"Set_HorizontalAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_HorizontalAlignment_Stretch);
	buttonClass.AddMethod(L"Set_HorizontalAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_HorizontalAlignment_Center);
	buttonClass.AddMethod(L"Set_HorizontalAlignment_Left", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_HorizontalAlignment_Left);
	buttonClass.AddMethod(L"Set_HorizontalAlignment_Right", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_HorizontalAlignment_Right);

	buttonClass.AddMethod(L"Set_VerticalAlignment_Stretch", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_VerticalAlignment_Stretch);
	buttonClass.AddMethod(L"Set_VerticalAlignment_Center", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_VerticalAlignment_Center);
	buttonClass.AddMethod(L"Set_VerticalAlignment_Top", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_VerticalAlignment_Top);
	buttonClass.AddMethod(L"Set_VerticalAlignment_Bottom", TYPE_VOID, LINK_INSTANCE, ACS_PUBLIC)
		.SetCallback(&shard_Button_Set_VerticalAlignment_Bottom);
}
