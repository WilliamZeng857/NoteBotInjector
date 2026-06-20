#include "protected_v3_ops.h"

#include <QCryptographicHash>
#include <QSet>

namespace NBAuth::Protected {

namespace {

QString normalizeKeyPlain(const QString &key)
{
    QString out = key.trimmed().toUpper();
    out.remove(' ');
    return out;
}

QString hashSha256Plain(const QString &text)
{
    return QString::fromLatin1(
               QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex())
        .toLower();
}

} // namespace

NB_NOINLINE QString NBVmp_V3_NormalizeKey(const QString &key)
{
    NB_VMP_MUTATE("NB.V3.NormalizeKey");
    const QString out = normalizeKeyPlain(key);
    NB_VMP_END();
    return out;
}

NB_NOINLINE QString NBVmp_V3_HashKeySha256(const QString &normalizedKey)
{
    NB_VMP_VIRTUALIZE("NB.V3.HashKeySha256");
    const QString out = hashSha256Plain(normalizedKey);
    NB_VMP_END();
    return out;
}

NB_NOINLINE QString NBVmp_V3_MakeKeyId(const QString &keyHash)
{
    NB_VMP_MUTATE("NB.V3.MakeKeyId");
    QString out;
    if (keyHash.size() >= 16) {
        out = QStringLiteral("kid_%1").arg(keyHash.left(16));
    }
    NB_VMP_END();
    return out;
}

NB_NOINLINE QJsonArray NBVmp_V3_NormalizeFeatureFlags(const QJsonArray &flags)
{
    NB_VMP_VIRTUALIZE("NB.V3.NormalizeFeatureFlags");
    QJsonArray out;
    QSet<QString> seen;

    for (const QJsonValue &value : flags) {
        const QString normalized = value.toString().trimmed().toLower();
        if (normalized.isEmpty() || seen.contains(normalized)) {
            continue;
        }
        seen.insert(normalized);
        out.append(normalized);
    }

    NB_VMP_END();
    return out;
}

} // namespace NBAuth::Protected
