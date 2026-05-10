#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/clock.h"


// API functions ------------------------------------------------------- //

KYRA_ENGINE_API ClockResult     clock_wall_now(WallClock *out_wall_clock);
KYRA_ENGINE_API ClockResult     clock_wall_now_utc(WallClock *out_wall_clock);

KYRA_ENGINE_API ClockResult     clock_wall_today(WallClock *out_wall_clock);
KYRA_ENGINE_API ClockResult     clock_wall_today_utc(WallClock *out_wall_clock);

KYRA_ENGINE_API ClockResult     clock_wall_from_julian(const Flt64 julian, WallClock *out_wall_clock);
KYRA_ENGINE_API ClockResult     clock_wall_to_julian(const WallClock wall_clock, Flt64 *out_julian);

KYRA_ENGINE_API ClockResult     clock_wall_from_unix(const UInt64 unix, WallClock *out_wall_clock);
KYRA_ENGINE_API ClockResult     clock_wall_to_unix(const WallClock wall_clock, UInt64 *out_unix);

KYRA_ENGINE_API ClockResult     clock_wall_parse_datetime(ConstStr str, WallClock *out_wall_clock);
KYRA_ENGINE_API ClockResult     clock_wall_parse_datetime_utc(ConstStr str, WallClock *out_wall_clock);

KYRA_ENGINE_API ClockResult     clock_wall_get_date(const WallClock wall_clock, WallClockDate *out_date);
KYRA_ENGINE_API ClockResult     clock_wall_get_time(const WallClock wall_clock, WallClockTime *out_time);
KYRA_ENGINE_API ClockResult     clock_wall_get_year(const WallClock wall_clock, UInt16 *out_year);
KYRA_ENGINE_API ClockResult     clock_wall_get_month(const WallClock wall_clock, UInt8 *out_month);
KYRA_ENGINE_API ClockResult     clock_wall_get_day(const WallClock wall_clock, UInt8 *out_day);
KYRA_ENGINE_API ClockResult     clock_wall_get_hour(const WallClock wall_clock, UInt8 *out_hour);
KYRA_ENGINE_API ClockResult     clock_wall_get_minute(const WallClock wall_clock, UInt8 *out_minute);
KYRA_ENGINE_API ClockResult     clock_wall_get_second(const WallClock wall_clock, UInt8 *out_second);
KYRA_ENGINE_API ClockResult     clock_wall_get_timezone(const WallClock wall_clock, Int32 *out_timezone);

KYRA_ENGINE_API ClockResult     clock_wall_is_am(const WallClock wall_clock, Bool *out_is_am);
KYRA_ENGINE_API ClockResult     clock_wall_is_pm(const WallClock wall_clock, Bool *out_is_pm);
KYRA_ENGINE_API ClockResult     clock_wall_is_leap_year(const WallClock wall_clock, Bool *out_is_leap_year);

KYRA_ENGINE_API ConstStr        clock_wall_result_to_string(ClockResult result);

