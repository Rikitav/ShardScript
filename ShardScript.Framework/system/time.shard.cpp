#include <ShardScript.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

using namespace shard;

namespace
{
    // -------------------------------------------------------------------------
    // Type symbols
    // -------------------------------------------------------------------------
    static StructSymbol* g_timeSpanStruct = nullptr;
    static StructSymbol* g_dateTimeStruct = nullptr;
    static EnumSymbol* g_dateTimeKindEnum = nullptr;

    static FieldSymbol* g_timeSpanTicksField = nullptr;

    static FieldSymbol* g_dateTimeTicksField = nullptr;
    static FieldSymbol* g_dateTimeKindField = nullptr;

    // -------------------------------------------------------------------------
    // Constants
    // -------------------------------------------------------------------------
    constexpr std::int64_t TicksPerMillisecond = 10'000;
    constexpr std::int64_t TicksPerSecond      = 10'000'000;
    constexpr std::int64_t TicksPerMinute      = 60 * TicksPerSecond;
    constexpr std::int64_t TicksPerHour        = 60 * TicksPerMinute;
    constexpr std::int64_t TicksPerDay         = 24 * TicksPerHour;

    // DateTimeKind enum values (must match script-visible enum)
    constexpr std::int64_t KindUtc         = 0;
    constexpr std::int64_t KindLocal       = 1;
    constexpr std::int64_t KindUnspecified = 2;

    // -------------------------------------------------------------------------
    // Platform time helpers
    // -------------------------------------------------------------------------
    static bool GetUtcTm(std::int64_t secondsSinceEpoch, std::tm& out)
    {
        std::time_t t = static_cast<std::time_t>(secondsSinceEpoch);
#ifdef _WIN32
        return gmtime_s(&out, &t) == 0;
#else
        return gmtime_r(&t, &out) != nullptr;
#endif
    }

    static bool GetLocalTm(std::int64_t secondsSinceEpoch, std::tm& out)
    {
        std::time_t t = static_cast<std::time_t>(secondsSinceEpoch);
#ifdef _WIN32
        return localtime_s(&out, &t) == 0;
#else
        return localtime_r(&t, &out) != nullptr;
#endif
    }

    static std::int64_t SecondsSinceEpochFromTicks(std::int64_t ticks)
    {
        return ticks / TicksPerSecond;
    }

    static std::int64_t TicksFromSecondsSinceEpoch(std::int64_t seconds)
    {
        return seconds * TicksPerSecond;
    }

    static std::int64_t TicksFromMillisecondsSinceEpoch(std::int64_t milliseconds)
    {
        return milliseconds * TicksPerMillisecond;
    }

    // -------------------------------------------------------------------------
    // TimeSpan helpers
    // -------------------------------------------------------------------------
    static std::int64_t GetTimeSpanTicks(ObjectInstance* instance)
    {
        if (instance == nullptr)
            return 0;

        return instance->GetField(g_timeSpanTicksField->SlotIndex)->AsInteger();
    }

    static ObjectInstance* CreateTimeSpan(const CallState& context, std::int64_t ticks)
    {
        ObjectInstance* result = context.Collector.AllocateInstance(g_timeSpanStruct);
        result->SetField(g_timeSpanTicksField->SlotIndex, context.Collector.FromValue(ticks));
        return result;
    }

    // -------------------------------------------------------------------------
    // DateTime helpers
    // -------------------------------------------------------------------------
    static std::int64_t GetDateTimeTicks(ObjectInstance* instance)
    {
        if (instance == nullptr)
            return 0;

        return instance->GetField(g_dateTimeTicksField->SlotIndex)->AsInteger();
    }

    static std::int64_t GetDateTimeKind(ObjectInstance* instance)
    {
        if (instance == nullptr)
            return KindUnspecified;

        return instance->GetField(g_dateTimeKindField->SlotIndex)->AsInteger();
    }

    static ObjectInstance* CreateDateTime(const CallState& context, std::int64_t ticks, std::int64_t kind)
    {
        ObjectInstance* result = context.Collector.AllocateInstance(g_dateTimeStruct);
        result->SetField(g_dateTimeTicksField->SlotIndex, context.Collector.FromValue(ticks));
        result->SetField(g_dateTimeKindField->SlotIndex, context.Collector.FromValue(kind));
        return result;
    }

    static std::int64_t DateTimeToUtcTicks(std::int64_t ticks, std::int64_t kind)
    {
        if (kind == KindUtc || kind == KindUnspecified)
            return ticks;

        // kind == KindLocal: convert local ticks to UTC ticks
        std::tm localTm{};
        if (!GetLocalTm(SecondsSinceEpochFromTicks(ticks), localTm))
            return ticks;

        std::time_t localTime = std::mktime(&localTm);
        if (localTime == static_cast<std::time_t>(-1))
            return ticks;

        return TicksFromSecondsSinceEpoch(static_cast<std::int64_t>(localTime));
    }

    static std::int64_t UtcTicksToLocalTicks(std::int64_t utcTicks)
    {
        std::int64_t utcSeconds = SecondsSinceEpochFromTicks(utcTicks);

        std::tm localTm{};
        if (!GetLocalTm(utcSeconds, localTm))
            return utcTicks;

        std::tm utcTm{};
        if (!GetUtcTm(utcSeconds, utcTm))
            return utcTicks;

        std::time_t localAsTime = std::mktime(&localTm);
        if (localAsTime == static_cast<std::time_t>(-1))
            return utcTicks;

#ifdef _WIN32
        std::time_t utcAsTime = _mkgmtime(&utcTm);
#else
        std::time_t utcAsTime = timegm(&utcTm);
#endif

        if (utcAsTime == static_cast<std::time_t>(-1))
            return utcTicks;

        std::int64_t offsetSeconds = static_cast<std::int64_t>(localAsTime) - static_cast<std::int64_t>(utcAsTime);
        return utcTicks + offsetSeconds * TicksPerSecond;
    }

    static std::tm GetDateTimeTm(ObjectInstance* instance)
    {
        std::int64_t ticks = GetDateTimeTicks(instance);
        std::int64_t kind  = GetDateTimeKind(instance);

        std::tm result{};
        if (kind == KindLocal)
        {
            GetLocalTm(SecondsSinceEpochFromTicks(ticks), result);
        }
        else
        {
            GetUtcTm(SecondsSinceEpochFromTicks(ticks), result);
        }

        return result;
    }

    static std::int64_t GetCurrentUtcTicks()
    {
        auto now = std::chrono::system_clock::now();
        auto sinceEpoch = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(sinceEpoch).count();
        return TicksFromMillisecondsSinceEpoch(millis);
    }

    static std::int64_t GetCurrentLocalTicks()
    {
        std::int64_t utcTicks = GetCurrentUtcTicks();
        return UtcTicksToLocalTicks(utcTicks);
    }

    // -------------------------------------------------------------------------
    // Formatting helper
    // -------------------------------------------------------------------------
    static std::wstring PadInt(int value, int width)
    {
        std::wostringstream ss;
        ss << std::setw(width) << std::setfill(L'0') << value;
        return ss.str();
    }

    static std::wstring FormatDateTime(const std::tm& tm, int millisecond, const std::wstring& format)
    {
        std::wstring result;
        std::size_t i = 0;

        while (i < format.size())
        {
            wchar_t c = format[i];

            if (c == L'y' && i + 3 < format.size() && format[i + 1] == L'y' && format[i + 2] == L'y' && format[i + 3] == L'y')
            {
                result += PadInt(tm.tm_year + 1900, 4);
                i += 4;
            }
            else if (c == L'M' && i + 1 < format.size() && format[i + 1] == L'M')
            {
                result += PadInt(tm.tm_mon + 1, 2);
                i += 2;
            }
            else if (c == L'd' && i + 1 < format.size() && format[i + 1] == L'd')
            {
                result += PadInt(tm.tm_mday, 2);
                i += 2;
            }
            else if (c == L'H' && i + 1 < format.size() && format[i + 1] == L'H')
            {
                result += PadInt(tm.tm_hour, 2);
                i += 2;
            }
            else if (c == L'm' && i + 1 < format.size() && format[i + 1] == L'm')
            {
                result += PadInt(tm.tm_min, 2);
                i += 2;
            }
            else if (c == L's' && i + 1 < format.size() && format[i + 1] == L's')
            {
                result += PadInt(tm.tm_sec, 2);
                i += 2;
            }
            else if (c == L'f' && i + 2 < format.size() && format[i + 1] == L'f' && format[i + 2] == L'f')
            {
                result += PadInt(millisecond, 3);
                i += 3;
            }
            else
            {
                result += c;
                ++i;
            }
        }

        return result;
    }

    // -------------------------------------------------------------------------
    // TimeSpan callbacks
    // -------------------------------------------------------------------------
    static ObjectInstance* timeSpan_FromTicks(const CallState& context)
    {
        auto [ticks] = GetArgs<std::int64_t>(context);
        return CreateTimeSpan(context, ticks);
    }

    static ObjectInstance* timeSpan_FromMilliseconds(const CallState& context)
    {
        auto [value] = GetArgs<double>(context);
        return CreateTimeSpan(context, static_cast<std::int64_t>(value * TicksPerMillisecond));
    }

    static ObjectInstance* timeSpan_FromSeconds(const CallState& context)
    {
        auto [value] = GetArgs<double>(context);
        return CreateTimeSpan(context, static_cast<std::int64_t>(value * TicksPerSecond));
    }

    static ObjectInstance* timeSpan_FromMinutes(const CallState& context)
    {
        auto [value] = GetArgs<double>(context);
        return CreateTimeSpan(context, static_cast<std::int64_t>(value * TicksPerMinute));
    }

    static ObjectInstance* timeSpan_FromHours(const CallState& context)
    {
        auto [value] = GetArgs<double>(context);
        return CreateTimeSpan(context, static_cast<std::int64_t>(value * TicksPerHour));
    }

    static ObjectInstance* timeSpan_FromDays(const CallState& context)
    {
        auto [value] = GetArgs<double>(context);
        return CreateTimeSpan(context, static_cast<std::int64_t>(value * TicksPerDay));
    }

    static ObjectInstance* timeSpan_Ticks_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(self));
    }

    static ObjectInstance* timeSpan_TotalMilliseconds_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(static_cast<double>(GetTimeSpanTicks(self)) / TicksPerMillisecond);
    }

    static ObjectInstance* timeSpan_TotalSeconds_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(static_cast<double>(GetTimeSpanTicks(self)) / TicksPerSecond);
    }

    static ObjectInstance* timeSpan_TotalMinutes_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(static_cast<double>(GetTimeSpanTicks(self)) / TicksPerMinute);
    }

    static ObjectInstance* timeSpan_TotalHours_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(static_cast<double>(GetTimeSpanTicks(self)) / TicksPerHour);
    }

    static ObjectInstance* timeSpan_TotalDays_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(static_cast<double>(GetTimeSpanTicks(self)) / TicksPerDay);
    }

    static ObjectInstance* timeSpan_Days_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(self) / TicksPerDay);
    }

    static ObjectInstance* timeSpan_Hours_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue((GetTimeSpanTicks(self) % TicksPerDay) / TicksPerHour);
    }

    static ObjectInstance* timeSpan_Minutes_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue((GetTimeSpanTicks(self) % TicksPerHour) / TicksPerMinute);
    }

    static ObjectInstance* timeSpan_Seconds_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue((GetTimeSpanTicks(self) % TicksPerMinute) / TicksPerSecond);
    }

    static ObjectInstance* timeSpan_Milliseconds_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue((GetTimeSpanTicks(self) % TicksPerSecond) / TicksPerMillisecond);
    }

    static ObjectInstance* timeSpan_Add(const CallState& context)
    {
        auto [self, other] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return CreateTimeSpan(context, GetTimeSpanTicks(self) + GetTimeSpanTicks(other));
    }

    static ObjectInstance* timeSpan_Subtract(const CallState& context)
    {
        auto [self, other] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return CreateTimeSpan(context, GetTimeSpanTicks(self) - GetTimeSpanTicks(other));
    }

    static ObjectInstance* timeSpan_Negate(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return CreateTimeSpan(context, -GetTimeSpanTicks(self));
    }

    static ObjectInstance* timeSpan_Duration(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::int64_t ticks = GetTimeSpanTicks(self);
        return CreateTimeSpan(context, ticks < 0 ? -ticks : ticks);
    }

    static ObjectInstance* timeSpan_ToString(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::int64_t ticks = GetTimeSpanTicks(self);
        bool negative = ticks < 0;
        if (negative)
            ticks = -ticks;

        std::int64_t days = ticks / TicksPerDay;
        std::int64_t hours = (ticks % TicksPerDay) / TicksPerHour;
        std::int64_t minutes = (ticks % TicksPerHour) / TicksPerMinute;
        std::int64_t seconds = (ticks % TicksPerMinute) / TicksPerSecond;
        std::int64_t millis = (ticks % TicksPerSecond) / TicksPerMillisecond;

        std::wostringstream ss;
        if (negative)
            ss << L"-";

        if (days > 0)
            ss << days << L".";

        ss << std::setw(2) << std::setfill(L'0') << hours << L":"
           << std::setw(2) << std::setfill(L'0') << minutes << L":"
           << std::setw(2) << std::setfill(L'0') << seconds;

        if (millis > 0)
            ss << L"." << std::setw(3) << std::setfill(L'0') << millis;

        return context.Collector.FromValue(ss.str());
    }

    static ObjectInstance* timeSpan_Multiply(const CallState& context)
    {
        auto [self, factor] = GetArgs<ObjectInstance*, double>(context);
        return CreateTimeSpan(context, static_cast<std::int64_t>(GetTimeSpanTicks(self) * factor));
    }

    static ObjectInstance* timeSpan_DivideByDouble(const CallState& context)
    {
        auto [self, divisor] = GetArgs<ObjectInstance*, double>(context);
        if (divisor == 0.0)
            throw std::runtime_error("Division by zero");

        return CreateTimeSpan(context, static_cast<std::int64_t>(GetTimeSpanTicks(self) / divisor));
    }

    static ObjectInstance* timeSpan_DivideByTimeSpan(const CallState& context)
    {
        auto [self, other] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        std::int64_t otherTicks = GetTimeSpanTicks(other);
        if (otherTicks == 0)
            throw std::runtime_error("Division by zero");

        return context.Collector.FromValue(static_cast<double>(GetTimeSpanTicks(self)) / static_cast<double>(otherTicks));
    }

    static ObjectInstance* timeSpan_Equals(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(a) == GetTimeSpanTicks(b));
    }

    static ObjectInstance* timeSpan_NotEquals(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(a) != GetTimeSpanTicks(b));
    }

    static ObjectInstance* timeSpan_LessThan(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(a) < GetTimeSpanTicks(b));
    }

    static ObjectInstance* timeSpan_GreaterThan(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(a) > GetTimeSpanTicks(b));
    }

    static ObjectInstance* timeSpan_LessThanOrEqual(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(a) <= GetTimeSpanTicks(b));
    }

    static ObjectInstance* timeSpan_GreaterThanOrEqual(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetTimeSpanTicks(a) >= GetTimeSpanTicks(b));
    }

    static ObjectInstance* timeSpan_UnaryNegation(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return CreateTimeSpan(context, -GetTimeSpanTicks(self));
    }

    // -------------------------------------------------------------------------
    // DateTime callbacks
    // -------------------------------------------------------------------------
    static ObjectInstance* dateTime_Now(const CallState& context)
    {
        return CreateDateTime(context, GetCurrentLocalTicks(), KindLocal);
    }

    static ObjectInstance* dateTime_UtcNow(const CallState& context)
    {
        return CreateDateTime(context, GetCurrentUtcTicks(), KindUtc);
    }

    static ObjectInstance* dateTime_FromUnixTimeSeconds(const CallState& context)
    {
        auto [seconds] = GetArgs<std::int64_t>(context);
        return CreateDateTime(context, TicksFromSecondsSinceEpoch(seconds), KindUtc);
    }

    static ObjectInstance* dateTime_FromUnixTimeMilliseconds(const CallState& context)
    {
        auto [milliseconds] = GetArgs<std::int64_t>(context);
        return CreateDateTime(context, TicksFromMillisecondsSinceEpoch(milliseconds), KindUtc);
    }

    static ObjectInstance* dateTime_Ticks_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(self));
    }

    static ObjectInstance* dateTime_UnixTimestamp_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(SecondsSinceEpochFromTicks(GetDateTimeTicks(self)));
    }

    static ObjectInstance* dateTime_UnixTimestampMilliseconds_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(self) / TicksPerMillisecond);
    }

    static ObjectInstance* dateTime_Kind_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_Year_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        return context.Collector.FromValue(static_cast<std::int64_t>(tm.tm_year + 1900));
    }

    static ObjectInstance* dateTime_Month_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        return context.Collector.FromValue(static_cast<std::int64_t>(tm.tm_mon + 1));
    }

    static ObjectInstance* dateTime_Day_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        return context.Collector.FromValue(static_cast<std::int64_t>(tm.tm_mday));
    }

    static ObjectInstance* dateTime_DayOfWeek_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        return context.Collector.FromValue(static_cast<std::int64_t>(tm.tm_wday));
    }

    static ObjectInstance* dateTime_Hour_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        return context.Collector.FromValue(static_cast<std::int64_t>(tm.tm_hour));
    }

    static ObjectInstance* dateTime_Minute_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        return context.Collector.FromValue(static_cast<std::int64_t>(tm.tm_min));
    }

    static ObjectInstance* dateTime_Second_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        return context.Collector.FromValue(static_cast<std::int64_t>(tm.tm_sec));
    }

    static ObjectInstance* dateTime_Millisecond_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::int64_t ticks = GetDateTimeTicks(self);
        std::int64_t ms = (ticks % TicksPerSecond) / TicksPerMillisecond;
        if (ms < 0)
            ms += 1000;
        return context.Collector.FromValue(ms);
    }

    static ObjectInstance* dateTime_Date_get(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::int64_t ticks = GetDateTimeTicks(self);
        std::int64_t kind = GetDateTimeKind(self);

        std::tm tm = GetDateTimeTm(self);
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        std::time_t dateSeconds = std::mktime(&tm);
        if (dateSeconds == static_cast<std::time_t>(-1))
            return CreateDateTime(context, ticks, kind);

        std::int64_t dateTicks = TicksFromSecondsSinceEpoch(static_cast<std::int64_t>(dateSeconds));
        return CreateDateTime(context, dateTicks, kind);
    }

    static ObjectInstance* dateTime_Add(const CallState& context)
    {
        auto [self, span] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) + GetTimeSpanTicks(span), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_AddMilliseconds(const CallState& context)
    {
        auto [self, value] = GetArgs<ObjectInstance*, double>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) + static_cast<std::int64_t>(value * TicksPerMillisecond), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_AddSeconds(const CallState& context)
    {
        auto [self, value] = GetArgs<ObjectInstance*, double>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) + static_cast<std::int64_t>(value * TicksPerSecond), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_AddMinutes(const CallState& context)
    {
        auto [self, value] = GetArgs<ObjectInstance*, double>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) + static_cast<std::int64_t>(value * TicksPerMinute), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_AddHours(const CallState& context)
    {
        auto [self, value] = GetArgs<ObjectInstance*, double>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) + static_cast<std::int64_t>(value * TicksPerHour), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_AddDays(const CallState& context)
    {
        auto [self, value] = GetArgs<ObjectInstance*, double>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) + static_cast<std::int64_t>(value * TicksPerDay), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_ToUniversalTime(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::int64_t ticks = GetDateTimeTicks(self);
        std::int64_t kind = GetDateTimeKind(self);
        return CreateDateTime(context, DateTimeToUtcTicks(ticks, kind), KindUtc);
    }

    static ObjectInstance* dateTime_ToLocalTime(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::int64_t ticks = GetDateTimeTicks(self);
        std::int64_t kind = GetDateTimeKind(self);

        std::int64_t utcTicks = (kind == KindLocal) ? DateTimeToUtcTicks(ticks, kind) : ticks;
        return CreateDateTime(context, UtcTicksToLocalTicks(utcTicks), KindLocal);
    }

    static ObjectInstance* dateTime_ToString(const CallState& context)
    {
        auto [self] = GetArgs<ObjectInstance*>(context);
        std::tm tm = GetDateTimeTm(self);
        std::int64_t ticks = GetDateTimeTicks(self);
        std::int64_t ms = (ticks % TicksPerSecond) / TicksPerMillisecond;
        if (ms < 0)
            ms += 1000;

        return context.Collector.FromValue(FormatDateTime(tm, static_cast<int>(ms), L"yyyy-MM-dd HH:mm:ss"));
    }

    static ObjectInstance* dateTime_ToStringFormatted(const CallState& context)
    {
        auto [self, format] = GetArgs<ObjectInstance*, std::wstring>(context);
        std::tm tm = GetDateTimeTm(self);
        std::int64_t ticks = GetDateTimeTicks(self);
        std::int64_t ms = (ticks % TicksPerSecond) / TicksPerMillisecond;
        if (ms < 0)
            ms += 1000;

        return context.Collector.FromValue(FormatDateTime(tm, static_cast<int>(ms), format));
    }

    static ObjectInstance* dateTime_SubtractDateTime(const CallState& context)
    {
        auto [self, other] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return CreateTimeSpan(context, GetDateTimeTicks(self) - GetDateTimeTicks(other));
    }

    static ObjectInstance* dateTime_SubtractTimeSpan(const CallState& context)
    {
        auto [self, span] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) - GetTimeSpanTicks(span), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_AddTimeSpanOperator(const CallState& context)
    {
        auto [self, span] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return CreateDateTime(context, GetDateTimeTicks(self) + GetTimeSpanTicks(span), GetDateTimeKind(self));
    }

    static ObjectInstance* dateTime_Equals(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(a) == GetDateTimeTicks(b));
    }

    static ObjectInstance* dateTime_NotEquals(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(a) != GetDateTimeTicks(b));
    }

    static ObjectInstance* dateTime_LessThan(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(a) < GetDateTimeTicks(b));
    }

    static ObjectInstance* dateTime_GreaterThan(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(a) > GetDateTimeTicks(b));
    }

    static ObjectInstance* dateTime_LessThanOrEqual(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(a) <= GetDateTimeTicks(b));
    }

    static ObjectInstance* dateTime_GreaterThanOrEqual(const CallState& context)
    {
        auto [a, b] = GetArgs<ObjectInstance*, ObjectInstance*>(context);
        return context.Collector.FromValue(GetDateTimeTicks(a) >= GetDateTimeTicks(b));
    }

}

// =============================================================================
// Library entry point
// =============================================================================
SHARDLIB_GETMETADATA
{
    lib.Name        = L"shard.time";
    lib.Description = L"Native date and time library for ShardScript";
    lib.Version     = L"1.0.0";
}

SHARDLIB_ENTRYPOINT
{
    SymbolBuilder<NamespaceSymbol> timeNs(context, L"time");

    // -------------------------------------------------------------------------
    // enum DateTimeKind
    // -------------------------------------------------------------------------
    {
        SymbolBuilder<EnumSymbol> kindEnum = timeNs.AddEnum(L"DateTimeKind", false, ACS_PUBLIC);
        g_dateTimeKindEnum = kindEnum.Get();

        kindEnum
            .AddValue(L"Utc", 0)
            .AddValue(L"Local", 1)
            .AddValue(L"Unspecified", 2);
    }

    // -------------------------------------------------------------------------
    // struct TimeSpan
    // -------------------------------------------------------------------------
    {
        SymbolBuilder<StructSymbol> spanStruct = timeNs.AddStruct(L"TimeSpan");
        g_timeSpanStruct = spanStruct.Get();

        g_timeSpanTicksField = spanStruct
            .AddField(L"_ticks", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE)
            .Get();

        // Static factory methods
        spanStruct.AddMethod(L"FromTicks", g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"ticks", TYPE_INT)
            .SetCallback(&timeSpan_FromTicks);

        spanStruct.AddMethod(L"FromMilliseconds", g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&timeSpan_FromMilliseconds);

        spanStruct.AddMethod(L"FromSeconds", g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&timeSpan_FromSeconds);

        spanStruct.AddMethod(L"FromMinutes", g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&timeSpan_FromMinutes);

        spanStruct.AddMethod(L"FromHours", g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&timeSpan_FromHours);

        spanStruct.AddMethod(L"FromDays", g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&timeSpan_FromDays);

        // Properties
        spanStruct.AddProperty(L"Ticks", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_Ticks_get);

        spanStruct.AddProperty(L"TotalMilliseconds", TYPE_DOUBLE, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_TotalMilliseconds_get);

        spanStruct.AddProperty(L"TotalSeconds", TYPE_DOUBLE, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_TotalSeconds_get);

        spanStruct.AddProperty(L"TotalMinutes", TYPE_DOUBLE, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_TotalMinutes_get);

        spanStruct.AddProperty(L"TotalHours", TYPE_DOUBLE, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_TotalHours_get);

        spanStruct.AddProperty(L"TotalDays", TYPE_DOUBLE, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_TotalDays_get);

        spanStruct.AddProperty(L"Days", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_Days_get);

        spanStruct.AddProperty(L"Hours", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_Hours_get);

        spanStruct.AddProperty(L"Minutes", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_Minutes_get);

        spanStruct.AddProperty(L"Seconds", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_Seconds_get);

        spanStruct.AddProperty(L"Milliseconds", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&timeSpan_Milliseconds_get);

        // Instance methods
        spanStruct.AddMethod(L"Add", g_timeSpanStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"other", g_timeSpanStruct)
            .SetCallback(&timeSpan_Add);

        spanStruct.AddMethod(L"Subtract", g_timeSpanStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"other", g_timeSpanStruct)
            .SetCallback(&timeSpan_Subtract);

        spanStruct.AddMethod(L"Negate", g_timeSpanStruct, LINK_INSTANCE, ACS_PUBLIC)
            .SetCallback(&timeSpan_Negate);

        spanStruct.AddMethod(L"Duration", g_timeSpanStruct, LINK_INSTANCE, ACS_PUBLIC)
            .SetCallback(&timeSpan_Duration);

        spanStruct.AddMethod(L"ToString", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC)
            .SetCallback(&timeSpan_ToString);

        // Operator overloads
        spanStruct.AddOperator(TokenType::AddOperator, g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_Add);

        spanStruct.AddOperator(TokenType::SubOperator, g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_Subtract);

        spanStruct.AddOperator(TokenType::MultOperator, g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"span", g_timeSpanStruct)
            .AddParameter(L"factor", TYPE_DOUBLE)
            .SetCallback(&timeSpan_Multiply);

        spanStruct.AddOperator(TokenType::MultOperator, g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"factor", TYPE_DOUBLE)
            .AddParameter(L"span", g_timeSpanStruct)
            .SetCallback(&timeSpan_Multiply);

        spanStruct.AddOperator(TokenType::DivOperator, g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"span", g_timeSpanStruct)
            .AddParameter(L"divisor", TYPE_DOUBLE)
            .SetCallback(&timeSpan_DivideByDouble);

        spanStruct.AddOperator(TokenType::DivOperator, TYPE_DOUBLE, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_DivideByTimeSpan);

        spanStruct.AddOperator(TokenType::SubOperator, g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"span", g_timeSpanStruct)
            .SetCallback(&timeSpan_UnaryNegation);

        spanStruct.AddOperator(TokenType::EqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_Equals);

        spanStruct.AddOperator(TokenType::NotEqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_NotEquals);

        spanStruct.AddOperator(TokenType::LessOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_LessThan);

        spanStruct.AddOperator(TokenType::GreaterOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_GreaterThan);

        spanStruct.AddOperator(TokenType::LessOrEqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_LessThanOrEqual);

        spanStruct.AddOperator(TokenType::GreaterOrEqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_timeSpanStruct)
            .AddParameter(L"b", g_timeSpanStruct)
            .SetCallback(&timeSpan_GreaterThanOrEqual);
    }

    // -------------------------------------------------------------------------
    // struct DateTime
    // -------------------------------------------------------------------------
    {
        SymbolBuilder<StructSymbol> dtStruct = timeNs.AddStruct(L"DateTime");
        g_dateTimeStruct = dtStruct.Get();

        g_dateTimeTicksField = dtStruct
            .AddField(L"_ticks", TYPE_INT, LINK_INSTANCE, ACS_PRIVATE)
            .Get();

        g_dateTimeKindField = dtStruct
            .AddField(L"_kind", g_dateTimeKindEnum, LINK_INSTANCE, ACS_PRIVATE)
            .Get();

        // Static factory methods
        dtStruct.AddProperty(L"Now", g_dateTimeStruct, LINK_STATIC, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Now);

        dtStruct.AddProperty(L"UtcNow", g_dateTimeStruct, LINK_STATIC, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_UtcNow);

        dtStruct.AddMethod(L"FromUnixTimeSeconds", g_dateTimeStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"seconds", TYPE_INT)
            .SetCallback(&dateTime_FromUnixTimeSeconds);

        dtStruct.AddMethod(L"FromUnixTimeMilliseconds", g_dateTimeStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"milliseconds", TYPE_INT)
            .SetCallback(&dateTime_FromUnixTimeMilliseconds);

        // Properties
        dtStruct.AddProperty(L"Ticks", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Ticks_get);

        dtStruct.AddProperty(L"UnixTimestamp", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_UnixTimestamp_get);

        dtStruct.AddProperty(L"UnixTimestampMilliseconds", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_UnixTimestampMilliseconds_get);

        dtStruct.AddProperty(L"Kind", g_dateTimeKindEnum, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Kind_get);

        dtStruct.AddProperty(L"Year", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Year_get);

        dtStruct.AddProperty(L"Month", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Month_get);

        dtStruct.AddProperty(L"Day", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Day_get);

        dtStruct.AddProperty(L"DayOfWeek", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_DayOfWeek_get);

        dtStruct.AddProperty(L"Hour", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Hour_get);

        dtStruct.AddProperty(L"Minute", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Minute_get);

        dtStruct.AddProperty(L"Second", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Second_get);

        dtStruct.AddProperty(L"Millisecond", TYPE_INT, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Millisecond_get);

        dtStruct.AddProperty(L"Date", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddGetter().SetCallback(&dateTime_Date_get);

        // Instance methods
        dtStruct.AddMethod(L"Add", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"span", g_timeSpanStruct)
            .SetCallback(&dateTime_Add);

        dtStruct.AddMethod(L"AddMilliseconds", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&dateTime_AddMilliseconds);

        dtStruct.AddMethod(L"AddSeconds", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&dateTime_AddSeconds);

        dtStruct.AddMethod(L"AddMinutes", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&dateTime_AddMinutes);

        dtStruct.AddMethod(L"AddHours", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&dateTime_AddHours);

        dtStruct.AddMethod(L"AddDays", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"value", TYPE_DOUBLE)
            .SetCallback(&dateTime_AddDays);

        dtStruct.AddMethod(L"ToUniversalTime", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .SetCallback(&dateTime_ToUniversalTime);

        dtStruct.AddMethod(L"ToLocalTime", g_dateTimeStruct, LINK_INSTANCE, ACS_PUBLIC)
            .SetCallback(&dateTime_ToLocalTime);

        dtStruct.AddMethod(L"ToString", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC)
            .SetCallback(&dateTime_ToString);

        dtStruct.AddMethod(L"ToString", TYPE_STRING, LINK_INSTANCE, ACS_PUBLIC)
            .AddParameter(L"format", TYPE_STRING)
            .SetCallback(&dateTime_ToStringFormatted);

        // Operator overloads
        dtStruct.AddOperator(TokenType::AddOperator, g_dateTimeStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"dt", g_dateTimeStruct)
            .AddParameter(L"span", g_timeSpanStruct)
            .SetCallback(&dateTime_AddTimeSpanOperator);

        dtStruct.AddOperator(TokenType::SubOperator, g_dateTimeStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"dt", g_dateTimeStruct)
            .AddParameter(L"span", g_timeSpanStruct)
            .SetCallback(&dateTime_SubtractTimeSpan);

        dtStruct.AddOperator(TokenType::SubOperator, g_timeSpanStruct, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_dateTimeStruct)
            .AddParameter(L"b", g_dateTimeStruct)
            .SetCallback(&dateTime_SubtractDateTime);

        dtStruct.AddOperator(TokenType::EqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_dateTimeStruct)
            .AddParameter(L"b", g_dateTimeStruct)
            .SetCallback(&dateTime_Equals);

        dtStruct.AddOperator(TokenType::NotEqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_dateTimeStruct)
            .AddParameter(L"b", g_dateTimeStruct)
            .SetCallback(&dateTime_NotEquals);

        dtStruct.AddOperator(TokenType::LessOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_dateTimeStruct)
            .AddParameter(L"b", g_dateTimeStruct)
            .SetCallback(&dateTime_LessThan);

        dtStruct.AddOperator(TokenType::GreaterOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_dateTimeStruct)
            .AddParameter(L"b", g_dateTimeStruct)
            .SetCallback(&dateTime_GreaterThan);

        dtStruct.AddOperator(TokenType::LessOrEqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_dateTimeStruct)
            .AddParameter(L"b", g_dateTimeStruct)
            .SetCallback(&dateTime_LessThanOrEqual);

        dtStruct.AddOperator(TokenType::GreaterOrEqualsOperator, TYPE_BOOL, LINK_STATIC, ACS_PUBLIC)
            .AddParameter(L"a", g_dateTimeStruct)
            .AddParameter(L"b", g_dateTimeStruct)
            .SetCallback(&dateTime_GreaterThanOrEqual);
    }
}
