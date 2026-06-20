#include "protected_name_ops.h"
#include "crypto/vmp_defs.h"

#include <climits>
#include <cstring>

namespace NBName::Protected {

static void appendU64(std::vector<unsigned char> &v, uint64_t x)
{
    for (int i = 0; i < 8; ++i) v.push_back(static_cast<unsigned char>((x >> (i * 8)) & 0xFF));
}

NB_NOINLINE bool NBVmp_Name_IsOriginalHook(const unsigned char *bytes,
                                           const unsigned char *expected,
                                           size_t size)
{
    NB_VMP_MUTATE("NB.Name.IsOriginalHook");
    bool ok = bytes && expected && size > 0 && std::memcmp(bytes, expected, size) == 0;
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Name_IsAlreadyPatched(const unsigned char *bytes,
                                             size_t size)
{
    NB_VMP_MUTATE("NB.Name.IsAlreadyPatched");
    bool ok = bytes && size > 0 && bytes[0] == 0xE9;
    NB_VMP_END();
    return ok;
}

NB_NOINLINE std::vector<unsigned char> NBVmp_Name_BuildShellcode(uint64_t fakeNameAddr,
                                                                 uint64_t strCopyAddr,
                                                                 uint64_t returnAddr)
{
    NB_VMP_VIRTUALIZE("NB.Name.BuildShellcode");
    std::vector<unsigned char> sc;
    sc.reserve(39);
    sc.push_back(0x49); sc.push_back(0xBB); appendU64(sc, fakeNameAddr);
    sc.push_back(0x4C); sc.push_back(0x89); sc.push_back(0xDA);
    sc.push_back(0x49); sc.push_back(0xBB); appendU64(sc, strCopyAddr);
    sc.push_back(0x41); sc.push_back(0xFF); sc.push_back(0xD3);
    sc.push_back(0x49); sc.push_back(0xBB); appendU64(sc, returnAddr);
    sc.push_back(0x41); sc.push_back(0xFF); sc.push_back(0xE3);
    NB_VMP_END();
    return sc;
}

NB_NOINLINE bool NBVmp_Name_MakeJmp5(uint64_t from,
                                     uint64_t to,
                                     unsigned char out[5])
{
    NB_VMP_MUTATE("NB.Name.MakeJmp5");
    bool ok = false;
    int64_t rel = static_cast<int64_t>(to) - static_cast<int64_t>(from + 5);
    if (out && rel >= INT32_MIN && rel <= INT32_MAX) {
        out[0] = 0xE9;
        int32_t r = static_cast<int32_t>(rel);
        std::memcpy(out + 1, &r, sizeof(r));
        ok = true;
    }
    NB_VMP_END();
    return ok;
}

} // namespace NBName::Protected
