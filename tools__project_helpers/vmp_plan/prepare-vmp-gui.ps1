param()

$ErrorActionPreference = 'Stop'

throw @'
VMP GUI automation is disabled.

The previous Lua callback path let VMProtect persist generated Procedure entries
into a GUI project and produced a project that VMP could no longer read. Do not
load generated selectors into a .vmp Script field and do not compile from this
helper until the workflow has been isolated and validated against this VMP build.
'@
