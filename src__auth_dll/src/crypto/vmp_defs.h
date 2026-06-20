#pragma once

#if defined(_MSC_VER)
#   define NB_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#   define NB_NOINLINE __attribute__((noinline))
#else
#   define NB_NOINLINE
#endif

#ifdef NB_VMP_BUILD
#   include "VMProtectSDK.h"
#   define NB_VMP_VIRTUALIZE(name)  VMProtectBeginVirtualization(name)
#   define NB_VMP_MUTATE(name)      VMProtectBeginMutation(name)
#   define NB_VMP_ULTRA(name)       VMProtectBeginUltra(name)
#   define NB_VMP_END()             VMProtectEnd()
#   define VMP_VIRTUALIZER_START()  VMProtectBeginVirtualization("")
#   define VMP_VIRTUALIZER_END()    VMProtectEnd()
#   define VMP_MUTATION_START()     VMProtectBeginMutation("")
#   define VMP_MUTATION_END()       VMProtectEnd()
#   define VMP_ULTRA_START()        VMProtectBeginUltra("")
#   define VMP_ULTRA_END()          VMProtectEnd()
#else
#   define NB_VMP_VIRTUALIZE(name)  ((void)0)
#   define NB_VMP_MUTATE(name)      ((void)0)
#   define NB_VMP_ULTRA(name)       ((void)0)
#   define NB_VMP_END()             ((void)0)
#   define VMP_VIRTUALIZER_START()  ((void)0)
#   define VMP_VIRTUALIZER_END()    ((void)0)
#   define VMP_MUTATION_START()     ((void)0)
#   define VMP_MUTATION_END()       ((void)0)
#   define VMP_ULTRA_START()        ((void)0)
#   define VMP_ULTRA_END()          ((void)0)
#endif
