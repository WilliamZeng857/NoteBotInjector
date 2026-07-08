from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
BACKEND = ROOT / "src__injector_exe" / "backend.cpp"
MODEL_API = ROOT / "src__model_dll" / "src" / "model_api.cpp"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")


def require(condition: bool, message: str) -> None:
    if not condition:
        print(f"FAIL: {message}")
        sys.exit(1)


backend = read_text(BACKEND)
model_api = read_text(MODEL_API)

runtime_match = re.search(
    r"bool Backend::modelRuntimeRequested\(\) const\s*\{(?P<body>.*?)\n\}",
    backend,
    re.S,
)
require(runtime_match is not None, "modelRuntimeRequested function not found")
runtime_body = runtime_match.group("body")
require(
    "m_modelArmOverrideEnabled" not in runtime_body,
    "legacy Arm Lock must not start the model runtime independently",
)
require(
    "return m_modelModificationEnabled;" in runtime_body,
    "model runtime must be controlled only by modelModificationEnabled",
)

require(
    "m_classicModeEnabled && !m_skinPngPath.isEmpty()" not in backend,
    "classic mode must not fall back to server model when PNG is missing",
)
require(
    "Arm Lock" not in backend,
    "standalone Arm Lock mode must not remain user-visible",
)
require(
    'cfg[QStringLiteral("arm_override_enabled")] = m_modelModificationEnabled && m_classicModeEnabled;' in backend,
    "arm override must only be enabled for classic square-skin replacement",
)

require(
    "include = cfg.armOverrideEnabled && !cfg.armSize.isEmpty();" in model_api,
    "ArmSize hooks must depend on armOverrideEnabled, not modelEnabled",
)

print("model route logic checks passed")
