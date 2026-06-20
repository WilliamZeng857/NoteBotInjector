#pragma once

#include <QByteArray>
#include <QString>

#include <cstring>

#include "crypto/crypto_utils.h"

namespace NBAuth::V3 {

inline uint8_t hexNibble(QChar ch)
{
    const ushort u = ch.unicode();
    if (u >= '0' && u <= '9') return static_cast<uint8_t>(u - '0');
    if (u >= 'a' && u <= 'f') return static_cast<uint8_t>(10 + (u - 'a'));
    if (u >= 'A' && u <= 'F') return static_cast<uint8_t>(10 + (u - 'A'));
    return 0;
}

inline ::NBAuth::ByteVector hexToBytes(const QString &hex)
{
    const QString normalized = hex.trimmed();
    ::NBAuth::ByteVector bytes;
    bytes.reserve(static_cast<size_t>(normalized.size() / 2));
    for (int i = 0; i + 1 < normalized.size(); i += 2) {
        const uint8_t hi = hexNibble(normalized[i]);
        const uint8_t lo = hexNibble(normalized[i + 1]);
        bytes.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }
    return bytes;
}

inline int formalServerPublicKeyVersion()
{
    return 4;
}

inline QString formalServerPublicKeyFingerprint()
{
    return QStringLiteral("sha256:caba170778c0380bbcc3535ce540a35d75bd25f4fb62bd29e56ac072ff6b59f8");
}

inline const ::NBAuth::ByteVector &formalServerPublicKeyDer()
{
    static const QString kHex = QStringLiteral(
        "30820122300d06092a864886f70d01010105000382010f003082010a0282010100d4e034546ed97c46aa51d7e5a952ea7cb542428ad96d077ab29ad2264a1bd625502793ca2a816c5f8ba678a495c7d96132591090b06447d95142c247b80eec7cdcacecd2640741f4c69b7593fdbb5110ba34314532ed29bf054b2c1b6c9b808401584cf24c08113fa979d30dec9367cc363f6c24c03628fc5f0e75884b9cd865c9b545f5e2ecca71e4e48d74ad0c9383733f786a0cf569bc59f82111dddb437f382325de6159d078fb892bf717cf14905fb16e1efb41c3ecdb33a57de137a3f8f79ad2d09f97f420074c827fd4288904e083530c8dad202c27d30627c5c9c3f6269728c7ffccca81ae6c5ae82a9d30cbc7d2539080b27f9a6b71930af4c347830203010001");
    static const ::NBAuth::ByteVector kBytes = hexToBytes(kHex);
    return kBytes;
}

inline bool signatureHexToFixed(const QString &hex, ::NBAuth::FixedSig256 &out)
{
    const QByteArray bytes = QByteArray::fromHex(hex.trimmed().toLatin1());
    if (bytes.size() != static_cast<int>(out.size())) {
        return false;
    }
    std::memcpy(out.data(), bytes.constData(), out.size());
    return true;
}

} // namespace NBAuth::V3
