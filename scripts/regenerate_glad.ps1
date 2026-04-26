# Regenerate GLAD bindings for OpenGL 4.5 core into external/glad
# Requires: pip install glad

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

python -m pip install --quiet glad
glad --api gl=4.5 --profile core --generator c --out-path external/glad

Write-Host "GLAD regenerated under external/glad"
