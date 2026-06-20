#pragma once

#include <QJsonObject>
#include <QString>

namespace NBAuth::V3 {

class StateManager;

QJsonObject dispatchAction(StateManager &state, const QString &action, const QJsonObject &request);

} // namespace NBAuth::V3
