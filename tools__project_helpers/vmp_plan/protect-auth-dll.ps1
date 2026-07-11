param()

$ErrorActionPreference = 'Stop'

throw @'
Automated VMProtect compilation is intentionally disabled.

Use prepare-vmp-gui.ps1 after a normal build. It regenerates the MAP-derived
Lua selectors and validates the four empty GUI projects. Then open VMProtect
yourself, paste the printed dofile(...) line into the matching project Script
box, and click Compile in the GUI.
'@
