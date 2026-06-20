#include "name_injector.h"
#include "protected_name_ops.h"

#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

namespace NBName {
namespace {

constexpr wchar_t kModuleName[] = L"Minecraft.Windows.exe";
constexpr DWORD kProcessAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                                 PROCESS_VM_READ | PROCESS_SUSPEND_RESUME;
constexpr DWORD kMemCommitReserve = MEM_COMMIT | MEM_RESERVE;
constexpr DWORD kExecRw = PAGE_EXECUTE_READWRITE;
constexpr DWORD kReadWrite = PAGE_READWRITE;
constexpr uint64_t kHook1Off = 0x213D0AE;
constexpr uint64_t kHook1RetOff = 0x213D0B3;
constexpr uint64_t kHook2Off = 0x21784DC;
constexpr uint64_t kHook2RetOff = 0x21784E1;
constexpr uint64_t kStrCopyOff = 0x6926E30;
constexpr unsigned char kOrig1[5] = {0xE8, 0x7D, 0x9D, 0x7E, 0x04};
constexpr unsigned char kOrig2[5] = {0xE8, 0x4F, 0xE9, 0x7A, 0x04};

class Handle {
public:
    explicit Handle(HANDLE h = nullptr) : h_(h) {}
    ~Handle() { if (h_ && h_ != INVALID_HANDLE_VALUE) CloseHandle(h_); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    HANDLE get() const { return h_; }
    explicit operator bool() const { return h_ && h_ != INVALID_HANDLE_VALUE; }
private:
    HANDLE h_;
};

void logLine(const LogFn &log, const QString &msg)
{
    if (log) log(msg);
}

bool readMem(HANDLE h, uint64_t addr, void *buf, size_t size)
{
    SIZE_T got = 0;
    return ReadProcessMemory(h, reinterpret_cast<LPCVOID>(addr), buf, size, &got) && got == size;
}

bool writeMem(HANDLE h, uint64_t addr, const void *buf, size_t size)
{
    DWORD oldProtect = 0;
    VirtualProtectEx(h, reinterpret_cast<LPVOID>(addr), size, kExecRw, &oldProtect);
    SIZE_T wrote = 0;
    BOOL ok = WriteProcessMemory(h, reinterpret_cast<LPVOID>(addr), buf, size, &wrote);
    DWORD ignored = 0;
    VirtualProtectEx(h, reinterpret_cast<LPVOID>(addr), size, oldProtect, &ignored);
    FlushInstructionCache(h, reinterpret_cast<LPCVOID>(addr), size);
    return ok && wrote == size;
}

uint64_t moduleBase(uint32_t pid)
{
    Handle snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
    if (!snap) return 0;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    for (BOOL ok = Module32FirstW(snap.get(), &me); ok; ok = Module32NextW(snap.get(), &me)) {
        if (_wcsicmp(me.szModule, kModuleName) == 0) {
            return reinterpret_cast<uint64_t>(me.modBaseAddr);
        }
    }
    return 0;
}

uint64_t allocNear(HANDLE h, uint64_t nearAddr, size_t size)
{
    constexpr uint64_t step = 0x10000;
    uint64_t lo = nearAddr > 0x50000000ull ? nearAddr - 0x50000000ull : 0x10000ull;
    uint64_t hi = nearAddr + 0x50000000ull;
    uint64_t addr = (nearAddr > 0x40000000ull ? nearAddr - 0x40000000ull : lo) & ~(step - 1);
    for (; addr < hi; addr += step) {
        if (addr < lo) continue;
        void *p = VirtualAllocEx(h, reinterpret_cast<LPVOID>(addr), size, kMemCommitReserve, kExecRw);
        if (p) return reinterpret_cast<uint64_t>(p);
    }
    return 0;
}

} // namespace

InjectResult injectName(uint32_t pid, const QString &name, LogFn log)
{
    InjectResult result;
    Handle proc(OpenProcess(kProcessAccess, FALSE, pid));
    if (!proc) {
        result.message = QStringLiteral("Failed to open game process");
        logLine(log, "[NAME] " + result.message);
        return result;
    }

    uint64_t base = moduleBase(pid);
    if (!base) {
        result.message = QStringLiteral("Game main module not found");
        logLine(log, "[NAME] " + result.message);
        return result;
    }

    uint64_t hook1 = base + kHook1Off;
    uint64_t hook1Ret = base + kHook1RetOff;
    uint64_t hook2 = base + kHook2Off;
    uint64_t hook2Ret = base + kHook2RetOff;
    uint64_t strCopy = base + kStrCopyOff;

    logLine(log, QStringLiteral("[NAME] Game process detected, waiting for login code..."));
    unsigned char b1[5] = {};
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (std::chrono::steady_clock::now() < deadline) {
        if (readMem(proc.get(), hook1, b1, sizeof(b1))) {
            if (Protected::NBVmp_Name_IsOriginalHook(b1, kOrig1, sizeof(kOrig1))) break;
            if (Protected::NBVmp_Name_IsAlreadyPatched(b1, sizeof(b1))) {
                result.ok = true;
                result.message = QStringLiteral("This process has already been handled");
                logLine(log, "[NAME] " + result.message);
                return result;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (!Protected::NBVmp_Name_IsOriginalHook(b1, kOrig1, sizeof(kOrig1))) {
        result.message = QStringLiteral("Timed out waiting for login code");
        logLine(log, "[NAME] " + result.message);
        return result;
    }

    unsigned char b2[5] = {};
    if (!readMem(proc.get(), hook2, b2, sizeof(b2))) {
        result.message = QStringLiteral("Failed to read second hook point");
        logLine(log, "[NAME] " + result.message);
        return result;
    }
    if (!Protected::NBVmp_Name_IsOriginalHook(b2, kOrig2, sizeof(kOrig2))) {
        logLine(log, QStringLiteral("[NAME] Second hook bytes differ, continuing"));
    }

    QByteArray nameBytes = name.toUtf8();
    size_t nameLen = static_cast<size_t>(nameBytes.size());
    void *dataMem = VirtualAllocEx(proc.get(), nullptr, nameLen + 1, kMemCommitReserve, kReadWrite);
    if (!dataMem) {
        result.message = QStringLiteral("Failed to allocate name memory");
        logLine(log, "[NAME] " + result.message);
        return result;
    }
    std::vector<unsigned char> nulName(nameBytes.begin(), nameBytes.end());
    nulName.push_back(0);
    SIZE_T wrote = 0;
    if (!WriteProcessMemory(proc.get(), dataMem, nulName.data(), nulName.size(), &wrote) || wrote != nulName.size()) {
        result.message = QStringLiteral("Failed to write name");
        logLine(log, "[NAME] " + result.message);
        return result;
    }

    unsigned char strObj[0x20] = {};
    uint64_t dataAddr = reinterpret_cast<uint64_t>(dataMem);
    uint64_t cap = std::max<uint64_t>(nameLen, 0x10);
    std::memcpy(strObj + 0x00, &dataAddr, sizeof(dataAddr));
    std::memcpy(strObj + 0x10, &nameLen, sizeof(nameLen));
    std::memcpy(strObj + 0x18, &cap, sizeof(cap));
    void *objMem = VirtualAllocEx(proc.get(), nullptr, sizeof(strObj), kMemCommitReserve, kReadWrite);
    if (!objMem || !WriteProcessMemory(proc.get(), objMem, strObj, sizeof(strObj), &wrote) || wrote != sizeof(strObj)) {
        result.message = QStringLiteral("Failed to write name object");
        logLine(log, "[NAME] " + result.message);
        return result;
    }
    uint64_t objAddr = reinterpret_cast<uint64_t>(objMem);

    auto sc1 = Protected::NBVmp_Name_BuildShellcode(objAddr, strCopy, hook1Ret);
    auto sc2 = Protected::NBVmp_Name_BuildShellcode(objAddr, strCopy, hook2Ret);
    uint64_t sc1Addr = allocNear(proc.get(), hook1, sc1.size());
    uint64_t sc2Addr = allocNear(proc.get(), hook2, sc2.size());
    if (!sc1Addr || !sc2Addr) {
        result.message = QStringLiteral("Failed to allocate hook code");
        logLine(log, "[NAME] " + result.message);
        return result;
    }
    if (!writeMem(proc.get(), sc1Addr, sc1.data(), sc1.size()) ||
        !writeMem(proc.get(), sc2Addr, sc2.data(), sc2.size())) {
        result.message = QStringLiteral("Failed to write hook code");
        logLine(log, "[NAME] " + result.message);
        return result;
    }

    unsigned char jmp1[5] = {};
    unsigned char jmp2[5] = {};
    if (!Protected::NBVmp_Name_MakeJmp5(hook1, sc1Addr, jmp1) ||
        !Protected::NBVmp_Name_MakeJmp5(hook2, sc2Addr, jmp2)) {
        result.message = QStringLiteral("Hook jump distance is out of range");
        logLine(log, "[NAME] " + result.message);
        return result;
    }

    bool ok1 = writeMem(proc.get(), hook1, jmp1, sizeof(jmp1));
    bool ok2 = writeMem(proc.get(), hook2, jmp2, sizeof(jmp2));
    result.ok = ok1 && ok2;
    result.message = result.ok ? QStringLiteral("Custom name written; reconnect to apply")
                               : QStringLiteral("Failed to write hook");
    logLine(log, "[NAME] " + result.message);
    return result;
}

} // namespace NBName
