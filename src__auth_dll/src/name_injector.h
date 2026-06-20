#pragma once

#include <QString>
#include <functional>
#include <cstdint>

namespace NBName {

using LogFn = std::function<void(const QString&)>;

struct InjectResult {
    bool ok = false;
    QString message;
};

InjectResult injectName(uint32_t pid, const QString &name, LogFn log);

} // namespace NBName
