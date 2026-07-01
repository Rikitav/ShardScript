#include <cmath>

#include <ShardScript.hpp>

using namespace shard;

static ObjectInstance* shard_math_Sin(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::sin(val));
}

static ObjectInstance* shard_math_Cos(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::cos(val));
}

static ObjectInstance* shard_math_Tan(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::tan(val));
}

static ObjectInstance* shard_math_Asin(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::asin(val));
}

static ObjectInstance* shard_math_Acos(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::acos(val));
}

static ObjectInstance* shard_math_Atan(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::atan(val));
}

static ObjectInstance* shard_math_Atan2(const CallState& context) noexcept
{
    double y = context.Args[0]->AsDouble();
    double x = context.Args[1]->AsDouble();
    return context.Collector.FromValue(std::atan2(y, x));
}

static ObjectInstance* shard_math_Pow(const CallState& context) noexcept
{
    double base = context.Args[0]->AsDouble();
    double exp = context.Args[1]->AsDouble();
    return context.Collector.FromValue(std::pow(base, exp));
}

static ObjectInstance* shard_math_Sqrt(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::sqrt(val));
}

static ObjectInstance* shard_math_Cbrt(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::cbrt(val));
}

static ObjectInstance* shard_math_Exp(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::exp(val));
}

static ObjectInstance* shard_math_Log(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::log(val));
}

static ObjectInstance* shard_math_Log10(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::log10(val));
}

static ObjectInstance* shard_math_Abs(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::abs(val));
}

static ObjectInstance* shard_math_Ceil(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::ceil(val));
}

static ObjectInstance* shard_math_Floor(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::floor(val));
}

static ObjectInstance* shard_math_Round(const CallState& context) noexcept
{
    double val = context.Args[0]->AsDouble();
    return context.Collector.FromValue(std::round(val));
}

static ObjectInstance* shard_math_Min(const CallState& context) noexcept
{
    double a = context.Args[0]->AsDouble();
    double b = context.Args[1]->AsDouble();
    return context.Collector.FromValue(std::fmin(a, b));
}

static ObjectInstance* shard_math_Max(const CallState& context) noexcept
{
    double a = context.Args[0]->AsDouble();
    double b = context.Args[1]->AsDouble();
    return context.Collector.FromValue(std::fmax(a, b));
}

static ObjectInstance* shard_math_PI_get(const CallState& context) noexcept
{
    return context.Collector.FromValue(3.14159265358979323846);
}

static ObjectInstance* shard_math_E_get(const CallState& context) noexcept
{
    return context.Collector.FromValue(2.71828182845904523536);
}

SHARDLIB_GETMETADATA
{
    lib.Name = L"shard.math";
    lib.Description = L"High-performance native math library wrappers";
    lib.Version = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> mathNamespace(context, L"math");

    SymbolBuilder<ClassSymbol> mathClass = mathNamespace.AddClass(L"Math", LINK_STATIC);

    mathClass.AddProperty(L"PI", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
        .AddGetter().SetCallback(&shard_math_PI_get);

    mathClass.AddProperty(L"E", TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
        .AddGetter().SetCallback(&shard_math_E_get);

    mathClass.AddMethod(L"Sin", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Sin);

    mathClass.AddMethod(L"Cos", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Cos);

    mathClass.AddMethod(L"Tan", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Tan);

    mathClass.AddMethod(L"Asin", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Asin);

    mathClass.AddMethod(L"Acos", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Acos);

    mathClass.AddMethod(L"Atan", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Atan);

    mathClass.AddMethod(L"Atan2", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"y", TYPE_DOUBLE)
        .AddParameter(L"x", TYPE_DOUBLE).SetCallback(&shard_math_Atan2);

    mathClass.AddMethod(L"Pow", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"base", TYPE_DOUBLE)
        .AddParameter(L"exponent", TYPE_DOUBLE).SetCallback(&shard_math_Pow);

    mathClass.AddMethod(L"Sqrt", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Sqrt);

    mathClass.AddMethod(L"Cbrt", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Cbrt);

    mathClass.AddMethod(L"Exp", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Exp);

    mathClass.AddMethod(L"Log", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Log);

    mathClass.AddMethod(L"Log10", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Log10);

    mathClass.AddMethod(L"Abs", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Abs);

    mathClass.AddMethod(L"Ceil", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Ceil);

    mathClass.AddMethod(L"Floor", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Floor);

    mathClass.AddMethod(L"Round", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"value", TYPE_DOUBLE).SetCallback(&shard_math_Round);

    mathClass.AddMethod(L"Min", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"a", TYPE_DOUBLE)
        .AddParameter(L"b", TYPE_DOUBLE).SetCallback(&shard_math_Min);

    mathClass.AddMethod(L"Max", TYPE_DOUBLE, LINK_STATIC)
        .AddParameter(L"a", TYPE_DOUBLE)
        .AddParameter(L"b", TYPE_DOUBLE).SetCallback(&shard_math_Max);
}