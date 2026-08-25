#!/bin/bash
# Dump NVIDIA PCI devices and any framebuffer / NVDA children.

set -euo pipefail

echo "===== IOPCIDevice vendor 10de ====="
ioreg -r -c IOPCIDevice -l -w0 2>/dev/null | awk '
  /IOPCIDevice/ { p=1 }
  p && /"vendor-id"/ { v=$0 }
  p && /"device-id"/ { d=$0 }
  p && /"class-code"/ { c=$0 }
  p && /"model"/ { m=$0 }
  p && /NVDAType/ { n=$0 }
  p && /AAPL,boot-display/ { b=$0 }
  p && /NvidiaGopFramebuffer30|NVDAStartup|NVDAResman|nvAccelerator|IOClass/ { print }
  /^[ |]*\+\-\o IOPCIDevice/ {
    if (v != "") print v, d, c, m, n, b
  }
'

echo
echo "===== NvidiaGopFramebuffer30 ====="
ioreg -r -c NvidiaGopFramebuffer30 -l -w0 2>/dev/null || echo "(not loaded)"

echo
echo "===== NVDAStartup / NVDA / nvAccelerator ====="
ioreg -l -w0 2>/dev/null | rg -n 'NVDAStartup|NVDAResman|"NVDA"|nvAccelerator|NvidiaGopFramebuffer30' || true

echo
echo "===== kernel log (this boot) ====="
log show --last boot --predicate 'eventMessage CONTAINS "NvidiaMetal30"' --style compact 2>/dev/null | tail -50 || true
