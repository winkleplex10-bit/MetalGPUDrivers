#!/bin/bash
# Move leftover Kepler NVIDIA kexts out of /Library/Extensions so they cannot
# attach to a modern GTX/RTX card. Does not touch /System/Library/Extensions.
# Prefer OpenCore Kernel > Block (see docs/INSTALL.md); this is for an already
# installed volume.

set -euo pipefail

LE="/Library/Extensions"
STAMP="$(date +%Y%m%d-%H%M%S)"
BACKUP="${BACKUP_DIR:-$HOME/NvidiaMetal40-legacy-nv-$STAMP}"

KEXTS=(
  "NVDAStartup.kext"
  "NVDAResman.kext"
  "NVDAGF100Hal.kext"
  "NVDAGK100Hal.kext"
  "GeForce.kext"
)

echo "Backup directory: $BACKUP"
mkdir -p "$BACKUP"

found=0
for k in "${KEXTS[@]}"; do
  if [ -d "$LE/$k" ]; then
    echo "Moving $LE/$k"
    sudo mv "$LE/$k" "$BACKUP/"
    found=1
  else
    echo "Not present: $LE/$k"
  fi
done

if [ "$found" -eq 0 ]; then
  echo "No leftover NVIDIA kexts in $LE"
  rmdir "$BACKUP" 2>/dev/null || true
  exit 0
fi

echo "Moved kexts to $BACKUP"
if command -v kmutil >/dev/null 2>&1; then
  echo "Rebuilding auxiliary kernel collection (may require a reboot)..."
  sudo kmutil install --volume-root / --update-all || true
fi
echo "Reboot, then check: ioreg -l | grep NvidiaGopFramebuffer40"
echo "Installer/OpenCore still needs Kernel > Block for these bundle IDs."
