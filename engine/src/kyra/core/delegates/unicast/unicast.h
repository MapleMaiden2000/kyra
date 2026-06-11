#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/delegates.h"


// API functions --------------------------------------------------- //

KYRA_ENGINE_API DelegateResult      delegate_unicast_startup(void);
KYRA_ENGINE_API DelegateResult      delegate_unicast_shutdown(void);

KYRA_ENGINE_API DelegateResult      delegate_unicast_register(ConstStr id, const Listener listener, const DelegateFunction callback);
KYRA_ENGINE_API DelegateResult      delegate_unicast_unregister(ConstStr id);

KYRA_ENGINE_API DelegateResult      delegate_unicast_update(ConstStr id, const Listener listener, const DelegateFunction callback);

KYRA_ENGINE_API DelegateResult      delegate_unicast_set_callback(ConstStr id, const DelegateFunction callback);
KYRA_ENGINE_API DelegateResult      delegate_unicast_invoke(ConstStr id, const Sender sender, const VoidPtr data);

KYRA_ENGINE_API ConstStr            delegate_unicast_result_to_string(const DelegateResult result);






