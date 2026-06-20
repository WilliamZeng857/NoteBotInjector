#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <string>

namespace NBAuth {

constexpr size_t RSA_KEY_SIZE_BITS = 2048;
constexpr size_t RSA_SIGNATURE_SIZE_BYTES = 256;
constexpr size_t SHA256_DIGEST_SIZE = 32;
constexpr size_t AES_GCM_KEY_SIZE = 32;
constexpr size_t AES_GCM_IV_SIZE = 12;
constexpr size_t AES_GCM_TAG_SIZE = 16;
constexpr size_t HMAC_KEY_SIZE = 32;

constexpr uint32_t PROTOCOL_MAGIC = 0x4E423430;
constexpr uint32_t TOKEN_VERSION  = 0x00010001;

using ByteVector = std::vector<uint8_t>;
using FixedHash32 = std::array<uint8_t, SHA256_DIGEST_SIZE>;
using FixedSig256 = std::array<uint8_t, RSA_SIGNATURE_SIZE_BYTES>;

struct RsaKeyPair {
    ByteVector privateKey;
    ByteVector publicKey;
};

bool RsaGenerateKeyPair(RsaKeyPair &out);
bool RsaSignSha256(const ByteVector &privateKeyDer,
                   const uint8_t *data, size_t dataLen,
                   FixedSig256 &outSignature);
bool RsaSignSha256Blob(const ByteVector &privateKeyBlob,
                       const uint8_t *data, size_t dataLen,
                       FixedSig256 &outSignature);
bool RsaVerifySha256(const ByteVector &publicKeyDer,
                     const uint8_t *data, size_t dataLen,
                     const FixedSig256 &signature);

bool HmacSha256(const uint8_t *key, size_t keyLen,
                const uint8_t *data, size_t dataLen,
                FixedHash32 &out);

bool Sha256(const uint8_t *data, size_t len, FixedHash32 &out);

bool AesGcmEncrypt(const uint8_t key[AES_GCM_KEY_SIZE],
                   const uint8_t iv[AES_GCM_IV_SIZE],
                   const uint8_t *plaintext, size_t ptLen,
                   const uint8_t *aad, size_t aadLen,
                   ByteVector &outCiphertext,
                   uint8_t tag[AES_GCM_TAG_SIZE]);

bool AesGcmDecrypt(const uint8_t key[AES_GCM_KEY_SIZE],
                   const uint8_t iv[AES_GCM_IV_SIZE],
                   const uint8_t *ciphertext, size_t ctLen,
                   const uint8_t *aad, size_t aadLen,
                   const uint8_t tag[AES_GCM_TAG_SIZE],
                   ByteVector &outPlaintext);

bool SecureRandomBytes(uint8_t *out, size_t len);

std::string Base64Encode(const uint8_t *data, size_t len);
ByteVector  Base64Decode(const std::string &b64);

/* DLL 下载 Token 生成（客户端）
 * 输入: key(32B), nonceOut(16B输出), timestamp, command
 * 输出: signatureOut(32B)
 * 返回 true=成功
 */
bool GenerateDllDownloadToken(
    const uint8_t *key, size_t keyLen,
    uint8_t *nonceOut, size_t nonceLen,
    uint64_t timestamp,
    const char *command,
    uint8_t *signatureOut, size_t sigLen);

} // namespace NBAuth
