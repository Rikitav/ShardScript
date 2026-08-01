#include <cmath>

#include <ShardScript.hpp>
#include <shard/runtime/NativeHelpers.hpp>

using namespace shard;

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.math";
    lib.Description = L"High-performance native math library wrappers";
    lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> mathNamespace(context, L"math");

    mathNamespace.AddClass(L"Math", ACS_PUBLIC, LINK_STATIC, [](SymbolBuilder<ClassSymbol> math)
    {
        math.AddProperty(L"PI", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddGetter().SetCallback([](const CallState& context)
			{
                return context.Collector.FromValue(3.14159265358979323846);
            });

        math.AddProperty(L"E", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddGetter().SetCallback([](const CallState& context)
			{
                return context.Collector.FromValue(2.71828182845904523536);
            });

        math.AddMethod(L"Sin", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::sin(value));
            });

        math.AddMethod(L"Cos", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::cos(value));
            });

        math.AddMethod(L"Tan", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::tan(value));
            });

        math.AddMethod(L"Asin", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::asin(value));
            });

        math.AddMethod(L"Acos", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::acos(value));
            });

        math.AddMethod(L"Atan", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::atan(value));
            });

        math.AddMethod(L"Atan2", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"y", TYPE_DOUBLE)
            .AddParameter(L"x", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [y, x] = GetArgs<double, double>(context);
                return context.Collector.FromValue(std::atan2(y, x));
            });

        math.AddMethod(L"Pow", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"base", TYPE_DOUBLE)
            .AddParameter(L"exponent", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [base, exponent] = GetArgs<double, double>(context);
                return context.Collector.FromValue(std::pow(base, exponent));
            });

        math.AddMethod(L"Sqrt", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::sqrt(value));
            });

        math.AddMethod(L"Cbrt", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::cbrt(value));
            });

        math.AddMethod(L"Exp", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::exp(value));
            });

        math.AddMethod(L"Log", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::log(value));
            });

        math.AddMethod(L"Log10", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::log10(value));
            });

        math.AddMethod(L"Abs", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::abs(value));
            });

        math.AddMethod(L"Ceil", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::ceil(value));
            });

        math.AddMethod(L"Floor", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::floor(value));
            });

        math.AddMethod(L"Round", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [value] = GetArgs<double>(context);
                return context.Collector.FromValue(std::round(value));
            });

        math.AddMethod(L"Min", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", TYPE_DOUBLE)
            .AddParameter(L"b", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [a, b] = GetArgs<double, double>(context);
                return context.Collector.FromValue(std::fmin(a, b));
            });

        math.AddMethod(L"Max", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", TYPE_DOUBLE)
            .AddParameter(L"b", TYPE_DOUBLE)
            .SetCallback([](const CallState& context)
			{
                auto [a, b] = GetArgs<double, double>(context);
                return context.Collector.FromValue(std::fmax(a, b));
            });
    });
}
