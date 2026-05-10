#include "kyra/core/hal/clock/wall/wall.h"

#include <stdio.h>
#include <string.h>


// Platform-specific thread-safe macros ----------------------------- //

#if KYRA_PLATFORM_WINDOWS
    #define threadsafe_localtime(time_info, timestamp) localtime_s(time_info, timestamp)
    #define threadsafe_gmtime(time_info, timestamp) gmtime_s(time_info, timestamp)
    #define threadsafe_asctime(buf, sz, time_info) asctime_s(buf, sz, time_info)
#else
    #define threadsafe_localtime(time_info, timestamp) localtime_r(timestamp, time_info)
    #define threadsafe_gmtime(time_info, timestamp) gmtime_r(timestamp, time_info)
    #define threadsafe_asctime(buf, sz, time_info) asctime_r(time_info, buf, sz)
#endif


// Helper functions ---------------------------------------------------- //

static WallClockTimeInfo _clock_wall_timeinfo_from_unix(const WallClockTimestamp timestamp, const Bool utc) {
    WallClockTimeInfo time_info = {0};

    if (utc) {
        // Request 'Coordinated Universal Time'...

        // Assign Greenwich Mean Time
        threadsafe_gmtime(&time_info, &timestamp);
    } else {
        // Otherwise...

        // Assign local time
        threadsafe_localtime(&time_info, &timestamp);
    }

    return time_info;
}

static WallClockTimestamp _clock_wall_parse_datetime(ConstStr datatime_str) {
    WallClockTimeInfo time_info = {0};
    Int32 year = 0, month = 0, day = 0;
    Int32 hour = 0, minute = 0, second = 0;

    // Parse date-time
    if (sscanf(datatime_str, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        // sscanf() does not return 6, indicating failed parsing... 
        
        // Return invalid timestamp
        return (WallClockTimestamp)(-1);
    }

    // Validate parsed date-time
    if ((year < 1900 || year > 2100) || (month < 1 || month > 12) || (day < 1 || day > 31) || 
    (hour < 0 || hour > 23) || (minute < 0 || minute > 59) || (second < 0 || second > 59)) {
        // Invalid date or time value...
        
        // Return invalid timestamp
        return (WallClockTimestamp)(-1);
    }

    // Convert to WallClockTimeInfo correct format
    time_info.tm_year = year - 1900;
    time_info.tm_mon = month - 1;
    time_info.tm_mday = day;
    time_info.tm_hour = hour;
    time_info.tm_min = minute;
    time_info.tm_sec = second;
    time_info.tm_isdst = -1; // Let mktime() determine daylight saving time

    // Return as WallClockTimestamp via mktime()
    return mktime(&time_info);
}


// API functions ------------------------------------------------------- //

KYRA_ENGINE_API ClockResult clock_wall_now(WallClock *out_wall_clock) {
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    // Get current time
    WallClockTimestamp timestamp = time(NULL);
    if (timestamp == (WallClockTimestamp)(-1)) {
        // Failed to get time from time(0)...

        return CLOCK_ERROR_WALL_FAILED_TO_GET_TIME;
    }

    // Initialised wall clock properties
    {
        out_wall_clock->timestamp = timestamp;
        threadsafe_localtime(&out_wall_clock->time_info, &timestamp);
    }

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_now_utc(WallClock *out_wall_clock) {
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    // Get current time
    WallClockTimestamp timestamp = time(NULL);
    if (timestamp == (WallClockTimestamp)(-1)) {
        // Failed to get time from time(0)...

        return CLOCK_ERROR_WALL_FAILED_TO_GET_TIME;
    }

    // Initialised wall clock properties
    {
        out_wall_clock->timestamp = timestamp;
        threadsafe_gmtime(&out_wall_clock->time_info, &timestamp);
    }

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_today(WallClock *out_wall_clock) {
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    // Get current time
    WallClockTimestamp timestamp = time(NULL);
    if (timestamp == (WallClockTimestamp)(-1)) {
        // Failed to get time from time(0)...

        return CLOCK_ERROR_WALL_FAILED_TO_GET_TIME;
    }

    // Convert to WallClockTimeInfo
    WallClockTimeInfo time_info;
    threadsafe_localtime(&time_info, &timestamp);

    // Zero out the time part
    {
        time_info.tm_hour = 0;
        time_info.tm_min = 0;
        time_info.tm_sec = 0;
    }

    // Initialised wall clock properties
    {
        out_wall_clock->time_info = time_info;
        out_wall_clock->timestamp = mktime(&time_info);
    }

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_today_utc(WallClock *out_wall_clock) {
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    // Get current time
    WallClockTimestamp timestamp = time(NULL);
    if (timestamp == (WallClockTimestamp)(-1)) {
        // Failed to get time from time(0)...

        return CLOCK_ERROR_WALL_FAILED_TO_GET_TIME;
    }

    // Convert to WallClockTimeInfo
    WallClockTimeInfo time_info;
    threadsafe_gmtime(&time_info, &timestamp);

    // Zero out the time part
    {
        time_info.tm_hour = 0;
        time_info.tm_min = 0;
        time_info.tm_sec = 0;
    }

    // Initialised wall clock properties
    {
        out_wall_clock->time_info = time_info;
        out_wall_clock->timestamp = mktime(&time_info);
    }

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_from_julian(const Flt64 julian, WallClock *out_wall_clock) {
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    // Since: JD = unix/86400 + 2440587.5
    // Thus: unix = (JD - 2440587.5) * 86400
    out_wall_clock->timestamp = (WallClockTimestamp)((julian - 2440587.5) * 86400.0);
    threadsafe_localtime(&out_wall_clock->time_info, &out_wall_clock->timestamp);

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_to_julian(const WallClock wall_clock, Flt64 *out_julian) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;
    
    // Since: JD = unix/86400 + 2440587.5
    // Save to ref
    if (out_julian) *out_julian = (Flt64)wall_clock.timestamp / 86400.0 + 2440587.5;
    
    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_from_unix(const UInt64 unix, WallClock *out_wall_clock) {
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    out_wall_clock->timestamp = unix;
    threadsafe_localtime(&out_wall_clock->time_info, &out_wall_clock->timestamp);
 
    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_to_unix(const WallClock wall_clock, UInt64 *out_unix) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Save to ref
    if (out_unix) *out_unix = (UInt64)wall_clock.timestamp;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_parse_datetime(ConstStr str, WallClock *out_wall_clock) {
    if (!str) return CLOCK_ERROR_WALL_STRING_NULL;
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    // Parse date-time into constructing timestamp
    WallClockTimestamp timestamp = _clock_wall_parse_datetime(str);
    if (timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_FAILED_TO_PARSE_DATETIME;

    // Initialised wall clock properties
    {
        out_wall_clock->timestamp = timestamp;
        threadsafe_localtime(&out_wall_clock->time_info, &timestamp);
    }

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_parse_datetime_utc(ConstStr str, WallClock *out_wall_clock) {
    if (!str) return CLOCK_ERROR_WALL_STRING_NULL;
    if (!out_wall_clock) return CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL;

    // Parse date-time into constructing timestamp
    WallClockTimestamp timestamp = _clock_wall_parse_datetime(str);
    if (timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_FAILED_TO_PARSE_DATETIME;

    // Initialised wall clock properties
    {
        out_wall_clock->timestamp = timestamp;
        threadsafe_gmtime(&out_wall_clock->time_info, &timestamp);
    }

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_date(const WallClock wall_clock, WallClockDate *out_date) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;
    
    // Get date from WallClockTimeInfo
    WallClockDate date = {0};
    date.year = wall_clock.time_info.tm_year + 1900;
    date.month_of_year = wall_clock.time_info.tm_mon + 1;
    date.day_of_month = wall_clock.time_info.tm_mday;
    date.day_of_week = wall_clock.time_info.tm_wday;

    // Save to ref
    if (out_date) *out_date = date;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_time(const WallClock wall_clock, WallClockTime *out_time) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get time from WallClockTimeInfo
    WallClockTime time = {0};
    time.hour = wall_clock.time_info.tm_hour;
    time.minute = wall_clock.time_info.tm_min;
    time.second = wall_clock.time_info.tm_sec;

    // Save to ref
    if (out_time) *out_time = time;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_year(const WallClock wall_clock, UInt16 *out_year) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get year from WallClockTimeInfo
    // Save to ref
    if (out_year) *out_year = wall_clock.time_info.tm_year + 1900;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_month(const WallClock wall_clock, UInt8 *out_month) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get month from WallClockTimeInfo
    // Save to ref
    if (out_month) *out_month = wall_clock.time_info.tm_mon + 1;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_day(const WallClock wall_clock, UInt8 *out_day) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get day of month from WallClockTimeInfo
    // Save to ref
    if (out_day) *out_day = wall_clock.time_info.tm_mday;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_hour(const WallClock wall_clock, UInt8 *out_hour) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get hour from WallClockTimeInfo
    // Save to ref
    if (out_hour) *out_hour = wall_clock.time_info.tm_hour;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_minute(const WallClock wall_clock, UInt8 *out_minute) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get minute from WallClockTimeInfo
    // Save to ref
    if (out_minute) *out_minute = wall_clock.time_info.tm_min;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_second(const WallClock wall_clock, UInt8 *out_second) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get second from WallClockTimeInfo
    // Save to ref
    if (out_second) *out_second = wall_clock.time_info.tm_sec;

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_get_timezone(const WallClock wall_clock, Int32 *out_timezone) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    #if KYRA_PLATFORM_WINDOWS
        long win_offset = 0; // _get_timezone explicitly requests a long

        // WIN32 _get_timezone returns positive for west of UTC, standard is east of UTC positive
        _get_timezone(&win_offset);
        
        // Save to ref
        if (out_timezone) *out_timezone = (Int32)(-win_offset);
    #else
        // Use POSIX standard
        // Save to ref
        if (out_timezone) *out_timezone = (Int32)wall_clock.time_info.tm_gmtoff;
    #endif

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_is_am(const WallClock wall_clock, Bool *out_is_am) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Compute if it is AM from WallClockTimeInfo
    // Save to ref
    if (out_is_am) *out_is_am = (wall_clock.time_info.tm_hour < 12);

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_is_pm(const WallClock wall_clock, Bool *out_is_pm) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Compute if it is PM from WallClockTimeInfo
    // Save to ref
    if (out_is_pm) *out_is_pm = (wall_clock.time_info.tm_hour >= 12);

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_wall_is_leap_year(const WallClock wall_clock, Bool *out_is_leap_year) {
    if (wall_clock.timestamp == (WallClockTimestamp)(-1)) return CLOCK_ERROR_WALL_INVALID_TIMESTAMP;

    // Get year from WallClockTimeInfo
    Int32 year = wall_clock.time_info.tm_year + 1900;
    
    // Check if year is a leap year
    // Such year is a leap year if it is divisible by 4
    // Except for end-of-century years, which must be divisible by 400
    
    // Save tp ref
    if (out_is_leap_year) *out_is_leap_year = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ConstStr clock_wall_result_to_string(ClockResult result) {
    switch (result) {
        case CLOCK_SUCCESS:                                     return "CLOCK_SUCCESS";

        case CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL:          return "CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL";
        case CLOCK_ERROR_WALL_FAILED_TO_GET_TIME:               return "CLOCK_ERROR_WALL_FAILED_TO_GET_TIME";
        case CLOCK_ERROR_WALL_INVALID_TIMESTAMP:                return "CLOCK_ERROR_WALL_INVALID_TIMESTAMP";
        case CLOCK_ERROR_WALL_STRING_NULL:                      return "CLOCK_ERROR_WALL_STRING_NULL";
        case CLOCK_ERROR_WALL_FAILED_TO_PARSE_DATETIME:         return "CLOCK_ERROR_WALL_FAILED_TO_PARSE_DATETIME";
    
        default:                                                return "UNKNOWN_CLOCK_RESULT";
    }
}


