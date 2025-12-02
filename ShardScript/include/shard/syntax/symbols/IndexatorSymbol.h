#pragma once
#pragma once
#include <shard/ShardScriptAPI.h>

#include <shard/syntax/symbols/PropertySymbol.h>
#include <shard/syntax/symbols/ParameterSymbol.h>

#include <vector>

namespace shard::syntax::symbols
{
    // IndexatorSymbol наследует всю семантику PropertySymbol
    // и добавляет только список параметров индексатора.
    class SHARD_API IndexatorSymbol : public PropertySymbol
    {
    public:
        std::vector<ParameterSymbol*> Parameters;

        inline IndexatorSymbol(std::wstring name)
            : PropertySymbol(std::move(name)) { }

        inline IndexatorSymbol(const IndexatorSymbol& other) = delete;

        inline ~IndexatorSymbol() override
        {
            for (ParameterSymbol* param : Parameters)
                delete param;
            // Остальное удалит деструктор PropertySymbol
        }
    };
}

