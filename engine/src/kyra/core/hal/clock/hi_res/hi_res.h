#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/clock.h"


// API functions ---------------------------------------------------- //

KYRA_ENGINE_API ClockResult     clock_hires_split(HiResClock *out_hires_clock);

KYRA_ENGINE_API ClockResult     clock_hires_elapsed_seconds(const HiResClock hires_clock, Flt64 *out_seconds);
KYRA_ENGINE_API ClockResult     clock_hires_elapsed_milliseconds(const HiResClock hires_clock, Flt64 *out_milliseconds);
KYRA_ENGINE_API ClockResult     clock_hires_elapsed_microseconds(const HiResClock hires_clock, Flt64 *out_microseconds);
KYRA_ENGINE_API ClockResult     clock_hires_elapsed_nanoseconds(const HiResClock hires_clock, Flt64 *out_nanoseconds);

KYRA_ENGINE_API ClockResult     clock_hires_frequency_hz(const HiResClock hires_clock, Flt64 *out_frequency_hz);
KYRA_ENGINE_API ClockResult     clock_hires_frequency_khz(const HiResClock hires_clock, Flt64 *out_frequency_khz);
KYRA_ENGINE_API ClockResult     clock_hires_frequency_mhz(const HiResClock hires_clock, Flt64 *out_frequency_mhz);
KYRA_ENGINE_API ClockResult     clock_hires_frequency_ghz(const HiResClock hires_clock, Flt64 *out_frequency_ghz);

KYRA_ENGINE_API ConstStr        clock_hires_result_to_string(const ClockResult result);
