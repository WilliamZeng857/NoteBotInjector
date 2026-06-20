#include "win32injector.h"
#include <windows.h>
#include <QFileInfo>

bool Win32Injector::injectDll(uint pid, const QString& dllPath,
                              const std::function<void(const QString&)>& logCb)
{
    auto log = [&](const QString& m) {
        if (logCb) logCb(m);
    };

    QString absPath = QFileInfo(dllPath).absoluteFilePath();
    const QString dllName = QFileInfo(absPath).fileName();
    if (!QFileInfo::exists(absPath)) {
        log(QString("[ERR] 未找到业务 DLL: %1").arg(dllName.isEmpty() ? QStringLiteral("unknown") : dllName));
        return false;
    }

    int wcharCount = absPath.length() + 1;
    std::vector<wchar_t> wcharBuf(wcharCount);
    absPath.toWCharArray(wcharBuf.data());
    wcharBuf[wcharCount - 1] = L'\0';

    log("[SYS] 正在打开目标进程");
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) {
        DWORD err = GetLastError();
        log(QString("[ERR] OpenProcess 失败 (err=%1)").arg(err));
        if (err == 5) log("[ERR] 权限不足，请以管理员身份运行");
        return false;
    }

    bool success = false;
    SIZE_T totalBytes = wcharCount * sizeof(wchar_t);

    QFile f(absPath);
    if (f.open(QIODevice::ReadOnly)) {
        f.seek(60);
        QByteArray off = f.read(4);
        quint32 peOff = *reinterpret_cast<const quint32*>(off.constData());
        f.seek(peOff + 4);
        QByteArray mach = f.read(2);
        quint16 machine = *reinterpret_cast<const quint16*>(mach.constData());
        log(QString("[DIAG] PE machine=0x%1 (%2)").arg(machine, 4, 16, QLatin1Char('0'))
            .arg(machine == 0x8664 ? "x64" : (machine == 0x14c ? "x86" : "unknown")));
        f.close();
    }

    log("[SYS] 正在准备远程加载环境");
    LPVOID rmem = VirtualAllocEx(hProc, nullptr, totalBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!rmem) {
        log("[ERR] VirtualAllocEx 失败");
        CloseHandle(hProc);
        return false;
    }

    SIZE_T nw = 0;
    if (!WriteProcessMemory(hProc, rmem, wcharBuf.data(), totalBytes, &nw)) {
        log("[ERR] WriteProcessMemory 失败");
        VirtualFreeEx(hProc, rmem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    HMODULE hK = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLL = GetProcAddress(hK, "LoadLibraryW");
    if (!pLL) {
        log("[ERR] 解析 LoadLibraryW 失败");
        VirtualFreeEx(hProc, rmem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    log("[SYS] 正在创建远程加载线程");

    DWORD tid = 0;
    HANDLE hT = CreateRemoteThread(hProc, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(pLL), rmem, 0, &tid);
    if (!hT) {
        log("[ERR] CreateRemoteThread 失败");
        VirtualFreeEx(hProc, rmem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    log("[SYS] 正在等待业务 DLL 加载");

    const DWORD waitRc = WaitForSingleObject(hT, 10000);
    if (waitRc == WAIT_TIMEOUT) {
        log("[ERR] 远程 LoadLibrary 超时，目标进程未在 10 秒内完成加载");
        CloseHandle(hT);
        VirtualFreeEx(hProc, rmem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }
    if (waitRc != WAIT_OBJECT_0) {
        log(QString("[ERR] 等待远程线程失败 (wait=%1 err=%2)")
                .arg(waitRc)
                .arg(GetLastError()));
        CloseHandle(hT);
        VirtualFreeEx(hProc, rmem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    DWORD ec = 0;
    GetExitCodeThread(hT, &ec);
    CloseHandle(hT);
    VirtualFreeEx(hProc, rmem, 0, MEM_RELEASE);

    if (ec && ec != STILL_ACTIVE) {
        log("[OK] 业务 DLL 已由目标进程加载");
        success = true;
    } else {
        DWORD remoteErr = GetLastError();
        if (ec == STILL_ACTIVE) {
            log("[ERR] 远程线程仍处于运行中，注入结果不可信");
        } else {
            log("[ERR] LoadLibrary 返回 NULL");
        }
        log(QString("[DIAG] GetLastError=%1").arg(remoteErr));
        if (remoteErr == 126) log("[DIAG] -> ERROR_MOD_NOT_FOUND: DLL or dependency missing");
        if (remoteErr == 193) log("[DIAG] -> ERROR_BAD_EXE_FORMAT: architecture mismatch");
        if (remoteErr == 5)  log("[DIAG] -> ERROR_ACCESS_DENIED: blocked by security software");
        if (remoteErr == 127) log("[DIAG] -> ERROR_PROC_NOT_FOUND: DLL exported function missing");
        log("[DIAG] Troubleshooting: check antivirus / Windows Defender / Exploit Protection");
        success = false;
    }

    CloseHandle(hProc);
    return success;
}
