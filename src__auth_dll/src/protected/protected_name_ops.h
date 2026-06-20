#pragma once

#include <cstdint>
#include <vector>
#include "crypto/vmp_defs.h"

namespace NBName::Protected {

NB_NOINLINE bool NBVmp_Name_IsOriginalHook(const unsigned char *bytes,
                                           const unsigned char *expected,
                                           size_t size);
NB_NOINLINE bool NBVmp_Name_IsAlreadyPatched(const unsigned char *bytes,
                                             size_t size);
NB_NOINLINE std::vector<unsigned char> NBVmp_Name_BuildShellcode(uint64_t fakeNameAddr,
                                                                 uint64_t strCopyAddr,
                                                                 uint64_t returnAddr);
NB_NOINLINE bool NBVmp_Name_MakeJmp5(uint64_t from,
                                     uint64_t to,
                                     unsigned char out[5]);

} // namespace NBName::Protected
