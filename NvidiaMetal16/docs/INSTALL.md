# Installing NvidiaMetal16 (unaccelerated GOP display)

This kext is unsigned. It is for lab / Hackintosh machines where you already inject kexts with OpenCore. It will not load on a stock Apple Silicon Mac, and it will not load on a sealed Intel Mac with SIP fully on.

## 1. Stop leftover Kepler NVIDIA kexts

`NVDAStartup.kext` matches **every** NVIDIA VGA device (`IOPCIMatch 0x000010de`, class `0x03`, probe score 100000). On this machine it lives in `/Library/Extensions` together with `NVDAResman`, `NVDAGF100Hal`, `NVDAGK100Hal`, and `GeForce.kext`. Those binaries are Kepler-era. If they attach to a GTX 900 / 10-series / RTX card, boot often dies.

**Preferred (OpenCore):** `Kernel > Block` the bundle IDs below. That works in the installer too.

| Bundle ID | Kext |
|---|---|
| `com.apple.nvidia.NVDAStartup` | NVDAStartup.kext |
| `com.apple.nvidia.driver.NVDAResman` | NVDAResman.kext |
| `com.apple.GeForce` | GeForce.kext |
| `com.apple.nvidia.driver.NVDAGF100Hal` | NVDAGF100Hal.kext |
| `com.apple.nvidia.driver.NVDAGK100Hal` | NVDAGK100Hal.kext |

Example `config.plist` fragment:

```xml
<key>Block</key>
<array>
	<dict>
		<key>Arch</key>
		<string>x86_64</string>
		<key>BundlePath</key>
		<string>NVDAStartup.kext</string>
		<key>Enabled</key>
		<true/>
		<key>Identifier</key>
		<string>com.apple.nvidia.NVDAStartup</string>
		<key>Strategy</key>
		<string>Exclude</string>
	</dict>
	<dict>
		<key>Arch</key>
		<string>x86_64</string>
		<key>Enabled</key>
		<true/>
		<key>Identifier</key>
		<string>com.apple.nvidia.driver.NVDAResman</string>
		<key>Strategy</key>
		<string>Exclude</string>
	</dict>
	<dict>
		<key>Arch</key>
		<string>x86_64</string>
		<key>Enabled</key>
		<true/>
		<key>Identifier</key>
		<string>com.apple.GeForce</string>
		<key>Strategy</key>
		<string>Exclude</string>
	</dict>
	<dict>
		<key>Arch</key>
		<string>x86_64</string>
		<key>Enabled</key>
		<true/>
		<key>Identifier</key>
		<string>com.apple.nvidia.driver.NVDAGF100Hal</string>
		<key>Strategy</key>
		<string>Exclude</string>
	</dict>
	<dict>
		<key>Arch</key>
		<string>x86_64</string>
		<key>Enabled</key>
		<true/>
		<key>Identifier</key>
		<string>com.apple.nvidia.driver.NVDAGK100Hal</string>
		<key>Strategy</key>
		<string>Exclude</string>
	</dict>
</array>
```

**Optional on an installed volume:** `sudo tools/disable-legacy-nv.sh` moves those kexts out of `/Library/Extensions` (does not touch SLE). OpenCore Block is still required for the installer.

This kext also uses probe score **500000** vs NVDAStartup's **100000**, so it should win even if Block is incomplete. Do not rely on that alone.

## 2. Inject NvidiaMetal16.kext

Copy `build/NvidiaMetal16.kext` to `EFI/OC/Kexts/` and add:

```xml
<dict>
	<key>Arch</key>
	<string>x86_64</string>
	<key>BundlePath</key>
	<string>NvidiaMetal16.kext</string>
	<key>Enabled</key>
	<true/>
	<key>ExecutablePath</key>
	<string>Contents/MacOS/NvidiaMetal16</string>
	<key>PlistPath</key>
	<string>Contents/Info.plist</string>
	<key>MinKernel</key>
	<string></string>
	<key>MaxKernel</key>
	<string></string>
</dict>
```

Put it **after** Lilu (if present) and **before** WhateverGreen is fine. It does not depend on Lilu.

`IOGraphicsFamily` is already in the kernel collection; do not Force-inject it unless OpenCore docs say you must (same note as MacHyperVFramebuffer).

## 3. Firmware GOP

The card must post a GOP framebuffer (UEFI). OpenCore:

- `UEFI > Output > ProvideConsoleGop` = true
- For cards without GOP, OpenCore `EnableGop` / a GOP firmware injection is a separate firmware problem; this kext cannot invent scan-out.

If the NVIDIA card is the primary output, the Apple logo should already be on that screen before the kext loads. The kext then hands that same buffer to WindowServer.

## 4. AppleGraphicsDevicePolicy (black screen after login)

Unsupported board-ids often black-screen even when the framebuffer is valid. Use WhateverGreen:

```
boot-args = agdpmod=pikera
```

Common on iMacPro1,1 / MacPro7,1 SMBIOS. This kext sets `AAPL,boot-display` when it owns GOP; it does not patch AGDP itself.

## 5. SIP / signing

OpenCore `csr-active-config` must allow untrusted kexts (typical Hackintosh values already do). On a real Mac you would need SIP off and an AuxKC; that is not a supported install path for this milestone.

## 6. Confirm it attached

Verbose boot (`-v`) or:

```bash
log show --last boot --predicate 'eventMessage CONTAINS "NvidiaMetal16"'
ioreg -l -w0 | grep -A20 NvidiaGopFramebuffer16
```

You want `NvidiaGopFramebuffer16` under the NVIDIA `IOPCIDevice`, **no** `NVDAStartup` / `NVDA` children, and `AAPL,boot-display` if this card is the console.

`tools/dump-nv-ioreg.sh` prints the NVIDIA PCI subtree.

## 7. Dual GPU (iGPU + NVIDIA)

If GOP is on the iGPU, this kext still claims NVIDIA and returns `enableController = unsupported`. That is intentional: boot should proceed on the iGPU. To force a GOP that is not inside a BAR (rare OpenCore bounce buffer), add `-nvfbforce`.

## 8. What will look wrong (expected)

- One resolution (whatever GOP programmed)
- No Metal / no hardware cursor / no rotation
- Slow WindowServer compositing
- No HDMI audio from the NVIDIA function
- Sleep/wake may blank the panel (GOP is not reprogrammed)

That is the unaccelerated milestone. GSP + NVKMS come next.
