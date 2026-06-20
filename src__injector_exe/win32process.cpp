#include "win32process.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <QPair>

static QString getProcessPath(uint pid)
{
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) return QString();
    wchar_t buf[MAX_PATH];
    DWORD size = MAX_PATH;
    QString path;
    if (QueryFullProcessImageNameW(hProc, 0, buf, &size)) {
        path = QString::fromWCharArray(buf, size);
    }
    CloseHandle(hProc);
    return path;
}

static BOOL CALLBACK enumWindowsCallback(HWND hwnd, LPARAM lParam)
{
    auto* data = reinterpret_cast<QPair<uint, WindowInfo*>*>(lParam);
    uint targetPid = data->first;
    WindowInfo* result = data->second;

    DWORD wp = 0;
    GetWindowThreadProcessId(hwnd, &wp);
    if (wp == targetPid) {
        if (!IsWindowVisible(hwnd)) return TRUE;
        wchar_t buf[256];
        int len = GetWindowTextW(hwnd, buf, 256);
        QString title = len > 0 ? QString::fromWCharArray(buf, len) : QString("(PID %1)").arg(targetPid);
        if (result->hwnd == nullptr || len > 0) {
            result->hwnd = hwnd;
            result->title = title;
            if (len > 0) return FALSE;
        }
    }
    return TRUE;
}

QList<ProcessInfo> Win32Process::findAllProcesses(const QString& name)
{
    QList<ProcessInfo> result;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return result;

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snap, &entry)) {
        do {
            QString exeName = QString::fromWCharArray(entry.szExeFile);
            if (exeName.compare(name, Qt::CaseInsensitive) == 0) {
                ProcessInfo info;
                info.pid = entry.th32ProcessID;
                info.exe = exeName;
                info.path = getProcessPath(entry.th32ProcessID);
                result.append(info);
            }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);
    return result;
}

WindowInfo Win32Process::getWindowForPid(uint pid)
{
    WindowInfo result;
    QPair<uint, WindowInfo*> data(pid, &result);
    EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return result;
}

void Win32Process::bringToFront(uint pid)
{
    WindowInfo winfo = getWindowForPid(pid);
    if (!winfo.hwnd) return;
    if (IsIconic(reinterpret_cast<HWND>(winfo.hwnd))) {
        ShowWindow(reinterpret_cast<HWND>(winfo.hwnd), SW_RESTORE);
    }
    SetForegroundWindow(reinterpret_cast<HWND>(winfo.hwnd));
}
