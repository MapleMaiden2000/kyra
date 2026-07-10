#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/delegates.h"
#include "kyra/defines/core/timer.h"
#include "kyra/defines/core/clock.h"


// API functions ------------------------------------------------ //

KYRA_ENGINE_API TimerResult     timer_startup(ConstStr config_filepath);
KYRA_ENGINE_API TimerResult     timer_shutdown(void);

KYRA_ENGINE_API TimerResult     timer_update(const Flt64 delta_time);

KYRA_ENGINE_API TimerResult     timer_set_time_scale(const Flt32 scale);
KYRA_ENGINE_API TimerResult     timer_set_paused(const Bool paused);

KYRA_ENGINE_API TimerResult     timer_construct_timer(
    ConstStr id,
    const Flt64 duration, 
    const Bool loop, 
    const Bool use_real_time, 
    const DelegateFunction callback, 
    const Sender sender, 
    const Listener listener, 
    const VoidPtr user_data, 
    TimerHandle *out_handle
);
KYRA_ENGINE_API TimerResult     timer_destruct_timer(TimerHandle *handle);
KYRA_ENGINE_API TimerResult     timer_pause(const TimerHandle handle);
KYRA_ENGINE_API TimerResult     timer_resume(const TimerHandle handle);
KYRA_ENGINE_API TimerResult     timer_reset(const TimerHandle handle);
KYRA_ENGINE_API TimerResult     timer_get_remaining(const TimerHandle handle, Flt64 *out_remaining);

KYRA_ENGINE_API HiResClock      timer_get_clock(void);
KYRA_ENGINE_API Flt64           timer_get_real_delta_time(void);
KYRA_ENGINE_API Flt64           timer_get_game_delta_time(void);
KYRA_ENGINE_API Flt64           timer_get_real_time_second(void);
KYRA_ENGINE_API Flt64           timer_get_game_time_second(void);
KYRA_ENGINE_API Flt32           timer_get_time_scale(void);
KYRA_ENGINE_API Bool            timer_is_paused(void);

KYRA_ENGINE_API ConstStr        timer_result_to_string(const TimerResult result);



