#pragma once

#include <QJsonArray>
#include <QString>

#include "crypto/vmp_defs.h"

namespace NBAuth::Protected {

NB_NOINLINE QString NBVmp_V3_NormalizeKey(const QString &key);
NB_NOINLINE QString NBVmp_V3_HashKeySha256(const QString &normalizedKey);
NB_NOINLINE QString NBVmp_V3_MakeKeyId(const QString &keyHash);
NB_NOINLINE QJsonArray NBVmp_V3_NormalizeFeatureFlags(const QJsonArray &flags);

} // namespace NBAuth::Protected
