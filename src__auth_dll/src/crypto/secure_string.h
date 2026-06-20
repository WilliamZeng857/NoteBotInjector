#pragma once
#include <cstdint>
#include <cstring>

namespace NBAuth {

template <size_t N>
class EncryptedString {
    uint8_t m_data[N];
    uint8_t m_key[N];
public:
    constexpr EncryptedString(const char (&str)[N]) {
        uint32_t seed = static_cast<uint32_t>(N * 0x9E3779B9u);
        for (size_t i = 0; i < N; ++i) {
            seed = seed * 1103515245u + 12345u;
            m_key[i] = static_cast<uint8_t>((seed >> 16) & 0xFF);
            m_data[i] = static_cast<uint8_t>(str[i]) ^ m_key[i];
        }
    }

    void decrypt(char out[N]) const {
        for (size_t i = 0; i < N; ++i) {
            out[i] = static_cast<char>(m_data[i] ^ m_key[i]);
        }
    }
};

} // namespace NBAuth

#define NB_ENC_STR(s) ([]() -> const char* { \
    static const NBAuth::EncryptedString<sizeof(s)> _es(s); \
    static char _buf[sizeof(s)]; \
    _es.decrypt(_buf); \
    _buf[sizeof(s)-1] = '\0'; \
    return _buf; \
}())
