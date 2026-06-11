#pragma once

#include "kyra/defines/core/types.h"
#include "kyra/defines/core/containers.h"


// Return codes ---------------------------------------------------- //

typedef enum Delegate_Result {
    DELEGATE_SUCCESS                                                            = 0,


    // -- Uni-cast -- //

    DELEGATE_UNICAST_ERROR_ALREADY_INITIALISED                                  = -100,
    DELEGATE_UNICAST_ERROR_NOT_INITIALISED                                      = -101,
    DELEGATE_UNICAST_ERROR_DELEGATE_ID_NULL                                     = -102,
    DELEGATE_UNICAST_ERROR_DELEGATE_CALLBACK_NULL                               = -103,
    DELEGATE_UNICAST_ERROR_DELEGATE_NOT_REGISTERED                              = -104,
    DELEGATE_UNICAST_ERROR_DELEGATE_SENDER_NULL                                 = -105,
    DELEGATE_UNICAST_ERROR_DELEGATE_HAS_NO_CALLBACK                             = -106,
    DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE                  = -107,
    DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE                 = -108,
    DELEGATE_UNICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP                     = -109,
    DELEGATE_UNICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP                      = -110,
    DELEGATE_UNICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE               = -111,
    DELEGATE_UNICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE              = -112,
    DELEGATE_UNICAST_ERROR_FAILED_TO_REGISTER_DELEGATE                          = -113,
    DELEGATE_UNICAST_ERROR_FAILED_TO_UNREGISTER_DELEGATE                        = -114,
    DELEGATE_UNICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID                     = -115,
    

    // -- Multi-cast -- //

    DELEGATE_MULTICAST_ERROR_ALREADY_INITIALISED                                = -200,
    DELEGATE_MULTICAST_ERROR_NOT_INITIALISED                                    = -201,
    DELEGATE_MULTICAST_ERROR_DELEGATE_ID_NULL                                   = -202,
    DELEGATE_MULTICAST_ERROR_DELEGATE_CALLBACK_NULL                             = -203,
    DELEGATE_MULTICAST_ERROR_DELEGATE_OLD_CALLBACK_NULL                         = -204,
    DELEGATE_MULTICAST_ERROR_DELEGATE_NEW_CALLBACK_NULL                         = -205,
    DELEGATE_MULTICAST_ERROR_DELEGATE_LISTENER_NULL                             = -206,
    DELEGATE_MULTICAST_ERROR_DELEGATE_SENDER_NULL                               = -207,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_STATE                = -208,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_STATE               = -209,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_ALLOCATE_MEMORY_FOR_DELEGATE             = -210,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_DEALLOCATE_MEMORY_OF_DELEGATE            = -211,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_DELEGATE_MAP                   = -212,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_MAP                    = -213,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ACTIVE_DELEGATES_ARRAY         = -214,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_ACTIVE_DELEGATES_ARRAY          = -215,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_LISTENERS_ARRAY        = -216,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_DESTRUCT_DELEGATE_CALLBACKS_ARRAY        = -217,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_CONSTRUCT_ARRAYS_FOR_DELEGATE            = -218,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_REGISTER_DELEGATE                        = -219,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_DELEGATE_FOR_ID                   = -220,
    DELEGATE_MULTICAST_ERROR_FAILED_TO_LOCATE_LISTENER_FOR_ID                   = -221,

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


// ----- Multicast delegate ----- //

typedef struct Multicast_Delegate {
    Array                           listeners;  // <Listener>
    Array                           callbacks;  // <DelegateFunction>
    Int32                           invoke_depth;

    // For allocations/deallocations
    ByteSize                        memory_size;
    
} *MulticastDelegate;







