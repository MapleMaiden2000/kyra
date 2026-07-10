#include "kyra/core/hal/clock/hi_res/hi_res.h"


// Conversion constants --------------------------------------------- //

#define SECOND_TO_MILLISECOND   1000.0
#define SECOND_TO_MICROSECOND   1000000.0
#define SECOND_TO_NANOSECOND    1000000000.0

#define HZ_TO_KHZ               1000.0
#define HZ_TO_MHZ               1000000.0
#define HZ_TO_GHZ               1000000000.0


// API functions ---------------------------------------------------- //

KYRA_ENGINE_API ClockResult clock_hires_split(HiResClock *out_hires_clock) {
    if (!out_hires_clock) return CLOCK_ERROR_HIRES_REF_OUT_HIRES_CLOCK_NULL;

    #if KYRA_PLATFORM_WINDOWS
        // For Windows...

        // Get frequency of performance counter
        QueryPerformanceFrequency(&out_hires_clock->counter);
        
        // Get current timestamp
        QueryPerformanceCounter(&out_hires_clock->timestamp);

    #elif KYRA_PLATFORM_LINUX
        // For Linux...

        // Get current time
        if (clock_gettime(CLOCK_MONOTONIC, &out_hires_clock->time_spec) != 0)
            return CLOCK_ERROR_HIRES_FAILED_TO_GET_TIME;

    #elif KYRA_PLATFORM_MACOS
        // For MacOS...
    
        // Get time-base info
        mach_timebase_info(&out_hires_clock->time_base_info);

        // Get current timestamp
        out_hires_clock->timestamp = mach_absolute_time();

    #endif
    
    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_hires_elapsed_seconds(const HiResClock hires_clock, Flt64 *out_seconds) {
    #if KYRA_PLATFORM_WINDOWS
        HiResTimestamp current_timestamp;
        
        // Get current timestamp
        QueryPerformanceCounter(&current_timestamp);
        
        // Calculate elapsed time
        // Save to ref
        if (out_seconds) *out_seconds = (Flt64)(current_timestamp.QuadPart - hires_clock.timestamp.QuadPart) / (Flt64)hires_clock.counter.QuadPart;
    
    #elif KYRA_PLATFORM_LINUX
        struct timespec current_time_spec;
        
        // Get current time
        if (clock_gettime(CLOCK_MONOTONIC, &current_time_spec) != 0) return CLOCK_ERROR_HIRES_FAILED_TO_GET_TIME;
        
        // Calculate elapsed time
        // Linux monotonic clock is nanosecond based
        // Save to ref
        if (out_seconds) *out_seconds = (Flt64)(current_time_spec.tv_sec - hires_clock.time_spec.tv_sec) + (Flt64)(current_time_spec.tv_nsec - hires_clock.time_spec.tv_nsec) / SECOND_TO_NANOSECOND;
    
    #elif KYRA_PLATFORM_MACOS
        UInt64 current_timestamp = mach_absolute_time();
        
        // Calculate elapsed time
        // Mach absolute time is nanosecond based
        // Save to ref
        if (out_seconds) *out_seconds = (Flt64)(current_timestamp - hires_clock.timestamp) * hires_clock.time_base_info.numer / (Flt64)hires_clock.time_base_info.denom / SECOND_TO_NANOSECOND;
    
    #endif

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_hires_elapsed_milliseconds(const HiResClock hires_clock, Flt64 *out_milliseconds) {
    Flt64 seconds = 0.0;
    
    ClockResult elapsed_result = clock_hires_elapsed_seconds(hires_clock, &seconds);
    if (elapsed_result == CLOCK_SUCCESS) {
        // Got elapsed seconds...

        // Convert to milliseconds
        // Save to ref
        if (out_milliseconds) *out_milliseconds = seconds * SECOND_TO_MILLISECOND;
    }

    return elapsed_result;
}

KYRA_ENGINE_API ClockResult clock_hires_elapsed_microseconds(const HiResClock hires_clock, Flt64 *out_microseconds) {
    Flt64 seconds = 0.0;
    
    ClockResult elapsed_result = clock_hires_elapsed_seconds(hires_clock, &seconds);
    if (elapsed_result == CLOCK_SUCCESS) {
        // Got elapsed seconds...

        // Convert to microseconds
        // Save to ref
        if (out_microseconds) *out_microseconds = seconds * SECOND_TO_MICROSECOND;
    }

    return elapsed_result;
}

KYRA_ENGINE_API ClockResult clock_hires_elapsed_nanoseconds(const HiResClock hires_clock, Flt64 *out_nanoseconds) {
    Flt64 seconds = 0.0;
    
    ClockResult elapsed_result = clock_hires_elapsed_seconds(hires_clock, &seconds);
    if (elapsed_result == CLOCK_SUCCESS) {
        // Got elapsed seconds...

        // Convert to nanoseconds
        // Save to ref
        if (out_nanoseconds) *out_nanoseconds = seconds * SECOND_TO_NANOSECOND;
    }

    return elapsed_result;
}

KYRA_ENGINE_API ClockResult clock_hires_frequency_hz(const HiResClock hires_clock, Flt64 *out_frequency_hz) {
    #if KYRA_PLATFORM_WINDOWS
        // Save to ref
        if (out_frequency_hz) *out_frequency_hz = (Flt64)hires_clock.counter.QuadPart;
    
    #elif KYRA_PLATFORM_LINUX
        // Linux monotonic clocks are nanosecond based (1 GHz)
        // Save to ref
        if (out_frequency_hz) *out_frequency_hz = SECOND_TO_NANOSECOND;
    
    #elif KYRA_PLATFORM_MACOS
        // Convert timebase to Hz: (1e9 * denom) / numer
        // Save to ref
        if (out_frequency_hz) *out_frequency_hz = SECOND_TO_NANOSECOND * (Flt64)hires_clock.time_base_info.denom / (Flt64)hires_clock.time_base_info.numer;
    
    #endif

    return CLOCK_SUCCESS;
}

KYRA_ENGINE_API ClockResult clock_hires_frequency_khz(const HiResClock hires_clock, Flt64 *out_frequency_khz) {
    Flt64 freq_hz;
    
    ClockResult freq_result = clock_hires_frequency_hz(hires_clock, &freq_hz);
    if (freq_result == CLOCK_SUCCESS) {
        // Got frequency in Hz...
        
        // Convert to kHz
        // Save to ref
        if (out_frequency_khz) *out_frequency_khz = freq_hz / HZ_TO_KHZ;
    }
    
    return freq_result;
}

KYRA_ENGINE_API ClockResult clock_hires_frequency_mhz(const HiResClock hires_clock, Flt64 *out_frequency_mhz) {
    Flt64 freq_hz;
    
    ClockResult freq_result = clock_hires_frequency_hz(hires_clock, &freq_hz);
    if (freq_result == CLOCK_SUCCESS) {
        // Got frequency in Hz...
        
        // Convert to mHz
        // Save to ref
        if (out_frequency_mhz) *out_frequency_mhz = freq_hz / HZ_TO_MHZ;
    }
    
    return freq_result;
}

KYRA_ENGINE_API ClockResult clock_hires_frequency_ghz(const HiResClock hires_clock, Flt64 *out_frequency_ghz) {
    Flt64 freq_hz;
    
    ClockResult freq_result = clock_hires_frequency_hz(hires_clock, &freq_hz);
    if (freq_result == CLOCK_SUCCESS) {
        // Got frequency in Hz...
        
        // Convert to gHz
        // Save to ref
        if (out_frequency_ghz) *out_frequency_ghz = freq_hz / HZ_TO_GHZ;
    }
    
    return freq_result;
}

KYRA_ENGINE_API ConstStr clock_hires_result_to_string(const ClockResult result) {
    switch (result) {
        case CLOCK_SUCCESS:                                     return "CLOCK_SUCCESS";

        case CLOCK_ERROR_HIRES_REF_OUT_HIRES_CLOCK_NULL:        return "CLOCK_ERROR_HIRES_REF_OUT_HIRES_CLOCK_NULL";

        default:                                                return "UNKNOWN_CLOCK_RESULT";
    }
}




