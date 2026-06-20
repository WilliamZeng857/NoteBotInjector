#include "../include/notebot_model.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtGui/QImage>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

namespace {

constexpr int kProtocolVersion = 1;
constexpr int kAbiVersion = 2;
constexpr wchar_t kTargetProcess[] = L"Minecraft.Windows.exe";
constexpr DWORD kStillActive = 259;
constexpr int kOk = 0;
constexpr int kErrInvalidConfig = 1;
constexpr int kErrAlreadyRunning = 2;
constexpr int kErrRuntime = 3;

constexpr DWORD kProcessAccess =
    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_VM_WRITE;
constexpr DWORD kProcessReadAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;

NBM_LogCallback g_logCb = nullptr;
NBM_StateCallback g_stateCb = nullptr;

void emitLog(const QString &msg)
{
    const QByteArray utf8 = msg.toUtf8();
    if (g_logCb) {
        g_logCb(utf8.constData());
    }
}

void emitState(const char *key, const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    if (g_stateCb) {
        g_stateCb(key, utf8.constData());
    }
}

QString hex64(quint64 value)
{
    return QStringLiteral("0x%1").arg(value, 0, 16);
}

quint16 readU16(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 2 > data.size()) {
        return 0;
    }
    quint16 v = 0;
    std::memcpy(&v, data.constData() + offset, sizeof(v));
    return v;
}

quint32 readU32(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 4 > data.size()) {
        return 0;
    }
    quint32 v = 0;
    std::memcpy(&v, data.constData() + offset, sizeof(v));
    return v;
}

qint32 readI32(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 4 > data.size()) {
        return 0;
    }
    qint32 v = 0;
    std::memcpy(&v, data.constData() + offset, sizeof(v));
    return v;
}

void appendU32(QByteArray *out, quint32 value)
{
    out->append(reinterpret_cast<const char *>(&value), 4);
}

void appendI32(QByteArray *out, qint32 value)
{
    out->append(reinterpret_cast<const char *>(&value), 4);
}

void appendU64(QByteArray *out, quint64 value)
{
    out->append(reinterpret_cast<const char *>(&value), 8);
}

QByteArray movR11Imm(quint64 addr)
{
    QByteArray out;
    out.append(char(0x49));
    out.append(char(0xBB));
    appendU64(&out, addr);
    return out;
}

QByteArray incCounter(quint64 counterAddr)
{
    QByteArray out = movR11Imm(counterAddr);
    out.append("\x49\xFF\x03", 3);
    return out;
}

struct ProcessHandle {
    HANDLE h = nullptr;
    ~ProcessHandle()
    {
        if (h) {
            CloseHandle(h);
        }
    }
    ProcessHandle() = default;
    explicit ProcessHandle(HANDLE handle) : h(handle) {}
    ProcessHandle(const ProcessHandle &) = delete;
    ProcessHandle &operator=(const ProcessHandle &) = delete;
    ProcessHandle(ProcessHandle &&other) noexcept : h(other.h) { other.h = nullptr; }
    ProcessHandle &operator=(ProcessHandle &&other) noexcept
    {
        if (this != &other) {
            if (h) {
                CloseHandle(h);
            }
            h = other.h;
            other.h = nullptr;
        }
        return *this;
    }
    explicit operator bool() const { return h != nullptr; }
};

QByteArray readMem(HANDLE process, quint64 addr, qsizetype size)
{
    if (!process || addr == 0 || size <= 0) {
        return {};
    }
    QByteArray out;
    out.resize(size);
    SIZE_T got = 0;
    if (!ReadProcessMemory(process,
                           reinterpret_cast<LPCVOID>(addr),
                           out.data(),
                           static_cast<SIZE_T>(size),
                           &got)) {
        return {};
    }
    out.resize(static_cast<qsizetype>(got));
    return out;
}

bool writeMem(HANDLE process, quint64 addr, const QByteArray &data, bool executable)
{
    if (!process || addr == 0 || data.isEmpty()) {
        return false;
    }
    DWORD oldProtect = 0;
    const DWORD protect = executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    const BOOL changed = VirtualProtectEx(process,
                                          reinterpret_cast<LPVOID>(addr),
                                          static_cast<SIZE_T>(data.size()),
                                          protect,
                                          &oldProtect);
    SIZE_T written = 0;
    const BOOL ok = WriteProcessMemory(process,
                                       reinterpret_cast<LPVOID>(addr),
                                       data.constData(),
                                       static_cast<SIZE_T>(data.size()),
                                       &written);
    if (changed) {
        DWORD restore = 0;
        VirtualProtectEx(process,
                         reinterpret_cast<LPVOID>(addr),
                         static_cast<SIZE_T>(data.size()),
                         oldProtect,
                         &restore);
    }
    return ok && written == static_cast<SIZE_T>(data.size());
}

std::optional<quint64> allocRemote(HANDLE process, qsizetype size, std::optional<quint64> nearAddr, bool executable)
{
    const DWORD protect = executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    if (nearAddr) {
        constexpr quint64 step = 0x10000;
        const quint64 anchor = *nearAddr;
        const quint64 lo = anchor > 0x50000000ULL ? anchor - 0x50000000ULL : 0x10000ULL;
        const quint64 hi = anchor + 0x50000000ULL;
        quint64 addr = (anchor > 0x40000000ULL ? anchor - 0x40000000ULL : 0x10000ULL) & ~(step - 1);
        while (addr < hi) {
            if (addr >= lo) {
                LPVOID result = VirtualAllocEx(process,
                                               reinterpret_cast<LPVOID>(addr),
                                               static_cast<SIZE_T>(size),
                                               MEM_COMMIT | MEM_RESERVE,
                                               protect);
                if (result) {
                    return reinterpret_cast<quint64>(result);
                }
            }
            addr += step;
        }
    }
    LPVOID result = VirtualAllocEx(process,
                                   nullptr,
                                   static_cast<SIZE_T>(size),
                                   MEM_COMMIT | MEM_RESERVE,
                                   protect);
    if (!result) {
        return std::nullopt;
    }
    return reinterpret_cast<quint64>(result);
}

bool processAlive(HANDLE process)
{
    DWORD code = 0;
    if (!GetExitCodeProcess(process, &code)) {
        return false;
    }
    return code == kStillActive;
}

std::vector<DWORD> enumTargetPids()
{
    std::vector<DWORD> out;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!snapshot || snapshot == INVALID_HANDLE_VALUE) {
        return out;
    }
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    BOOL ok = Process32FirstW(snapshot, &entry);
    while (ok) {
        if (_wcsicmp(entry.szExeFile, kTargetProcess) == 0) {
            out.push_back(entry.th32ProcessID);
        }
        ok = Process32NextW(snapshot, &entry);
    }
    CloseHandle(snapshot);
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

ProcessHandle openProcess(DWORD pid, bool write)
{
    return ProcessHandle(OpenProcess(write ? kProcessAccess : kProcessReadAccess, FALSE, pid));
}

struct ModuleInfo {
    quint64 base = 0;
    quint32 size = 0;
};

std::optional<ModuleInfo> getModuleBase(HANDLE process)
{
    HMODULE mods[2048]{};
    DWORD needed = 0;
    if (!EnumProcessModulesEx(process,
                              mods,
                              sizeof(mods),
                              &needed,
                              LIST_MODULES_ALL)) {
        return std::nullopt;
    }
    const int count = static_cast<int>(needed / sizeof(HMODULE));
    wchar_t path[MAX_PATH * 4]{};
    for (int i = 0; i < count; ++i) {
        path[0] = 0;
        GetModuleFileNameExW(process, mods[i], path, DWORD(std::size(path)));
        const QString fileName = QFileInfo(QString::fromWCharArray(path)).fileName();
        if (fileName.compare(QStringLiteral("Minecraft.Windows.exe"), Qt::CaseInsensitive) != 0) {
            continue;
        }
        MODULEINFO info{};
        if (!GetModuleInformation(process, mods[i], &info, sizeof(info))) {
            continue;
        }
        return ModuleInfo{reinterpret_cast<quint64>(info.lpBaseOfDll), info.SizeOfImage};
    }
    return std::nullopt;
}

struct Section {
    QString name;
    quint64 start = 0;
    quint32 size = 0;
    quint32 rva = 0;
};

std::vector<Section> getModuleSections(HANDLE process, quint64 base)
{
    std::vector<Section> sections;
    const QByteArray header = readMem(process, base, 0x1000);
    if (header.size() < 0x100 || header.left(2) != QByteArrayLiteral("MZ")) {
        return sections;
    }
    const quint32 peOff = readU32(header, 0x3C);
    const QByteArray nt = readMem(process, base + peOff, 0x108);
    if (nt.size() < 0x18 || nt.left(4) != QByteArray("PE\0\0", 4)) {
        return sections;
    }
    const quint16 sectionCount = readU16(nt, 6);
    const quint16 optionalSize = readU16(nt, 20);
    const quint64 tableAddr = base + peOff + 24 + optionalSize;
    const QByteArray raw = readMem(process, tableAddr, sectionCount * 40);
    for (int i = 0; i < sectionCount; ++i) {
        const int off = i * 40;
        if (off + 40 > raw.size()) {
            break;
        }
        QByteArray nameBytes = raw.mid(off, 8);
        const int nul = nameBytes.indexOf('\0');
        if (nul >= 0) {
            nameBytes.truncate(nul);
        }
        const QString name = QString::fromLatin1(nameBytes).trimmed();
        const quint32 virtualSize = readU32(raw, off + 8);
        const quint32 virtualAddress = readU32(raw, off + 12);
        const quint32 rawSize = readU32(raw, off + 16);
        const quint32 size = (std::max)(virtualSize, rawSize);
        if (virtualAddress == 0 || size == 0) {
            continue;
        }
        sections.push_back(Section{name.isEmpty() ? QStringLiteral("sec%1").arg(i) : name,
                                   base + virtualAddress,
                                   size,
                                   virtualAddress});
    }
    return sections;
}

const Section *sectionByName(const std::vector<Section> &sections, const QString &name)
{
    for (const Section &section : sections) {
        if (section.name.compare(name, Qt::CaseInsensitive) == 0) {
            return &section;
        }
    }
    return nullptr;
}

const Section *sectionContainingRva(const std::vector<Section> &sections, quint64 base, quint32 rva)
{
    const quint64 addr = base + rva;
    for (const Section &section : sections) {
        if (section.start <= addr && addr < section.start + section.size) {
            return &section;
        }
    }
    return nullptr;
}

std::vector<int> findAll(const QByteArray &data, const QByteArray &needle)
{
    std::vector<int> hits;
    int pos = data.indexOf(needle);
    while (pos >= 0) {
        hits.push_back(pos);
        pos = data.indexOf(needle, pos + 1);
    }
    return hits;
}

std::vector<quint64> findAsciiStringAddrs(const QByteArray &data, quint64 sectionStart, const QByteArray &text)
{
    std::vector<quint64> out;
    const QByteArray needle = text + QByteArray(1, '\0');
    for (int pos : findAll(data, needle)) {
        out.push_back(sectionStart + quint64(pos));
    }
    return out;
}

struct RipPattern {
    QByteArray bytes;
    int dispOffset = 0;
    int insnLen = 0;
};

const std::vector<RipPattern> &ripPatterns()
{
    static const std::vector<RipPattern> patterns{
        {QByteArray::fromHex("488D0D"), 3, 7},
        {QByteArray::fromHex("488D15"), 3, 7},
        {QByteArray::fromHex("488D35"), 3, 7},
        {QByteArray::fromHex("488D3D"), 3, 7},
        {QByteArray::fromHex("F20F1005"), 4, 8},
    };
    return patterns;
}

std::unordered_map<std::string, std::vector<quint64>> findRipRefsForTargets(
    const QByteArray &textData,
    quint64 textStart,
    const std::unordered_map<quint64, std::vector<std::string>> &targetFields)
{
    std::unordered_map<std::string, std::vector<quint64>> refs;
    for (const RipPattern &pattern : ripPatterns()) {
        int pos = textData.indexOf(pattern.bytes);
        while (pos >= 0) {
            if (pos + pattern.insnLen <= textData.size()) {
                const qint32 disp = readI32(textData, pos + pattern.dispOffset);
                const quint64 target = textStart + quint64(pos) + quint64(pattern.insnLen) + disp;
                const auto it = targetFields.find(target);
                if (it != targetFields.end()) {
                    const quint64 ref = textStart + quint64(pos);
                    for (const std::string &field : it->second) {
                        refs[field].push_back(ref);
                    }
                }
            }
            pos = textData.indexOf(pattern.bytes, pos + 1);
        }
    }
    return refs;
}

std::optional<quint64> findFunctionStartFromRef(const QByteArray &textData, quint64 textStart, quint64 refAddr)
{
    const qint64 pos = qint64(refAddr - textStart);
    if (pos < 0 || pos >= textData.size()) {
        return std::nullopt;
    }
    const std::vector<QByteArray> prologues{
        QByteArray::fromHex("40555356574154415541564157"),
        QByteArray::fromHex("555356574154415541564157"),
    };
    std::vector<int> starts;
    for (const QByteArray &prologue : prologues) {
        const int from = std::max<qint64>(0, pos - 0x8000);
        const int start = textData.lastIndexOf(prologue, int(pos));
        if (start >= from) {
            starts.push_back(start);
        }
    }
    if (starts.empty()) {
        return std::nullopt;
    }
    return textStart + quint64(*std::min_element(starts.begin(), starts.end()));
}

std::optional<quint64> findNearCall(const QByteArray &textData,
                                    quint64 textStart,
                                    quint64 anchor,
                                    const QString &mode,
                                    int index,
                                    quint64 moduleStart,
                                    quint64 moduleEnd,
                                    std::optional<quint64> functionStart)
{
    const qint64 pos = qint64(anchor - textStart);
    if (pos < 0 || pos >= textData.size()) {
        return std::nullopt;
    }
    constexpr int window = 0x120;
    auto valid = [&](int i) -> bool {
        if (i < 0 || i + 5 > textData.size() || uchar(textData.at(i)) != 0xE8) {
            return false;
        }
        const qint32 rel = readI32(textData, i + 1);
        const quint64 target = textStart + quint64(i) + 5 + rel;
        return moduleStart <= target && target < moduleEnd;
    };
    if (mode == QStringLiteral("prev_call")) {
        const int minPos = int(std::max<qint64>(functionStart ? qint64(*functionStart - textStart) : 0,
                                                pos - window));
        int seen = 0;
        for (int i = int(pos) - 1; i >= minPos; --i) {
            if (valid(i)) {
                ++seen;
                if (seen >= index) {
                    return textStart + quint64(i);
                }
            }
        }
    } else {
        const int maxPos = std::min<int>(textData.size(), int(pos) + window);
        int seen = 0;
        for (int i = int(pos); i < maxPos; ++i) {
            if (valid(i)) {
                ++seen;
                if (seen >= index) {
                    return textStart + quint64(i);
                }
            }
        }
    }
    return std::nullopt;
}

enum class HookKind {
    SkinId,
    SkinData,
    UInt32,
    ResourcePatch,
    GeometryData,
    GeometryEngineVersion,
    AnimationData,
    ArmSize,
    Bool,
};

struct HookDef {
    QString name;
    quint32 rva = 0;
    QByteArray expected;
    HookKind kind = HookKind::Bool;
    QString arg;
    int value = 0;
    int shellSize = 0x200;
    QString group;
    QString hookId;
    quint64 address = 0;
    QString resolvedBy;
};

std::vector<HookDef> baseHooks()
{
    return {
        {"force_skin_id", 0x21719b1, QByteArray::fromHex("e85a8e7b04"), HookKind::SkinId, {}, 0, 0x200},
        {"patch_skin_data_head_face", 0x2171ac2, QByteArray::fromHex("e8d904adfe"), HookKind::SkinData, {}, 0, 0x2000},
        {"force_skin_width", 0x2171c06, QByteArray::fromHex("e8e9f4e300"), HookKind::UInt32, "skin_width", 0, 0x200},
        {"force_skin_height", 0x2171c39, QByteArray::fromHex("e8e9c1e301"), HookKind::UInt32, "skin_height", 0, 0x200},
        {"force_resource_patch", 0x2171efd, QByteArray::fromHex("e8be4a7b04"), HookKind::ResourcePatch, {}, 0, 0x200},
        {"force_geometry_data", 0x2171f8f, QByteArray::fromHex("e82c4a7b04"), HookKind::GeometryData, {}, 0, 0x200},
        {"force_geometry_engine_version", 0x217207c, QByteArray::fromHex("e83f497b04"), HookKind::GeometryEngineVersion, {}, 0, 0x200},
        {"force_animation_data", 0x217215b, QByteArray::fromHex("e860487b04"), HookKind::AnimationData, {}, 0, 0x200},
        {"force_arm_size", 0x21725ec, QByteArray::fromHex("e81f827b04"), HookKind::ArmSize, {}, 0, 0x200},
        {"force_trusted_true", 0x2172a8a, QByteArray::fromHex("e8510f7303"), HookKind::Bool, {}, 1, 0x200},
        {"force_premium_true", 0x2172ac7, QByteArray::fromHex("e874c87203"), HookKind::Bool, {}, 1, 0x200},
        {"force_persona_false", 0x2172b04, QByteArray::fromHex("e817c87203"), HookKind::Bool, {}, 0, 0x200},
    };
}

struct Locator {
    QString group;
    QByteArray field;
    QString mode;
    int index = 1;
};

const std::unordered_map<std::string, Locator> &hookLocators()
{
    static const std::unordered_map<std::string, Locator> locators{
        {"force_skin_id", {"classic_skin", "SkinId", "prev_call", 1}},
        {"patch_skin_data_head_face", {"classic_skin", "SkinData", "prev_call", 2}},
        {"force_skin_width", {"classic_skin", "SkinImageWidth", "prev_call", 1}},
        {"force_skin_height", {"classic_skin", "SkinImageHeight", "prev_call", 1}},
        {"force_resource_patch", {"classic_skin", "SkinResourcePatch", "prev_call", 2}},
        {"force_geometry_data", {"classic_skin", "SkinGeometryData", "prev_call", 2}},
        {"force_geometry_engine_version", {"classic_skin", "SkinGeometryDataEngineVersion", "prev_call", 2}},
        {"force_animation_data", {"classic_skin", "SkinAnimationData", "prev_call", 2}},
        {"force_arm_size", {"classic_skin", "ArmSize", "prev_call", 1}},
        {"force_trusted_true", {"classic_skin", "TrustedSkin", "prev_call", 1}},
        {"force_premium_true", {"classic_skin", "PremiumSkin", "prev_call", 1}},
        {"force_persona_false", {"classic_skin", "PersonaSkin", "prev_call", 1}},
    };
    return locators;
}

struct GroupRequirement {
    int minFields = 0;
    quint32 preferredRva = 0;
    std::vector<QByteArray> fields;
};

const std::unordered_map<std::string, GroupRequirement> &groupRequirements()
{
    static const std::unordered_map<std::string, GroupRequirement> groups{
        {"classic_skin",
         {9,
          0x2171978,
          {"SkinId", "SkinData", "SkinImageWidth", "SkinImageHeight", "SkinResourcePatch",
           "SkinGeometryData", "SkinGeometryDataEngineVersion", "SkinAnimationData", "ArmSize",
           "PersonaPieces", "TrustedSkin", "PremiumSkin", "PersonaSkin"}}},
    };
    return groups;
}

QString hookIdentity(const HookDef &hook)
{
    return hook.hookId.isEmpty() ? hook.name : hook.hookId;
}

quint64 hookAddress(quint64 base, const HookDef &hook)
{
    return hook.address ? hook.address : base + hook.rva;
}

quint64 callTargetFromRel32(quint64 callAddr, const QByteArray &bytes)
{
    const qint32 rel = readI32(bytes, 1);
    return callAddr + 5 + rel;
}

std::pair<bool, QString> hookBytesCompatible(quint64 addr,
                                             const QByteArray &current,
                                             const HookDef &hook,
                                             quint64 moduleBase,
                                             quint32 moduleSize)
{
    if (current == hook.expected) {
        return {true, QStringLiteral("exact")};
    }
    if (current.size() >= 5 &&
        hook.expected.size() >= 5 &&
        uchar(hook.expected.at(0)) == 0xE8 &&
        uchar(current.at(0)) == 0xE8) {
        const quint64 target = callTargetFromRel32(addr, current);
        const quint64 end = moduleBase + moduleSize;
        if (moduleBase <= target && target < end) {
            return {true, QStringLiteral("call_rel32_drift:%1").arg(hex64(target))};
        }
    }
    return {false, QStringLiteral("mismatch")};
}

struct PayloadConfig {
    QString targetProcess = QStringLiteral("Minecraft.Windows.exe");
    QString skinId = QStringLiteral("c18e65aa-7b21-4637-9b63-8ad63622ef01.CustomSlimaf8fd34e3bc6df55dfee6dd80d80a1bb");
    QString geometryPath;
    QString texturePath;
    QString geometryData;
    QString geometryId;
    QString geometryEngineVersion = QStringLiteral("1.12.0");
    QString resourcePatch;
    QString armSize = QStringLiteral("slim");
    QString animationData;
    int skinWidth = 0;
    int skinHeight = 0;
    int trustedValue = 1;
    int premiumValue = 1;
    int personaValue = 1;
    int processTimeoutMs = 86400 * 1000;
    int hookTimeoutMs = 90000;
    int hitTimeoutMs = 45000;
    QByteArray skinRgba;
};

bool readTextFile(const QString &path, QString *out, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("无法读取文件：%1").arg(QFileInfo(path).fileName());
        }
        return false;
    }
    *out = QString::fromUtf8(file.readAll());
    return true;
}

bool parseGeometryInfo(PayloadConfig *cfg, QString *error)
{
    if (!readTextFile(cfg->geometryPath, &cfg->geometryData, error)) {
        return false;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(cfg->geometryData.toUtf8(), &parseError);
    if (!doc.isObject()) {
        if (error) {
            *error = QStringLiteral("geometry JSON 无法解析：%1").arg(parseError.errorString());
        }
        return false;
    }
    const QJsonObject root = doc.object();
    const QJsonArray geometries = root.value(QStringLiteral("minecraft:geometry")).toArray();
    if (geometries.isEmpty()) {
        if (error) {
            *error = QStringLiteral("geometry JSON 缺少 minecraft:geometry");
        }
        return false;
    }
    const QJsonObject desc = geometries.at(0).toObject().value(QStringLiteral("description")).toObject();
    cfg->geometryId = desc.value(QStringLiteral("identifier")).toString().trimmed();
    if (cfg->geometryId.isEmpty()) {
        cfg->geometryId = QStringLiteral("geometry.unknown");
    }
    cfg->skinWidth = desc.value(QStringLiteral("texture_width")).toInt(cfg->skinWidth ? cfg->skinWidth : 64);
    cfg->skinHeight = desc.value(QStringLiteral("texture_height")).toInt(cfg->skinHeight ? cfg->skinHeight : 64);
    const QString format = root.value(QStringLiteral("format_version")).toString().trimmed();
    if (!format.isEmpty()) {
        cfg->geometryEngineVersion = format;
    }
    if (cfg->resourcePatch.isEmpty()) {
        cfg->resourcePatch = QString::fromUtf8(QJsonDocument(QJsonObject{
            {QStringLiteral("geometry"), QJsonObject{{QStringLiteral("default"), cfg->geometryId}}}
        }).toJson(QJsonDocument::Compact));
    }
    return true;
}

bool loadSkin(PayloadConfig *cfg, QString *error)
{
    QImage image(cfg->texturePath);
    if (image.isNull()) {
        if (error) {
            *error = QStringLiteral("无法读取皮肤 PNG：%1").arg(QFileInfo(cfg->texturePath).fileName());
        }
        return false;
    }
    const QSize size = image.size();
    if (!(size == QSize(64, 64) || size == QSize(128, 128) || size == QSize(256, 256))) {
        if (error) {
            *error = QStringLiteral("皮肤 PNG 必须是 64x64 / 128x128 / 256x256，当前是 %1x%2")
                         .arg(size.width())
                         .arg(size.height());
        }
        return false;
    }
    cfg->skinWidth = size.width();
    cfg->skinHeight = size.height();
    QImage rgba = image.convertToFormat(QImage::Format_RGBA8888);
    cfg->skinRgba = QByteArray(reinterpret_cast<const char *>(rgba.constBits()), rgba.sizeInBytes());
    return true;
}

std::optional<PayloadConfig> parseConfig(const char *json, QString *error)
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json ? json : "{}"), &parseError);
    if (!doc.isObject()) {
        if (error) {
            *error = QStringLiteral("模型替换配置 JSON 无法解析：%1").arg(parseError.errorString());
        }
        return std::nullopt;
    }
    const QJsonObject obj = doc.object();
    PayloadConfig cfg;
    cfg.skinId = obj.value(QStringLiteral("skin_id")).toString(cfg.skinId).trimmed();
    cfg.geometryPath = obj.value(QStringLiteral("geometry_path")).toString().trimmed();
    cfg.texturePath = obj.value(QStringLiteral("texture_path")).toString().trimmed();
    cfg.resourcePatch = obj.value(QStringLiteral("resource_patch")).toString().trimmed();
    cfg.geometryEngineVersion = obj.value(QStringLiteral("geometry_engine_version")).toString(cfg.geometryEngineVersion).trimmed();
    cfg.armSize = obj.value(QStringLiteral("arm_size")).toString(cfg.armSize).trimmed();
    cfg.trustedValue = obj.value(QStringLiteral("trusted_raw")).toInt(obj.value(QStringLiteral("trusted")).toBool(true) ? 1 : 0);
    cfg.premiumValue = obj.value(QStringLiteral("premium")).toBool(true) ? 1 : 0;
    cfg.personaValue = obj.value(QStringLiteral("persona")).toBool(true) ? 1 : 0;
    cfg.processTimeoutMs = obj.value(QStringLiteral("process_timeout_ms")).toInt(cfg.processTimeoutMs);
    cfg.hookTimeoutMs = obj.value(QStringLiteral("hook_timeout_ms")).toInt(cfg.hookTimeoutMs);
    cfg.hitTimeoutMs = obj.value(QStringLiteral("hit_timeout_ms")).toInt(cfg.hitTimeoutMs);
    if (cfg.geometryPath.isEmpty() || cfg.texturePath.isEmpty()) {
        if (error) {
            *error = QStringLiteral("缺少 geometry_path 或 texture_path");
        }
        return std::nullopt;
    }
    if (!QFileInfo::exists(cfg.geometryPath) || !QFileInfo::exists(cfg.texturePath)) {
        if (error) {
            *error = QStringLiteral("模型资源文件不存在");
        }
        return std::nullopt;
    }
    if (!loadSkin(&cfg, error) || !parseGeometryInfo(&cfg, error)) {
        return std::nullopt;
    }
    if (cfg.skinWidth <= 0 || cfg.skinHeight <= 0 || cfg.skinRgba.isEmpty()) {
        if (error) {
            *error = QStringLiteral("皮肤尺寸或数据无效");
        }
        return std::nullopt;
    }
    return cfg;
}

std::vector<HookDef> selectedHooks(const PayloadConfig &cfg)
{
    std::vector<HookDef> hooks;
    for (HookDef hook : baseHooks()) {
        hook.hookId = hook.name;
        bool include = true;
        switch (hook.kind) {
        case HookKind::SkinId:
            include = !cfg.skinId.isEmpty();
            break;
        case HookKind::SkinData:
            include = !cfg.skinRgba.isEmpty();
            break;
        case HookKind::UInt32:
            hook.value = hook.arg == QStringLiteral("skin_width") ? cfg.skinWidth : cfg.skinHeight;
            include = hook.value > 0;
            break;
        case HookKind::ResourcePatch:
            include = !cfg.resourcePatch.isEmpty();
            break;
        case HookKind::GeometryData:
            include = !cfg.geometryData.isEmpty();
            break;
        case HookKind::GeometryEngineVersion:
            include = !cfg.geometryEngineVersion.isEmpty();
            break;
        case HookKind::AnimationData:
            include = !cfg.animationData.isEmpty();
            break;
        case HookKind::ArmSize:
            include = !cfg.armSize.isEmpty();
            break;
        case HookKind::Bool:
            if (hook.name == QStringLiteral("force_trusted_true")) {
                hook.value = cfg.trustedValue;
                hook.name = hook.value ? QStringLiteral("force_trusted_true") : QStringLiteral("force_trusted_false");
            } else if (hook.name == QStringLiteral("force_premium_true")) {
                hook.value = cfg.premiumValue;
                hook.name = hook.value ? QStringLiteral("force_premium_true") : QStringLiteral("force_premium_false");
            } else if (hook.name == QStringLiteral("force_persona_false")) {
                hook.value = cfg.personaValue;
                hook.name = hook.value ? QStringLiteral("force_persona_true") : QStringLiteral("force_persona_false");
            }
            break;
        }
        if (include) {
            hooks.push_back(hook);
        }
    }
    return hooks;
}

bool resolveHookAddresses(HANDLE process,
                          quint64 base,
                          quint32 size,
                          std::vector<HookDef> *hooks)
{
    const auto &locators = hookLocators();
    const auto &groups = groupRequirements();
    std::vector<std::string> neededGroups;
    for (const HookDef &hook : *hooks) {
        const std::string id = hookIdentity(hook).toStdString();
        const auto it = locators.find(id);
        if (it != locators.end()) {
            if (std::find(neededGroups.begin(), neededGroups.end(), it->second.group.toStdString()) == neededGroups.end()) {
                neededGroups.push_back(it->second.group.toStdString());
            }
        }
    }

    const std::vector<Section> sections = getModuleSections(process, base);
    const Section *textSection = sectionByName(sections, QStringLiteral(".text"));
    if (!textSection && !neededGroups.empty()) {
        quint32 preferred = UINT_MAX;
        for (const std::string &group : neededGroups) {
            preferred = (std::min)(preferred, groups.at(group).preferredRva);
        }
        textSection = sectionContainingRva(sections, base, preferred);
    }
    if (!textSection) {
        emitLog(QStringLiteral("[MODEL] 定位替换点失败：模块段不可用"));
        return false;
    }

    std::vector<Section> dataSections;
    if (const Section *rdata = sectionByName(sections, QStringLiteral(".rdata"))) {
        dataSections.push_back(*rdata);
    } else {
        for (const Section &section : sections) {
            if (section.start != textSection->start && section.size <= 0x4000000) {
                dataSections.push_back(section);
            }
        }
    }

    const QByteArray textData = readMem(process, textSection->start, textSection->size);
    if (textData.isEmpty()) {
        emitLog(QStringLiteral("[MODEL] 定位替换点失败：模块读取失败"));
        return false;
    }

    std::vector<QByteArray> neededFields;
    auto addField = [&neededFields](const QByteArray &field) {
        if (std::find(neededFields.begin(), neededFields.end(), field) == neededFields.end()) {
            neededFields.push_back(field);
        }
    };
    for (const std::string &group : neededGroups) {
        for (const QByteArray &field : groups.at(group).fields) {
            addField(field);
        }
    }
    for (const HookDef &hook : *hooks) {
        const auto it = locators.find(hookIdentity(hook).toStdString());
        if (it != locators.end()) {
            addField(it->second.field);
        }
    }

    std::unordered_map<std::string, std::vector<quint64>> fieldStringAddrs;
    for (const QByteArray &field : neededFields) {
        fieldStringAddrs[field.toStdString()] = {};
    }
    for (const Section &section : dataSections) {
        const QByteArray data = readMem(process, section.start, section.size);
        if (data.isEmpty() || data.indexOf(QByteArray("SkinId\0", 7)) < 0) {
            continue;
        }
        bool allFound = true;
        for (const QByteArray &field : neededFields) {
            auto hits = findAsciiStringAddrs(data, section.start, field);
            auto &vec = fieldStringAddrs[field.toStdString()];
            vec.insert(vec.end(), hits.begin(), hits.end());
            if (vec.empty()) {
                allFound = false;
            }
        }
        if (allFound) {
            break;
        }
    }

    std::unordered_map<quint64, std::vector<std::string>> targetFields;
    for (const auto &pair : fieldStringAddrs) {
        for (quint64 addr : pair.second) {
            targetFields[addr].push_back(pair.first);
        }
    }
    const auto rawRefs = findRipRefsForTargets(textData, textSection->start, targetFields);

    struct GroupInfo {
        quint64 start = 0;
        int score = 0;
        std::unordered_map<std::string, std::vector<quint64>> refs;
    };
    std::unordered_map<std::string, GroupInfo> groupInfos;
    for (const std::string &group : neededGroups) {
        const GroupRequirement &req = groups.at(group);
        std::unordered_map<quint64, std::unordered_map<std::string, std::vector<quint64>>> candidates;
        for (const QByteArray &fieldBytes : req.fields) {
            const std::string field = fieldBytes.toStdString();
            const auto refsIt = rawRefs.find(field);
            if (refsIt == rawRefs.end()) {
                continue;
            }
            for (quint64 ref : refsIt->second) {
                const auto funcStart = findFunctionStartFromRef(textData, textSection->start, ref);
                if (funcStart) {
                    candidates[*funcStart][field].push_back(ref);
                }
            }
        }
        struct Score {
            int score = 0;
            quint64 distance = 0;
            quint64 start = 0;
        };
        std::vector<Score> scored;
        const quint64 preferred = base + req.preferredRva;
        for (const auto &candidate : candidates) {
            const int score = int(candidate.second.size());
            if (score >= req.minFields) {
                const quint64 start = candidate.first;
                const quint64 distance = start > preferred ? start - preferred : preferred - start;
                scored.push_back({score, distance, start});
            }
        }
        if (scored.empty()) {
            emitLog(QStringLiteral("[MODEL] 定位替换点失败：目标字段未匹配"));
            return false;
        }
        std::sort(scored.begin(), scored.end(), [](const Score &a, const Score &b) {
            if (a.score != b.score) {
                return a.score > b.score;
            }
            return a.distance < b.distance;
        });
        GroupInfo info;
        info.start = scored.front().start;
        info.score = scored.front().score;
        info.refs = candidates[info.start];
        groupInfos[group] = info;
    }

    std::vector<QString> unresolved;
    for (HookDef &hook : *hooks) {
        const auto locIt = locators.find(hookIdentity(hook).toStdString());
        if (locIt == locators.end()) {
            continue;
        }
        const Locator &locator = locIt->second;
        const std::string group = locator.group.toStdString();
        const std::string field = locator.field.toStdString();
        auto groupIt = groupInfos.find(group);
        if (groupIt == groupInfos.end()) {
            unresolved.push_back(hook.name);
            continue;
        }
        auto refsIt = groupIt->second.refs.find(field);
        if (refsIt == groupIt->second.refs.end() || refsIt->second.empty()) {
            unresolved.push_back(hook.name);
            continue;
        }
        std::sort(refsIt->second.begin(), refsIt->second.end());
        const quint64 anchor = refsIt->second.front();
        const auto addr = findNearCall(textData,
                                       textSection->start,
                                       anchor,
                                       locator.mode,
                                       locator.index,
                                       base,
                                       base + size,
                                       groupIt->second.start);
        if (!addr) {
            unresolved.push_back(hook.name);
            continue;
        }
        hook.address = *addr;
        const QByteArray current = readMem(process, hook.address, 5);
        const auto compat = hookBytesCompatible(hook.address, current, hook, base, size);
        hook.resolvedBy = QStringLiteral("%1:%2:%3:%4:%5")
                              .arg(locator.group,
                                   QString::fromLatin1(locator.field),
                                   locator.mode,
                                   QString::number(locator.index),
                                   compat.second);
    }
    if (!unresolved.empty()) {
        emitLog(QStringLiteral("[MODEL] 定位替换点失败：替换点不完整"));
        return false;
    }
    return true;
}

QByteArray makeJmp5(quint64 from, quint64 to, bool *ok)
{
    const qint64 rel64 = qint64(to) - qint64(from + 5);
    if (rel64 < INT32_MIN || rel64 > INT32_MAX) {
        *ok = false;
        return {};
    }
    *ok = true;
    QByteArray out;
    out.append(char(0xE9));
    appendI32(&out, qint32(rel64));
    return out;
}

QByteArray makeBoolShellcode(quint64 callTarget, quint64 returnAddr, quint64 counterAddr, int value)
{
    QByteArray out = incCounter(counterAddr);
    out.append(char(0xBA));
    appendU32(&out, quint32(value & 0xFF));
    out.append(movR11Imm(callTarget));
    out.append("\x41\xFF\xD3", 3);
    out.append(movR11Imm(returnAddr));
    out.append("\x41\xFF\xE3", 3);
    return out;
}

QByteArray makeStringArgShellcode(quint64 callTarget, quint64 returnAddr, quint64 counterAddr, quint64 stringObjAddr)
{
    QByteArray out = incCounter(counterAddr);
    out.append("\x48\xBA", 2);
    appendU64(&out, stringObjAddr);
    out.append(movR11Imm(callTarget));
    out.append("\x41\xFF\xD3", 3);
    out.append(movR11Imm(returnAddr));
    out.append("\x41\xFF\xE3", 3);
    return out;
}

struct AsmBuilder {
    QByteArray buf;
    std::unordered_map<std::string, int> labels;
    std::vector<std::pair<int, std::string>> fixups;

    void append(const QByteArray &data) { buf.append(data); }
    void label(const std::string &name) { labels[name] = buf.size(); }
    void jcc32(uchar op2, const std::string &target)
    {
        buf.append(char(0x0F));
        buf.append(char(op2));
        const int pos = buf.size();
        appendI32(&buf, 0);
        fixups.push_back({pos, target});
    }
    QByteArray build()
    {
        for (const auto &fixup : fixups) {
            const int pos = fixup.first;
            const int target = labels[fixup.second];
            const qint32 rel = target - (pos + 4);
            std::memcpy(buf.data() + pos, &rel, 4);
        }
        return buf;
    }
};

QByteArray makeSkinDataShellcode(quint64 callTarget,
                                 quint64 returnAddr,
                                 quint64 counterAddr,
                                 quint64 replacementAddr,
                                 int replacementSize)
{
    AsmBuilder a;
    a.append(incCounter(counterAddr));
    a.append(QByteArray::fromHex("4989D2"));
    a.append(QByteArray::fromHex("50515256574150415141524153"));
    a.append(QByteArray::fromHex("4D8B5A18"));
    a.append(QByteArray::fromHex("4D89D1"));
    a.append(QByteArray::fromHex("4983FB0F"));
    a.append(QByteArray::fromHex("7603"));
    a.append(QByteArray::fromHex("4D8B0A"));
    a.append(QByteArray::fromHex("4D85C9"));
    a.jcc32(0x84, "done_patch");
    a.append(QByteArray::fromHex("4D8B5A10"));
    a.append(QByteArray::fromHex("4981FB00100000"));
    a.jcc32(0x82, "done_patch");
    if (replacementAddr && replacementSize > 0) {
        a.append(QByteArray::fromHex("4981FB"));
        appendU32(&a.buf, quint32(replacementSize));
        a.jcc32(0x82, "done_patch");
        a.append(QByteArray::fromHex("4C89CF"));
        a.append(QByteArray::fromHex("48BE"));
        appendU64(&a.buf, replacementAddr);
        a.append(QByteArray::fromHex("48C7C1"));
        appendU32(&a.buf, quint32(replacementSize));
        a.append(QByteArray::fromHex("F3A4"));
    }
    a.label("done_patch");
    a.append(QByteArray::fromHex("415B415A415941585F5E5A5958"));
    a.append(movR11Imm(callTarget));
    a.append(QByteArray::fromHex("41FFD3"));
    a.append(movR11Imm(returnAddr));
    a.append(QByteArray::fromHex("41FFE3"));
    return a.build();
}

std::optional<quint64> makeRemoteStdStringBytes(HANDLE process, const QByteArray &data)
{
    const auto dataAddr = allocRemote(process, data.size() + 1, std::nullopt, false);
    if (!dataAddr) {
        return std::nullopt;
    }
    if (!writeMem(process, *dataAddr, data + QByteArray(1, '\0'), false)) {
        return std::nullopt;
    }
    QByteArray obj(0x20, '\0');
    quint64 ptr = *dataAddr;
    quint64 len = quint64(data.size());
    quint64 cap = std::max<quint64>(len, 0x10);
    std::memcpy(obj.data() + 0x00, &ptr, 8);
    std::memcpy(obj.data() + 0x10, &len, 8);
    std::memcpy(obj.data() + 0x18, &cap, 8);
    const auto objAddr = allocRemote(process, 0x20, std::nullopt, false);
    if (!objAddr) {
        return std::nullopt;
    }
    if (!writeMem(process, *objAddr, obj, false)) {
        return std::nullopt;
    }
    return objAddr;
}

std::optional<quint64> makeRemoteString(HANDLE process, const QString &text)
{
    return makeRemoteStdStringBytes(process, text.toUtf8());
}

struct RemotePayload {
    quint64 skinId = 0;
    quint64 resourcePatch = 0;
    quint64 geometryData = 0;
    quint64 geometryEngineVersion = 0;
    quint64 animationData = 0;
    quint64 armSize = 0;
    quint64 fakeSkinData = 0;
    quint64 replacementSkin = 0;
    int replacementSkinSize = 0;
};

bool prepareRemotePayload(HANDLE process, const PayloadConfig &cfg, RemotePayload *remote)
{
    auto makeText = [&](const QString &label, const QString &value, quint64 *target) -> bool {
        if (value.isEmpty()) {
            return true;
        }
        const auto addr = makeRemoteString(process, value);
        if (!addr) {
            Q_UNUSED(label);
            emitLog(QStringLiteral("[MODEL] 准备替换数据失败：资源分配失败"));
            return false;
        }
        *target = *addr;
        return true;
    };
    if (!makeText(QStringLiteral("SkinId"), cfg.skinId, &remote->skinId) ||
        !makeText(QStringLiteral("SkinResourcePatch"), cfg.resourcePatch, &remote->resourcePatch) ||
        !makeText(QStringLiteral("SkinGeometryData"), cfg.geometryData, &remote->geometryData) ||
        !makeText(QStringLiteral("SkinGeometryDataEngineVersion"), cfg.geometryEngineVersion, &remote->geometryEngineVersion) ||
        !makeText(QStringLiteral("SkinAnimationData"), cfg.animationData, &remote->animationData) ||
        !makeText(QStringLiteral("ArmSize"), cfg.armSize, &remote->armSize)) {
        return false;
    }
    const auto skinObj = makeRemoteStdStringBytes(process, cfg.skinRgba);
    if (!skinObj) {
        emitLog(QStringLiteral("[MODEL] 准备替换数据失败：贴图对象分配失败"));
        return false;
    }
    remote->fakeSkinData = *skinObj;
    const auto skinBytes = allocRemote(process, cfg.skinRgba.size(), std::nullopt, false);
    if (!skinBytes) {
        emitLog(QStringLiteral("[MODEL] 准备替换数据失败：贴图内存分配失败"));
        return false;
    }
    if (!writeMem(process, *skinBytes, cfg.skinRgba, false)) {
        emitLog(QStringLiteral("[MODEL] 写入皮肤贴图远程内存失败"));
        return false;
    }
    remote->replacementSkin = *skinBytes;
    remote->replacementSkinSize = cfg.skinRgba.size();
    return true;
}

bool installHook(HANDLE process,
                 quint64 base,
                 quint32 moduleSize,
                 const HookDef &hook,
                 const RemotePayload &remote,
                 quint64 *counterOut)
{
    const quint64 addr = hookAddress(base, hook);
    const QByteArray current = readMem(process, addr, 5);
    if (current.startsWith(char(0xE9))) {
        return false;
    }
    const auto compat = hookBytesCompatible(addr, current, hook, base, moduleSize);
    if (!compat.first) {
        return false;
    }
    const auto shellAddr = allocRemote(process, hook.shellSize, addr, true);
    if (!shellAddr) {
        return false;
    }
    const quint64 counterAddr = *shellAddr + hook.shellSize - 0x10;
    const quint64 callTarget = callTargetFromRel32(addr, current);
    const quint64 returnAddr = addr + 5;

    QByteArray shell;
    auto remoteStringForKind = [&]() -> quint64 {
        switch (hook.kind) {
        case HookKind::SkinId: return remote.skinId;
        case HookKind::ResourcePatch: return remote.resourcePatch;
        case HookKind::GeometryData: return remote.geometryData;
        case HookKind::GeometryEngineVersion: return remote.geometryEngineVersion;
        case HookKind::AnimationData: return remote.animationData;
        case HookKind::ArmSize: return remote.armSize;
        default: return 0;
        }
    };

    if (hook.kind == HookKind::Bool || hook.kind == HookKind::UInt32) {
        shell = makeBoolShellcode(callTarget, returnAddr, counterAddr, hook.value);
    } else if (hook.kind == HookKind::SkinData) {
        if (remote.fakeSkinData) {
            shell = makeStringArgShellcode(callTarget, returnAddr, counterAddr, remote.fakeSkinData);
        } else {
            shell = makeSkinDataShellcode(callTarget,
                                          returnAddr,
                                          counterAddr,
                                          remote.replacementSkin,
                                          remote.replacementSkinSize);
        }
    } else {
        const quint64 str = remoteStringForKind();
        if (!str) {
            return false;
        }
        shell = makeStringArgShellcode(callTarget, returnAddr, counterAddr, str);
    }

    if (shell.size() >= hook.shellSize - 0x20) {
        return false;
    }
    if (!writeMem(process, *shellAddr, shell, true)) {
        return false;
    }
    QByteArray zero(8, '\0');
    writeMem(process, counterAddr, zero, false);
    bool jmpOk = false;
    const QByteArray jmp = makeJmp5(addr, *shellAddr, &jmpOk);
    if (!jmpOk || !writeMem(process, addr, jmp, true)) {
        return false;
    }
    *counterOut = counterAddr;
    return true;
}

std::optional<quint64> readU64Remote(HANDLE process, quint64 addr)
{
    const QByteArray data = readMem(process, addr, 8);
    if (data.size() != 8) {
        return std::nullopt;
    }
    quint64 value = 0;
    std::memcpy(&value, data.constData(), 8);
    return value;
}

bool probeProcessReady(DWORD pid, const PayloadConfig &cfg)
{
    ProcessHandle process = openProcess(pid, false);
    if (!process || !processAlive(process.h)) {
        return false;
    }
    const auto module = getModuleBase(process.h);
    if (!module) {
        return false;
    }
    std::vector<HookDef> hooks = selectedHooks(cfg);
    if (!resolveHookAddresses(process.h, module->base, module->size, &hooks)) {
        return false;
    }
    for (const HookDef &hook : hooks) {
        const quint64 addr = hookAddress(module->base, hook);
        const QByteArray current = readMem(process.h, addr, 5);
        if (!hookBytesCompatible(addr, current, hook, module->base, module->size).first) {
            return false;
        }
    }
    return true;
}

bool patchProcess(DWORD pid, const PayloadConfig &cfg)
{
    emitState("model_state", QStringLiteral("正在准备替换"));
    ProcessHandle process = openProcess(pid, true);
    if (!process) {
        emitLog(QStringLiteral("[MODEL] 打开游戏进程失败，可能权限不够"));
        return false;
    }
    const qint64 moduleDeadline = QDateTime::currentMSecsSinceEpoch() + 30000;
    std::optional<ModuleInfo> module;
    while (QDateTime::currentMSecsSinceEpoch() < moduleDeadline) {
        if (!processAlive(process.h)) {
            emitLog(QStringLiteral("[MODEL] 游戏进程已退出"));
            return false;
        }
        module = getModuleBase(process.h);
        if (module) {
            emitLog(QStringLiteral("[MODEL] 模块已就绪"));
            break;
        }
        QThread::msleep(20);
    }
    if (!module) {
        emitLog(QStringLiteral("[MODEL] 找不到游戏主模块"));
        return false;
    }

    RemotePayload remote;
    if (!prepareRemotePayload(process.h, cfg, &remote)) {
        return false;
    }

    std::vector<HookDef> hooks = selectedHooks(cfg);
    if (!resolveHookAddresses(process.h, module->base, module->size, &hooks)) {
        return false;
    }

    std::unordered_map<std::string, quint64> counters;
    const qint64 hookDeadline = QDateTime::currentMSecsSinceEpoch() + cfg.hookTimeoutMs;
    while (QDateTime::currentMSecsSinceEpoch() < hookDeadline && counters.size() < hooks.size()) {
        if (!processAlive(process.h)) {
            emitLog(QStringLiteral("[MODEL] 游戏进程已退出"));
            return false;
        }
        for (const HookDef &hook : hooks) {
            if (counters.find(hook.name.toStdString()) != counters.end()) {
                continue;
            }
            const quint64 addr = hookAddress(module->base, hook);
            const QByteArray current = readMem(process.h, addr, 5);
            if (!hookBytesCompatible(addr, current, hook, module->base, module->size).first) {
                continue;
            }
            quint64 counter = 0;
            if (installHook(process.h, module->base, module->size, hook, remote, &counter)) {
                counters[hook.name.toStdString()] = counter;
            }
        }
        QThread::msleep(2);
    }
    emitState("model_state", QStringLiteral("修改点已安装"));

    int missing = 0;
    for (const HookDef &hook : hooks) {
        if (counters.find(hook.name.toStdString()) == counters.end()) {
            ++missing;
        }
    }
    emitLog(QStringLiteral("[MODEL] 修改点安装完成：%1/%2")
                .arg(static_cast<qulonglong>(counters.size()))
                .arg(static_cast<qulonglong>(hooks.size())));
    if (counters.empty()) {
        return false;
    }

    std::vector<std::string> hitSeen;
    const qint64 hitDeadline = QDateTime::currentMSecsSinceEpoch() + cfg.hitTimeoutMs;
    while (QDateTime::currentMSecsSinceEpoch() < hitDeadline && hitSeen.size() < counters.size()) {
        if (!processAlive(process.h)) {
            emitLog(QStringLiteral("[MODEL] 游戏进程已退出"));
            break;
        }
        for (const auto &pair : counters) {
            if (std::find(hitSeen.begin(), hitSeen.end(), pair.first) != hitSeen.end()) {
                continue;
            }
            const auto value = readU64Remote(process.h, pair.second);
            if (value && *value > 0) {
                hitSeen.push_back(pair.first);
            }
        }
        QThread::msleep(20);
    }
    if (!hitSeen.empty()) {
        emitState("model_state", QStringLiteral("登录包已命中"));
    }
    emitLog(QStringLiteral("[MODEL] 本次替换完成，安装 %1/%2，命中 %3")
                .arg(static_cast<qulonglong>(counters.size()))
                .arg(static_cast<qulonglong>(hooks.size()))
                .arg(static_cast<qulonglong>(hitSeen.size())));
    Q_UNUSED(missing);
    return !counters.empty();
}

class ModelRuntime {
public:
    ~ModelRuntime()
    {
        shutdown();
    }

    int start(const PayloadConfig &cfg)
    {
        QMutexLocker locker(&m_mutex);
        if (m_running) {
            return kErrAlreadyRunning;
        }
        if (m_worker.joinable()) {
            m_worker.join();
        }
        m_stopRequested.store(false);
        m_running = true;
        m_worker = std::thread([this, cfg]() {
            run(cfg);
        });
        return kOk;
    }

    int stop()
    {
        m_stopRequested.store(true);
        emitState("model_state", QStringLiteral("停止中"));
        emitLog(QStringLiteral("[MODEL] 已请求停止等待"));
        return kOk;
    }

    int isRunning() const
    {
        QMutexLocker locker(&m_mutex);
        return m_running ? 1 : 0;
    }

    int isWaitingForProcess() const
    {
        return m_waitingForProcess.load() ? 1 : 0;
    }

    void shutdown()
    {
        m_stopRequested.store(true);
        std::thread old;
        {
            QMutexLocker locker(&m_mutex);
            if (m_worker.joinable()) {
                old = std::move(m_worker);
            }
        }
        if (old.joinable()) {
            old.join();
        }
        m_waitingForProcess.store(false);
    }

private:
    void run(PayloadConfig cfg)
    {
        emitState("model_running", QStringLiteral("true"));
        m_waitingForProcess.store(true);
        emitState("model_state", QStringLiteral("正在运行"));
        emitLog(QStringLiteral("[MODEL] 开始等待下次 Minecraft 启动"));

        std::vector<DWORD> baseline = enumTargetPids();
        const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + cfg.processTimeoutMs;
        while (!m_stopRequested.load() && QDateTime::currentMSecsSinceEpoch() < deadline) {
            const std::vector<DWORD> current = enumTargetPids();
            std::vector<DWORD> candidates;
            for (DWORD pid : current) {
                if (std::find(baseline.begin(), baseline.end(), pid) == baseline.end()) {
                    candidates.push_back(pid);
                }
            }
            for (DWORD pid : candidates) {
                if (m_stopRequested.load()) {
                    break;
                }
                if (!probeProcessReady(pid, cfg)) {
                    continue;
                }
                m_waitingForProcess.store(false);
                emitLog(QStringLiteral("[MODEL] 找到可替换的新进程"));
                const bool ok = patchProcess(pid, cfg);
                emitState("model_state", ok ? QStringLiteral("本次替换完成") : QStringLiteral("失败"));
                if (m_stopRequested.load()) {
                    break;
                }
                m_waitingForProcess.store(true);
                emitState("model_state", QStringLiteral("正在运行"));
                baseline = enumTargetPids();
            }
            QThread::msleep(50);
        }
        m_waitingForProcess.store(false);
        if (m_stopRequested.load()) {
            emitState("model_state", QStringLiteral("已停止"));
            emitLog(QStringLiteral("[MODEL] 已停止等待"));
        } else {
            emitState("model_state", QStringLiteral("失败"));
            emitLog(QStringLiteral("[MODEL] 等待 Minecraft 启动超时"));
        }
        emitState("model_running", QStringLiteral("false"));
        QMutexLocker locker(&m_mutex);
        m_running = false;
    }

    mutable QMutex m_mutex;
    std::thread m_worker;
    std::atomic_bool m_stopRequested{false};
    std::atomic_bool m_waitingForProcess{false};
    bool m_running = false;
};

ModelRuntime g_runtime;

} // namespace

extern "C" {

NBMAPI int nbm_get_protocol_version(void)
{
    return kProtocolVersion;
}

NBMAPI int nbm_get_abi_version(void)
{
    return kAbiVersion;
}

NBMAPI int nbm_init(void)
{
    return kOk;
}

NBMAPI void nbm_shutdown(void)
{
    g_runtime.shutdown();
}

NBMAPI void nbm_set_log_callback(NBM_LogCallback cb)
{
    g_logCb = cb;
}

NBMAPI void nbm_set_state_callback(NBM_StateCallback cb)
{
    g_stateCb = cb;
}

NBMAPI int nbm_start_wait(const char *json_config)
{
    QString error;
    const auto cfg = parseConfig(json_config, &error);
    if (!cfg) {
        emitLog(QStringLiteral("[MODEL] 配置无效：%1").arg(error));
        emitState("model_state", QStringLiteral("失败"));
        return kErrInvalidConfig;
    }
    return g_runtime.start(*cfg);
}

NBMAPI int nbm_stop_wait(void)
{
    return g_runtime.stop();
}

NBMAPI int nbm_is_running(void)
{
    return g_runtime.isRunning();
}

NBMAPI int nbm_is_waiting_for_process(void)
{
    return g_runtime.isWaitingForProcess();
}

}
