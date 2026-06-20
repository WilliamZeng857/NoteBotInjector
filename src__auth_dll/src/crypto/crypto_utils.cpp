#include "crypto_utils.h"
#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include <string>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#pragma comment(lib, "bcrypt.lib")

namespace NBAuth {

// ------------------------------------------------------------------
// DER parsing helpers
// ------------------------------------------------------------------
static bool DerParseLength(const uint8_t *p, size_t maxLen, size_t *outLen, size_t *outHeaderLen)
{
    if (maxLen < 1) return false;
    if (p[0] & 0x80) {
        size_t lenBytes = p[0] & 0x7F;
        if (1 + lenBytes > maxLen) return false;
        *outLen = 0;
        for (size_t i = 0; i < lenBytes; i++) {
            *outLen = (*outLen << 8) | p[1 + i];
        }
        *outHeaderLen = 1 + lenBytes;
    } else {
        *outLen = p[0];
        *outHeaderLen = 1;
    }
    return true;
}

static bool DerParseSequence(const uint8_t *der, size_t derLen,
                             const uint8_t **outContent, size_t *outContentLen,
                             size_t *outTotalLen)
{
    if (derLen < 2 || der[0] != 0x30) return false;
    size_t contentLen, headerLen;
    if (!DerParseLength(der + 1, derLen - 1, &contentLen, &headerLen)) return false;
    *outTotalLen = 1 + headerLen + contentLen;
    if (*outTotalLen > derLen) return false;
    *outContent = der + 1 + headerLen;
    *outContentLen = contentLen;
    return true;
}

static bool DerParseInteger(const uint8_t *der, size_t derLen,
                            const uint8_t **outData, size_t *outLen,
                            size_t *outTotalLen)
{
    if (derLen < 2 || der[0] != 0x02) return false;
    size_t dataLen, headerLen;
    if (!DerParseLength(der + 1, derLen - 1, &dataLen, &headerLen)) return false;
    *outTotalLen = 1 + headerLen + dataLen;
    if (*outTotalLen > derLen) return false;
    *outData = der + 1 + headerLen;
    *outLen = dataLen;
    return true;
}

static bool DerParseBitString(const uint8_t *der, size_t derLen,
                              const uint8_t **outData, size_t *outLen,
                              size_t *outTotalLen)
{
    if (derLen < 3 || der[0] != 0x03) return false;
    size_t dataLen, headerLen;
    if (!DerParseLength(der + 1, derLen - 1, &dataLen, &headerLen)) return false;
    *outTotalLen = 1 + headerLen + dataLen;
    if (*outTotalLen > derLen || dataLen < 1) return false;
    if (der[1 + headerLen] != 0x00) return false;
    *outData = der + 1 + headerLen + 1;
    *outLen = dataLen - 1;
    return true;
}

static bool DerDecodeRsaPublicKeyFromSpki(const uint8_t *der, size_t derLen,
                                          const uint8_t **outRsaDer,
                                          size_t *outRsaDerLen)
{
    const uint8_t *spkiContent = nullptr;
    size_t spkiContentLen = 0, spkiTotalLen = 0;
    if (!DerParseSequence(der, derLen, &spkiContent, &spkiContentLen, &spkiTotalLen)) {
        return false;
    }

    const uint8_t *algContent = nullptr;
    size_t algContentLen = 0, algTotalLen = 0;
    if (!DerParseSequence(spkiContent, spkiContentLen, &algContent, &algContentLen, &algTotalLen)) {
        return false;
    }

    const uint8_t *bitData = nullptr;
    size_t bitLen = 0, bitTotalLen = 0;
    if (!DerParseBitString(spkiContent + algTotalLen,
                           spkiContentLen - algTotalLen,
                           &bitData,
                           &bitLen,
                           &bitTotalLen)) {
        return false;
    }

    if (bitLen < 1 || bitData[0] != 0x30) {
        return false;
    }

    *outRsaDer = bitData;
    *outRsaDerLen = bitLen;
    return true;
}

// ------------------------------------------------------------------
// DER Integer to BCRYPT RSA blob integer (big-endian, strip leading zeros)
// ------------------------------------------------------------------
static ByteVector IntegerToLeBlob(const uint8_t *beData, size_t beLen, size_t targetLen)
{
    ByteVector result;
    // Skip leading zero byte (positive sign indicator)
    size_t start = 0;
    if (beLen > 0 && beData[0] == 0) start++;
    size_t actualLen = beLen - start;

    if (targetLen > 0) {
        result.resize(targetLen, 0);
        size_t copyLen = (actualLen < targetLen) ? actualLen : targetLen;
        size_t offset = targetLen - copyLen;
        std::memcpy(result.data() + offset, beData + start + (actualLen - copyLen), copyLen);
    } else {
        // No target length — use actual length
        result.resize(actualLen);
        std::memcpy(result.data(), beData + start, actualLen);
    }
    return result;
}

// ------------------------------------------------------------------
// Encode big-endian DER integer from little-endian blob
// ------------------------------------------------------------------
static ByteVector LeBlobToDerInteger(const uint8_t *leData, size_t leLen)
{
    // Reverse to big-endian
    ByteVector be;
    be.reserve(leLen);
    for (size_t i = leLen; i > 0; i--) be.push_back(leData[i - 1]);

    // Strip leading zeros
    size_t start = 0;
    while (start < be.size() - 1 && be[start] == 0) start++;

    bool needZero = (be[start] & 0x80) != 0;
    size_t dataLen = be.size() - start + (needZero ? 1 : 0);

    ByteVector result;
    result.push_back(0x02);
    if (dataLen < 128) {
        result.push_back((uint8_t)dataLen);
    } else if (dataLen < 256) {
        result.push_back(0x81);
        result.push_back((uint8_t)dataLen);
    } else {
        result.push_back(0x82);
        result.push_back((uint8_t)(dataLen >> 8));
        result.push_back((uint8_t)(dataLen & 0xFF));
    }
    if (needZero) result.push_back(0x00);
    for (size_t i = start; i < be.size(); i++) result.push_back(be[i]);
    return result;
}

static ByteVector EncodeDerSequence(const ByteVector &a, const ByteVector &b)
{
    size_t total = a.size() + b.size();
    ByteVector seq;
    seq.push_back(0x30);
    if (total < 128) {
        seq.push_back((uint8_t)total);
    } else if (total < 256) {
        seq.push_back(0x81);
        seq.push_back((uint8_t)total);
    } else {
        seq.push_back(0x82);
        seq.push_back((uint8_t)(total >> 8));
        seq.push_back((uint8_t)(total & 0xFF));
    }
    seq.insert(seq.end(), a.begin(), a.end());
    seq.insert(seq.end(), b.begin(), b.end());
    return seq;
}

// ------------------------------------------------------------------
// RSA: DER <-> BCRYPT BLOB conversions
// ------------------------------------------------------------------
static bool ImportRsaPublicKeyDer(const ByteVector &der, ByteVector &outBlob)
{
    const uint8_t *rsaDer = der.data();
    size_t rsaDerLen = der.size();
    const uint8_t *spkiRsaDer = nullptr;
    size_t spkiRsaDerLen = 0;
    if (DerDecodeRsaPublicKeyFromSpki(der.data(), der.size(), &spkiRsaDer, &spkiRsaDerLen)) {
        rsaDer = spkiRsaDer;
        rsaDerLen = spkiRsaDerLen;
    }

    const uint8_t *seqContent;
    size_t seqContentLen, seqTotalLen;
    if (!DerParseSequence(rsaDer, rsaDerLen, &seqContent, &seqContentLen, &seqTotalLen)) return false;

    const uint8_t *modData;
    size_t modLen, modTotal;
    if (!DerParseInteger(seqContent, seqContentLen, &modData, &modLen, &modTotal)) return false;

    const uint8_t *expData;
    size_t expLen, expTotal;
    if (!DerParseInteger(seqContent + modTotal, seqContentLen - modTotal, &expData, &expLen, &expTotal)) return false;

    ByteVector modLe = IntegerToLeBlob(modData, modLen, 0);
    ByteVector expLe = IntegerToLeBlob(expData, expLen, 0);

    size_t blobSize = sizeof(BCRYPT_RSAKEY_BLOB) + expLe.size() + modLe.size();
    outBlob.resize(blobSize);

    BCRYPT_RSAKEY_BLOB *hdr = (BCRYPT_RSAKEY_BLOB *)outBlob.data();
    hdr->Magic = BCRYPT_RSAPUBLIC_MAGIC;
    hdr->BitLength = (ULONG)(modLe.size() * 8);
    hdr->cbPublicExp = (ULONG)expLe.size();
    hdr->cbModulus = (ULONG)modLe.size();
    hdr->cbPrime1 = 0;
    hdr->cbPrime2 = 0;

    uint8_t *p = outBlob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
    memcpy(p, expLe.data(), expLe.size());
    p += expLe.size();
    memcpy(p, modLe.data(), modLe.size());
    return true;
}

static bool ImportRsaPrivateKeyDer(const ByteVector &der, ByteVector &outBlob)
{
    const uint8_t *seqContent;
    size_t seqContentLen, seqTotalLen;
    if (!DerParseSequence(der.data(), der.size(), &seqContent, &seqContentLen, &seqTotalLen)) return false;

    // version INTEGER
    const uint8_t *vData;
    size_t vLen, vTotal;
    if (!DerParseInteger(seqContent, seqContentLen, &vData, &vLen, &vTotal)) return false;

    // modulus
    const uint8_t *modData;
    size_t modLen, modTotal;
    if (!DerParseInteger(seqContent + vTotal, seqContentLen - vTotal, &modData, &modLen, &modTotal)) return false;

    // publicExponent
    const uint8_t *expData;
    size_t expLen, expTotal;
    if (!DerParseInteger(seqContent + vTotal + modTotal, seqContentLen - vTotal - modTotal, &expData, &expLen, &expTotal)) return false;

    // privateExponent (skip)
    const uint8_t *privExpData;
    size_t privExpLen, privExpTotal;
    if (!DerParseInteger(seqContent + vTotal + modTotal + expTotal, seqContentLen - vTotal - modTotal - expTotal,
                         &privExpData, &privExpLen, &privExpTotal)) return false;

    // prime1 (P)
    const uint8_t *p1Data;
    size_t p1Len, p1Total;
    if (!DerParseInteger(seqContent + vTotal + modTotal + expTotal + privExpTotal,
                         seqContentLen - vTotal - modTotal - expTotal - privExpTotal,
                         &p1Data, &p1Len, &p1Total)) return false;

    // prime2 (Q)
    const uint8_t *p2Data;
    size_t p2Len, p2Total;
    if (!DerParseInteger(seqContent + vTotal + modTotal + expTotal + privExpTotal + p1Total,
                         seqContentLen - vTotal - modTotal - expTotal - privExpTotal - p1Total,
                         &p2Data, &p2Len, &p2Total)) return false;

    ByteVector modLe = IntegerToLeBlob(modData, modLen, 0);
    ByteVector expLe = IntegerToLeBlob(expData, expLen, 0);
    ByteVector p1Le  = IntegerToLeBlob(p1Data, p1Len, modLe.size() / 2);
    ByteVector p2Le  = IntegerToLeBlob(p2Data, p2Len, modLe.size() / 2);

    size_t blobSize = sizeof(BCRYPT_RSAKEY_BLOB) + expLe.size() + modLe.size() + p1Le.size() + p2Le.size();
    outBlob.resize(blobSize);

    BCRYPT_RSAKEY_BLOB *hdr = (BCRYPT_RSAKEY_BLOB *)outBlob.data();
    hdr->Magic = BCRYPT_RSAPRIVATE_MAGIC;
    hdr->BitLength = (ULONG)(modLe.size() * 8);
    hdr->cbPublicExp = (ULONG)expLe.size();
    hdr->cbModulus = (ULONG)modLe.size();
    hdr->cbPrime1 = (ULONG)p1Le.size();
    hdr->cbPrime2 = (ULONG)p2Le.size();

    uint8_t *p = outBlob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
    memcpy(p, expLe.data(), expLe.size()); p += expLe.size();
    memcpy(p, modLe.data(), modLe.size()); p += modLe.size();
    memcpy(p, p1Le.data(),  p1Le.size());  p += p1Le.size();
    memcpy(p, p2Le.data(),  p2Le.size());
    return true;
}

static bool ExportRsaPublicKeyDer(const ByteVector &blob, ByteVector &outDer)
{
    const BCRYPT_RSAKEY_BLOB *hdr = (const BCRYPT_RSAKEY_BLOB *)blob.data();
    if (hdr->Magic != BCRYPT_RSAPUBLIC_MAGIC) return false;

    const uint8_t *exp = blob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
    const uint8_t *mod = exp + hdr->cbPublicExp;

    ByteVector modDer = LeBlobToDerInteger(mod, hdr->cbModulus);
    ByteVector expDer = LeBlobToDerInteger(exp, hdr->cbPublicExp);
    outDer = EncodeDerSequence(modDer, expDer);
    return true;
}

static bool ExportRsaPrivateKeyDer(const ByteVector &blob, ByteVector &outDer)
{
    const BCRYPT_RSAKEY_BLOB *hdr = (const BCRYPT_RSAKEY_BLOB *)blob.data();
    if (hdr->Magic != BCRYPT_RSAPRIVATE_MAGIC) return false;

    const uint8_t *exp = blob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
    const uint8_t *mod = exp + hdr->cbPublicExp;
    const uint8_t *p1  = mod + hdr->cbModulus;
    const uint8_t *p2  = p1  + hdr->cbPrime1;

    // version = 0
    ByteVector verDer;
    verDer.push_back(0x02); verDer.push_back(0x01); verDer.push_back(0x00);

    ByteVector modDer = LeBlobToDerInteger(mod, hdr->cbModulus);
    ByteVector expDer = LeBlobToDerInteger(exp, hdr->cbPublicExp);
    // privateExponent: we don't have it in BCRYPT_RSAPRIVATE_BLOB, but we need it for DER.
    // For our use case, we generate the key pair and keep both BLOBs.
    // So this function is not fully implemented — we store BLOBs instead of DER for private keys.
    // Alternatively, we can use BCRYPT_RSAFULLPRIVATE_BLOB to get all fields.
    (void)p1; (void)p2;
    return false; // Not needed for our workflow
}

// ------------------------------------------------------------------
// RSA Sign / Verify
// ------------------------------------------------------------------
bool RsaSignSha256(const ByteVector &privateKeyDer, const uint8_t *data, size_t dataLen, FixedSig256 &outSignature)
{
    ByteVector privBlob;
    if (!ImportRsaPrivateKeyDer(privateKeyDer, privBlob)) return false;
    return RsaSignSha256Blob(privBlob, data, dataLen, outSignature);
}

bool RsaSignSha256Blob(const ByteVector &privateKeyBlob, const uint8_t *data, size_t dataLen, FixedSig256 &outSignature)
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) return false;

    BCRYPT_KEY_HANDLE hKey = NULL;
    if (!NT_SUCCESS(BCryptImportKeyPair(hAlg, NULL, BCRYPT_RSAPRIVATE_BLOB, &hKey,
                                        const_cast<PUCHAR>(privateKeyBlob.data()),
                                        (ULONG)privateKeyBlob.size(), 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    FixedHash32 hash;
    if (!Sha256(data, dataLen, hash)) {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCRYPT_PKCS1_PADDING_INFO paddingInfo{};
    paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

    ULONG sigLen = 0;
    NTSTATUS status = BCryptSignHash(hKey, &paddingInfo, hash.data(), (ULONG)hash.size(),
                                     NULL, 0, &sigLen, BCRYPT_PAD_PKCS1);
    if (!NT_SUCCESS(status) || sigLen != RSA_SIGNATURE_SIZE_BYTES) {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    ByteVector sig(sigLen);
    status = BCryptSignHash(hKey, &paddingInfo, hash.data(), (ULONG)hash.size(),
                            sig.data(), (ULONG)sig.size(), &sigLen, BCRYPT_PAD_PKCS1);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (!NT_SUCCESS(status)) return false;
    memcpy(outSignature.data(), sig.data(), RSA_SIGNATURE_SIZE_BYTES);
    return true;
}

bool RsaVerifySha256(const ByteVector &publicKeyDer, const uint8_t *data, size_t dataLen, const FixedSig256 &signature)
{
    ByteVector pubBlob;
    if (!ImportRsaPublicKeyDer(publicKeyDer, pubBlob)) return false;

    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) return false;

    BCRYPT_KEY_HANDLE hKey = NULL;
    if (!NT_SUCCESS(BCryptImportKeyPair(hAlg, NULL, BCRYPT_RSAPUBLIC_BLOB, &hKey,
                                        pubBlob.data(), (ULONG)pubBlob.size(), 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    FixedHash32 hash;
    if (!Sha256(data, dataLen, hash)) {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCRYPT_PKCS1_PADDING_INFO paddingInfo{};
    paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

    NTSTATUS status = BCryptVerifySignature(hKey, &paddingInfo, hash.data(), (ULONG)hash.size(),
                                            (PUCHAR)signature.data(), (ULONG)signature.size(),
                                            BCRYPT_PAD_PKCS1);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    return NT_SUCCESS(status);
}

// ------------------------------------------------------------------
// Hashing
// ------------------------------------------------------------------
bool Sha256(const uint8_t *data, size_t len, FixedHash32 &out)
{
    NTSTATUS status = BCryptHash(BCRYPT_SHA256_ALG_HANDLE, NULL, 0,
                                 (PUCHAR)data, (ULONG)len,
                                 out.data(), (ULONG)out.size());
    return NT_SUCCESS(status);
}

bool HmacSha256(const uint8_t *key, size_t keyLen, const uint8_t *data, size_t dataLen, FixedHash32 &out)
{
    NTSTATUS status = BCryptHash(BCRYPT_HMAC_SHA256_ALG_HANDLE,
                                 (PUCHAR)key, (ULONG)keyLen,
                                 (PUCHAR)data, (ULONG)dataLen,
                                 out.data(), (ULONG)out.size());
    return NT_SUCCESS(status);
}

// ------------------------------------------------------------------
// AES-256-GCM
// ------------------------------------------------------------------
bool AesGcmEncrypt(const uint8_t key[AES_GCM_KEY_SIZE],
                   const uint8_t iv[AES_GCM_IV_SIZE],
                   const uint8_t *plaintext, size_t ptLen,
                   const uint8_t *aad, size_t aadLen,
                   ByteVector &outCiphertext,
                   uint8_t tag[AES_GCM_TAG_SIZE])
{
    BCRYPT_KEY_HANDLE hKey = NULL;
    ULONG keyObjLen = 0;
    ULONG result = 0;

    if (!NT_SUCCESS(BCryptGetProperty(BCRYPT_AES_GCM_ALG_HANDLE, BCRYPT_OBJECT_LENGTH,
                                      (PUCHAR)&keyObjLen, sizeof(ULONG), &result, 0))) return false;

    ByteVector keyObj(keyObjLen);
    if (!NT_SUCCESS(BCryptGenerateSymmetricKey(BCRYPT_AES_GCM_ALG_HANDLE, &hKey,
                                               keyObj.data(), (ULONG)keyObj.size(),
                                               (PUCHAR)key, AES_GCM_KEY_SIZE, 0))) return false;

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = (PUCHAR)iv;
    authInfo.cbNonce = AES_GCM_IV_SIZE;
    authInfo.pbTag = tag;
    authInfo.cbTag = AES_GCM_TAG_SIZE;
    if (aad && aadLen > 0) {
        authInfo.pbAuthData = (PUCHAR)aad;
        authInfo.cbAuthData = (ULONG)aadLen;
    }

    ULONG cipherLen = 0;
    NTSTATUS status = BCryptEncrypt(hKey, (PUCHAR)plaintext, (ULONG)ptLen, &authInfo,
                                    NULL, 0, NULL, 0, &cipherLen, 0);
    if (!NT_SUCCESS(status)) {
        BCryptDestroyKey(hKey);
        return false;
    }

    outCiphertext.resize(cipherLen);
    status = BCryptEncrypt(hKey, (PUCHAR)plaintext, (ULONG)ptLen, &authInfo,
                           NULL, 0, outCiphertext.data(), (ULONG)outCiphertext.size(), &cipherLen, 0);

    BCryptDestroyKey(hKey);
    return NT_SUCCESS(status);
}

bool AesGcmDecrypt(const uint8_t key[AES_GCM_KEY_SIZE],
                   const uint8_t iv[AES_GCM_IV_SIZE],
                   const uint8_t *ciphertext, size_t ctLen,
                   const uint8_t *aad, size_t aadLen,
                   const uint8_t tag[AES_GCM_TAG_SIZE],
                   ByteVector &outPlaintext)
{
    BCRYPT_KEY_HANDLE hKey = NULL;
    ULONG keyObjLen = 0;
    ULONG result = 0;

    if (!NT_SUCCESS(BCryptGetProperty(BCRYPT_AES_GCM_ALG_HANDLE, BCRYPT_OBJECT_LENGTH,
                                      (PUCHAR)&keyObjLen, sizeof(ULONG), &result, 0))) return false;

    ByteVector keyObj(keyObjLen);
    if (!NT_SUCCESS(BCryptGenerateSymmetricKey(BCRYPT_AES_GCM_ALG_HANDLE, &hKey,
                                               keyObj.data(), (ULONG)keyObj.size(),
                                               (PUCHAR)key, AES_GCM_KEY_SIZE, 0))) return false;

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = (PUCHAR)iv;
    authInfo.cbNonce = AES_GCM_IV_SIZE;
    authInfo.pbTag = (PUCHAR)tag;
    authInfo.cbTag = AES_GCM_TAG_SIZE;
    if (aad && aadLen > 0) {
        authInfo.pbAuthData = (PUCHAR)aad;
        authInfo.cbAuthData = (ULONG)aadLen;
    }

    ULONG plainLen = 0;
    NTSTATUS status = BCryptDecrypt(hKey, (PUCHAR)ciphertext, (ULONG)ctLen, &authInfo,
                                    NULL, 0, NULL, 0, &plainLen, 0);
    if (!NT_SUCCESS(status)) {
        BCryptDestroyKey(hKey);
        return false;
    }

    outPlaintext.resize(plainLen);
    status = BCryptDecrypt(hKey, (PUCHAR)ciphertext, (ULONG)ctLen, &authInfo,
                           NULL, 0, outPlaintext.data(), (ULONG)outPlaintext.size(), &plainLen, 0);

    BCryptDestroyKey(hKey);
    return NT_SUCCESS(status);
}

// ------------------------------------------------------------------
// Secure Random
// ------------------------------------------------------------------
bool SecureRandomBytes(uint8_t *out, size_t len)
{
    NTSTATUS status = BCryptGenRandom(BCRYPT_RNG_ALG_HANDLE, out, (ULONG)len, 0);
    return NT_SUCCESS(status);
}

// ------------------------------------------------------------------
// Base64
// ------------------------------------------------------------------
static const char BASE64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64Encode(const uint8_t *data, size_t len)
{
    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        uint32_t b = data[i] << 16;
        if (i + 1 < len) b |= data[i + 1] << 8;
        if (i + 2 < len) b |= data[i + 2];

        out.push_back(BASE64_CHARS[(b >> 18) & 0x3F]);
        out.push_back(BASE64_CHARS[(b >> 12) & 0x3F]);
        out.push_back((i + 1 < len) ? BASE64_CHARS[(b >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < len) ? BASE64_CHARS[b & 0x3F] : '=');
    }
    return out;
}

ByteVector Base64Decode(const std::string &b64)
{
    static int decodeTable[256];
    static bool init = false;
    if (!init) {
        memset(decodeTable, -1, sizeof(decodeTable));
        for (int i = 0; i < 64; i++) decodeTable[(unsigned char)BASE64_CHARS[i]] = i;
        init = true;
    }

    ByteVector out;
    out.reserve((b64.size() * 3) / 4);

    uint32_t val = 0;
    int valBits = 0;

    for (char c : b64) {
        if (c == '=') break;
        int v = decodeTable[(unsigned char)c];
        if (v < 0) continue;
        val = (val << 6) | v;
        valBits += 6;
        if (valBits >= 8) {
            valBits -= 8;
            out.push_back((uint8_t)((val >> valBits) & 0xFF));
        }
    }
    return out;
}

// ------------------------------------------------------------------
// Key Generation (for server-side use or testing)
// ------------------------------------------------------------------
bool RsaGenerateKeyPair(RsaKeyPair &out)
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RSA_ALGORITHM, NULL, 0))) return false;

    BCRYPT_KEY_HANDLE hKey = NULL;
    if (!NT_SUCCESS(BCryptGenerateKeyPair(hAlg, &hKey, (ULONG)RSA_KEY_SIZE_BITS, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (!NT_SUCCESS(BCryptFinalizeKeyPair(hKey, 0))) {
        BCryptDestroyKey(hKey);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    // Export public key
    ULONG pubLen = 0;
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &pubLen, 0);
    ByteVector pubBlob(pubLen);
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPUBLIC_BLOB, pubBlob.data(), (ULONG)pubBlob.size(), &pubLen, 0);

    // Export private key
    ULONG privLen = 0;
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, 0, &privLen, 0);
    ByteVector privBlob(privLen);
    BCryptExportKey(hKey, NULL, BCRYPT_RSAPRIVATE_BLOB, privBlob.data(), (ULONG)privBlob.size(), &privLen, 0);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    // Convert BLOBs to DER
    if (!ExportRsaPublicKeyDer(pubBlob, out.publicKey)) return false;

    // Private key: we need BCRYPT_RSAFULLPRIVATE_BLOB to get all fields for DER
    // For now, just store the BCRYPT_RSAPRIVATE_BLOB format
    // The server (Python) generates keys, so this is only for testing.
    out.privateKey = privBlob; // Store as BLOB, not DER
    return true;
}

// ------------------------------------------------------------------
// DLL Download Token Generation
// ------------------------------------------------------------------
bool GenerateDllDownloadToken(
    const uint8_t *key, size_t keyLen,
    uint8_t *nonceOut, size_t nonceLen,
    uint64_t timestamp,
    const char *command,
    uint8_t *signatureOut, size_t sigLen)
{
    if (!key || keyLen == 0 || !nonceOut || nonceLen == 0 || !command || !signatureOut || sigLen < SHA256_DIGEST_SIZE) {
        return false;
    }

    // 1. 生成随机 nonce
    if (!SecureRandomBytes(nonceOut, nonceLen)) {
        return false;
    }

    // 2. 构建数据: nonce || timestamp_string || command
    std::string tsStr = std::to_string(timestamp);
    std::vector<uint8_t> data;
    data.reserve(nonceLen + tsStr.size() + strlen(command));
    data.insert(data.end(), nonceOut, nonceOut + nonceLen);
    data.insert(data.end(), tsStr.begin(), tsStr.end());
    data.insert(data.end(), command, command + strlen(command));

    // 3. HMAC-SHA256
    FixedHash32 hmac;
    if (!HmacSha256(key, keyLen, data.data(), data.size(), hmac)) {
        return false;
    }

    std::memcpy(signatureOut, hmac.data(), SHA256_DIGEST_SIZE);
    return true;
}

} // namespace NBAuth
