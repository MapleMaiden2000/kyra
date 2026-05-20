#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/delegates.h"


// API functions --------------------------------------------------- //

KYRA_ENGINE_API DelegateResult      delegate_multicast_startup(void);
KYRA_ENGINE_API DelegateResult      delegate_multicast_shutdown(void);

KYRA_ENGINE_API DelegateResult      delegate_multicast_register(ConstStr id, const Listener listener, const DelegateFunction callback);
KYRA_ENGINE_API DelegateResult      delegate_multicast_unregister(ConstStr id, const Listener listener);

KYRA_ENGINE_API DelegateResult      delegate_multicast_invoke(ConstStr id, const Sender sender, const VoidPtr data);

KYRA_ENGINE_API ConstStr            delegate_multicast_result_to_string(const DelegateResult result);
