#include <shard/syntax/symbols/PropertySymbol.hpp>

using namespace shard;

PropertySymbol* PropertySymbol::GenerateBackingField()
{
    FieldSymbol* backingField = new FieldSymbol(L"<" + Name + L">k__BackingField");
    backingField->Accesibility = SymbolAccesibility::Private;
    backingField->ReturnType = ReturnType;
    backingField->DefaultValueExpression = DefaultValueExpression;
    backingField->IsStatic = IsStatic;

    BackingField = backingField;
    return this;
}