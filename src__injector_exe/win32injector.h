#ifndef WIN32INJECTOR_H
#define WIN32INJECTOR_H

#include <QString>
#include <functional>

struct ProcessInfo {
    uint pid = 0;
    QString exe;
    QString path;
};

struct WindowInfo {
    void* hwnd = nullptr;
    QString title;
};

class Win32Injector
{
public:
    Win32Injector() = delete;
    static bool injectDll(uint pid, const QString& dllPath,
                          const std::function<void(const QString&)>& logCb = nullptr);
};

#endif
