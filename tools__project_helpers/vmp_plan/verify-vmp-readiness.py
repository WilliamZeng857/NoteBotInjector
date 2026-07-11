"""Verify source, protection-table, and linker-MAP readiness for VMProtect."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PLAN_DIR = ROOT / "tools__project_helpers" / "vmp_plan"

TARGETS = {
    "Injector": {
        "table": PLAN_DIR / "NoteBotInjector.protect.tsv",
        "map": ROOT / "build__injector_exe_cache" / "NoteBotInjector.map",
        "project": ROOT / "build__injector_exe_cache" / "NoteBotInjector.exe.vmp",
        "sources": [ROOT / "src__injector_exe" / "backend.cpp",
                    ROOT / "src__injector_exe" / "updater.cpp",
                    ROOT / "src__injector_exe" / "win32injector.cpp"],
    },
    "Updater": {
        "table": PLAN_DIR / "NoteBotUpdater.protect.tsv",
        "map": ROOT / "build__injector_exe_cache" / "NoteBotUpdater.map",
        "project": ROOT / "build__injector_exe_cache" / "NoteBotUpdater.exe.vmp",
        "sources": [ROOT / "src__updater_exe" / "updater_main.cpp"],
    },
    "Auth": {
        "table": PLAN_DIR / "NoteBotAuth.protect.tsv",
        "map": ROOT / "build__auth_dll_cache" / "NoteBotAuth.map",
        "project": ROOT / "build__auth_dll_cache" / "NoteBotAuth.dll.vmp",
        "sources": list((ROOT / "src__auth_dll").rglob("*.cpp")),
    },
    "Model": {
        "table": PLAN_DIR / "NoteBotModel.protect.tsv",
        "map": ROOT / "build__injector_exe_cache" / "NoteBotModel.map",
        "project": ROOT / "dist__model_runtime_artifacts" / "NoteBotModel.dll.vmp",
        "sources": [ROOT / "src__model_dll" / "src" / "model_api.cpp"],
    },
}

MAP_REQUIRED = (
    "/MAP:NoteBotInjector.map",
    "/MAP:NoteBotUpdater.map",
    "/MAP:NoteBotModel.map",
)
CORE_REQUIRED = {
    "Injector": {
        "NBVmp_Injector_AuthAbiMatches",
        "NBVmp_Injector_ModelAbiMatches",
        "NBVmp_Injector_ProtectedLicenseEnvelopeReady",
        "NBVmp_Injector_ManifestIdentityReady",
        "NBVmp_Injector_SignManifestPayload",
    },
    "Updater": {
        "NBVmp_Updater_IsReplaceActionSupported",
        "NBVmp_Updater_RestartAllowedForAction",
    },
    "Auth": {
        "NBVmp_Verify_SignDevicePayload",
        "NBVmp_Verify_TicketResponseReady",
        "NBVmp_Verify_WrapperKeyReady",
        "NBVmp_Verify_InjectResultEnvelopeMatches",
    },
    "Model": {
        "NBVmp_Model_RuntimeModeRequested",
        "NBVmp_Model_ModelAssetsConfigured",
        "NBVmp_Model_SkinDimensionsAllowed",
        "NBVmp_Model_UsablePayloadReady",
    },
}
FORBIDDEN_HOT_PATH_MARKERS = (
    "NBVmp_Model_PatchProcess",
    "NBVmp_Model_InstallHook",
    "NBVmp_Injector_DownloadArtifact",
    "NBVmp_Injector_DoInject",
    "NBVmp_Updater_ReplaceFile",
)
ROW_RE = re.compile(r"^(mutation|virtualize|ultra|super)\t(NBVmp_[^\t]+)\t[^\t]+\t.+$")
MAP_SYMBOL_RE = re.compile(r"^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\?NBVmp_\S*)\s+[0-9A-Fa-f]{16}\s+(?:f\s+)?\S+\.obj\s*$")


def read_table(path: Path) -> list[str]:
    symbols: list[str] = []
    for line_no, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith("category\t"):
            continue
        match = ROW_RE.fullmatch(line)
        if not match:
            raise ValueError(f"{path}:{line_no}: invalid protection row")
        symbols.append(match.group(2))
    if not symbols:
        raise ValueError(f"{path}: no protection rows")
    if len(symbols) != len(set(symbols)):
        raise ValueError(f"{path}: duplicate symbol_key")
    return symbols


def verify_empty_project(path: Path, target: str) -> list[str]:
    if not path.exists():
        return [f"{target}: missing GUI-owned project {path}"]
    project = path.read_text(encoding="utf-8")
    errors: list[str] = []
    if "<Document" not in project or "<Protection" not in project:
        errors.append(f"{target}: invalid VMP project XML")
    if "<Procedure" in project or "MapAddress=" in project:
        errors.append(f"{target}: VMP project must remain empty; Procedure entries are forbidden")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-maps", action="store_true")
    args = parser.parse_args()
    errors: list[str] = []

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8-sig")
    for flag in MAP_REQUIRED:
        if flag not in cmake:
            errors.append(f"CMakeLists.txt missing {flag}")

    for target, config in TARGETS.items():
        table = config["table"]
        if not table.exists():
            errors.append(f"{target}: missing table {table.name}")
            continue
        try:
            expected = read_table(table)
        except ValueError as exc:
            errors.append(str(exc))
            continue

        errors.extend(verify_empty_project(config["project"], target))

        source_text = "\n".join(path.read_text(encoding="utf-8-sig")
                                for path in config["sources"] if path.exists())
        for symbol in CORE_REQUIRED[target]:
            if symbol not in source_text:
                errors.append(f"{target}: core protection marker absent from source: {symbol}")
        for symbol in expected:
            if symbol not in source_text:
                errors.append(f"{target}: table symbol absent from source: {symbol}")

        if args.skip_maps:
            continue
        map_path = config["map"]
        if not map_path.exists():
            errors.append(f"{target}: missing map {map_path}")
            continue
        map_symbols = []
        for line in map_path.read_text(encoding="utf-8", errors="replace").splitlines():
            match = MAP_SYMBOL_RE.search(line)
            if match:
                map_symbols.append(match.group(1))
        for symbol in expected:
            matches = [name for name in map_symbols if symbol in name]
            if len(matches) != 1:
                errors.append(f"{target}: {symbol} resolved {len(matches)} times in map")
        for symbol in map_symbols:
            if not any(expected_symbol in symbol for expected_symbol in expected):
                errors.append(f"{target}: unlisted protected symbol in map: {symbol}")

    all_source_text = "\n".join(
        path.read_text(encoding="utf-8-sig")
        for config in TARGETS.values()
        for path in config["sources"]
        if path.exists()
    )
    for symbol in FORBIDDEN_HOT_PATH_MARKERS:
        if symbol in all_source_text:
            errors.append(f"hot-path function must remain unprotected: {symbol}")

    if errors:
        print("VMP readiness verification failed:")
        for error in errors:
            print(f"  - {error}")
        return 1
    print("VMP readiness verification passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
