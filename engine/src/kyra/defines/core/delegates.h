#pragma once

#include "kyra/defines/core/types.h"
#include "kyra/defines/core/containers.h"


// Return codes ---------------------------------------------------- //

typedef enum Delegate_Result {
    DELEGATE_SUCCESS                                                        = 0,


    // -- Unicast -- //

    DELEGATE_UNICAST_ERROR_ALREADY_INITIALISED                              = 101,
    DELEGATE_UNICAST_ERROR_NOT_INITIALISED                                  = 102,
    DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL                                 = 103,
    DELEGATE_UNICAST_ERROR_DELEGATE_CALLBACK_NULL                           = 104,
    DELEGATE_UNICAST_ERROR_DELEGATE_NOT_REGISTERED                          = 105,
    DELEGATE_UNICAST_ERROR_DELEGATE_SENDER_NULL                             = 106,
    DELEGATE_UNICAST_ERROR_DELEGATE_HAS_NO_CALLBACK                         = 107,
    DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE              = 108,
    DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE             = 109,
    DELEGATE_UNICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP                 = 110,
    DELEGATE_UNICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP                  = 111,
    DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE           = 112,
    DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE          = 113,
    DELEGATE_UNICAST_ERROR_FAILED_TO_REGISTER_DELEGATE                      = 114,
    DELEGATE_UNICAST_ERROR_FAILED_TO_UNREGISTER_DELEGATE                    = 115,
    DELEGATE_UNICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID                 = 116,
    
} DelegateResult;


// Types ----------------------------------------------------------- //

// ----- Sender & Listener ----- //

typedef     VoidPtr                 Sender;
typedef     VoidPtr                 Listener;


// ----- Delegate function ----- //

typedef     void                    (*Delegate_Function)(Sender sender, Listener listener, VoidPtr data);
typedef     Delegate_Function       DelegateFunction;


// ----- Unicast delegate ----- //

typedef struct Unicast_Delegate {
    Listener                        listener;
    DelegateFunction                callback;

    // For allocations/deallocations
    ByteSize                        memory_size;

} *UnicastDelegate;









