#pragma once

#include "kyra/defines/core/types.h"

#include <time.h>


// Return codes ------------------------------------------------- //

typedef enum Clock_Result {
    CLOCK_SUCCESS                                   = 0,

    
    // --- Wall clock --- //

    CLOCK_ERROR_WALL_REF_OUT_WALL_CLOCK_NULL        = -1,
    CLOCK_ERROR_WALL_FAILED_TO_GET_TIME             = -2,
    CLOCK_ERROR_WALL_INVALID_TIMESTAMP              = -3,
    CLOCK_ERROR_WALL_STRING_NULL                    = -4,
    CLOCK_ERROR_WALL_FAILED_TO_PARSE_DATETIME       = -5,


} ClockResult;


// Wall clock --------------------------------------------------- //

typedef struct tm           WallClockTimeInfo;
typedef time_t              WallClockTimestamp;

typedef enum WallClock_MonthOfYear {
    MONTH_JANUARY = 1,
    MONTH_FEBRUARY,
    MONTH_MARCH,
    MONTH_APRIL,
    MONTH_MAY,
    MONTH_JUNE,
    MONTH_JULY,
    MONTH_AUGUST,
    MONTH_SEPTEMBER,
    MONTH_OCTOBER,
    MONTH_NOVEMBER,
    MONTH_DECEMBER
} WallClockMonthOfYear;

typedef enum WallClock_DayOfWeek {
    DAY_SUNDAY = 1,
    DAY_MONDAY,
    DAY_TUESDAY,
    DAY_WEDNESDAY,
    DAY_THURSDAY,
    DAY_FRIDAY,
    DAY_SATURDAY
} WallClockDayOfWeek;

typedef struct WallClock_Handle {
    WallClockTimeInfo       time_info;
    WallClockTimestamp      timestamp;
} WallClock;

typedef struct WallClock_Date {
    WallClockMonthOfYear    month_of_year;
    WallClockDayOfWeek      day_of_week;
    UInt8                   day_of_month;
    UInt16                  year;
} WallClockDate;

typedef struct WallClock_Time {
    UInt8                   hour;
    UInt8                   minute;
    UInt8                   second;
} WallClockTime;

typedef struct WallClock_DateTime {
    WallClockDate           date;
    WallClockTime           time;
} WallClockDateTime;



