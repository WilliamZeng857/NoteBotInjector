#ifndef WIN32PROCESS_H
#define WIN32PROCESS_H

#include "win32injector.h"
#include <QString>
#include <QList>

class Win32Process
{
public:
    Win32Process() = delete;
    static QList<ProcessInfo> findAllProcesses(const QString& name);
    static WindowInfo getWindowForPid(uint pid);
    static void bringToFront(uint pid);
};

#endif
