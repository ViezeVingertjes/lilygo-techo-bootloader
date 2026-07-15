# Flashing SoftDevice S140 7.3.0 onto the LilyGo T-Echo

Download the artifacts from the
[latest release](https://github.com/ViezeVingertjes/lilygo-techo-bootloader/releases/latest),
or build them yourself with the steps in [README.md](README.md). CI builds them from
upstream 0.11.0 plus this board port and runs `tools/verify_artifacts.py` over the result
before anything is published.

**Start with the `.zip`, not the `.uf2`.** The UF2 is the familiar drag-drop format, but
the one in this release carries the bootloader alone and cannot touch the SoftDevice, so
it can't perform this upgrade. Serial DFU with the zip is the only way in without a
debugger.

| File | What it is |
|---|---|
| `lilygo_techo_bootloader-0.11.0_s140_7.3.0.zip` | **The upgrade package. This is the one you want.** Bootloader 0.11.0 + S140 7.3.0, applied over serial DFU in one transaction. |
| `lilygo_techo_bootloader-0.11.0_s140_7.3.0.hex` | Merged MBR + SD 7.3.0 + bootloader image for SWD flashing (J-Link/pyocd). Recovery or factory-style flash. |
| `update-lilygo_techo_bootloader-0.11.0_nosd.uf2` | Bootloader-only update via UF2 drag-drop. **Does NOT change the SoftDevice.** Only useful once you are already on 7.3.0 and want a newer bootloader. |
| `techo_sd730_sample.uf2` | Proof-of-life app linked at 0x27000: enables SD 7.3.0, verifies FWID 0x0123, advertises as `TEcho-SD730`. |
| `s140_nrf52_7.3.0_license-agreement.txt` | Nordic's license for the SoftDevice binary inside the zip and hex. |

## Pre-flight checks

1. Double-press the reset button. The `TECHOBOOT` USB drive appears.
2. Open `INFO_UF2.TXT` on the drive. Expect something like:
   `Bootloader: s140 6.1.1`, `Board-ID: nRF52840-TEcho-v1`.
   That confirms the current SoftDevice (6.1.1) and that the board identity matches this build.
3. Note your hardware revision. This build targets the **2021-03-26 revision** (red LED on P1.03). On the rare early 2020-12-12 revision the bootloader works identically (same buttons P1.10/P0.11, same USB), but LED feedback will be wrong or dark. Don't mistake that for a brick.

## Step 1: flash bootloader + SoftDevice (serial DFU)

With the device connected over USB, in normal mode or bootloader mode (`--touch 1200` triggers DFU entry):

```bash
pip install --user --break-system-packages adafruit-nrfutil   # if not installed
adafruit-nrfutil --verbose dfu serial \
  --package lilygo_techo_bootloader-0.11.0_s140_7.3.0.zip \
  -p /dev/ttyACM0 -b 115200 --singlebank --touch 1200
```

This replaces bootloader **and** SoftDevice in one transaction. The package's init data uses `sd-req 0xFFFE` (any), which is what lets a device on 6.1.1 accept it. The MBR (2.4.1) is untouched, since both SD versions bundle the identical MBR.

**After this step the device has no valid application.** The old app was linked at 0x26000, which is now inside the SoftDevice. It will sit in the UF2 bootloader. That's expected, not a brick.

## Step 2: flash the sample app (UF2 drag-drop)

Double-press reset, then copy `techo_sd730_sample.uf2` onto `TECHOBOOT`.

Expected result:
- **Blue LED blinking at 1 Hz** means S140 7.3.0 is enabled, its firmware ID verified as 0x0123, and BLE advertising is running.
- Scan with any BLE app (nRF Connect on a phone, say). A non-connectable advertiser named **`TEcho-SD730`** appears.
- Red fast blink means an sd_* call failed. Red solid means a SoftDevice fault. Neither is a brick, double-press reset still enters the bootloader.

Also check `INFO_UF2.TXT` now reads `s140 7.3.0` and `Bootloader: 0.11.0`.

The sample busy-polls for its blink timing, so the CPU never sleeps (~3-5 mA). Fine as a proof of life, but don't leave it running on battery for weeks.

## Recovery paths, in order of escalation

1. **The bootloader always survives app flashing.** Double-press reset, get `TECHOBOOT`, drop a different UF2.
2. **Downgrade to factory state.** Serial-DFU LilyGo's stock package back: the `lilygo_techo_bootloader-0.6.1_s140_6.1.1` zip from https://github.com/Xinyuan-LilyGO/T-Echo (bootloader/), same `adafruit-nrfutil dfu serial` command. Your old 6.1.1-based firmware then works again.
3. **Wrong-SD app flashed.** An old 0x26000-linked UF2 dragged on after the upgrade overwrites the tail of the SoftDevice. Re-run Step 1; the DFU package rewrites the whole SD and bootloader.
4. **SWD**, only if the bootloader itself is destroyed, which normal UF2 or app flashing cannot do. Wire a J-Link/DAPLink to the SWD pads and flash the merged `_s140_7.3.0.hex` (or LilyGo's factory hex) with `pyocd flash -t nrf52840 <hex>` or J-Flash.

Rule of thumb after the upgrade: **only flash firmware built for SD 7.x at base 0x27000.** DFU packages for app updates must be generated with `--sd-req 0x0123` (S140 7.3.0), not `0x00B6` (6.1.1). Never flash Feather nRF52840 bootloader-update UF2s. The T-Echo shares the Feather's USB VID/PID (a factory decision, kept for compatibility), so the board-ID check will not protect you there.

## What changed between S140 6.1.1 and 7.3.0

Reference, so you can judge against your own firmware. Whether any of it matters depends
entirely on what you run.

- 7.0.1 is the first Core 5.1 qualified release (6.1.1 is Core 5.0, from 2018); 7.3.0's
  notes reference conformance to Core 5.2.
- Bug fixes across 7.0 to 7.3, including extended advertising no longer timing out early
  (DRGN-10367), a missing `BLE_GAP_EVT_DISCONNECTED` when events are not pulled
  continuously (DRGN-15619), and assert and hang fixes in the central role and
  channel-survey paths.
- Improved advertiser scheduling is on by default. In 6.1.1 it was opt-in via
  `BLE_COMMON_OPT_ADV_SCHED_CFG`, which 7.x removes. That is the only API removal, and
  it is the one thing that can break a source build.
- Added APIs: connection-event PPI triggers (`sd_ble_gap_conn_evt_trigger_start`),
  `sd_ble_gap_next_conn_evt_counter_get`, efficient 128-bit UUID discovery
  (`BLE_GATTC_OPT_UUID_DISC`, 7.2.0), CAR/PPCP characteristic inclusion config, and
  `BLE_GAP_SLAVE_LATENCY_WAIT_FOR_ACK` (7.3.0).
- Unchanged: 2M PHY, Coded PHY/Long Range, extended advertising and LE Secure
  Connections all exist in 6.1.1 already. The SoftDevice handles BLE only, so nothing
  here touches LoRa.
- Costs: the application base moves 0x26000 to 0x27000 (4 KB less app flash) and the
  minimum SoftDevice RAM floor rises 0x50 bytes. The MBR is 2.4.1 in both, so there is
  nothing to migrate there.

## Rebuilding your firmware for the new memory map

Whatever you run has to be rebuilt against the 7.x headers and linked at 0x27000, and any
OTA/DFU packages you generate need `--sd-req 0x0123`. Two common cases, both verified
against the projects' current sources on 2026-07-15:

**MeshCore** (github.com/meshcore-dev/MeshCore) already vendors both pieces for its other
boards: `lib/nrf52/s140_nrf52_7.3.0_API/` headers and `boards/nrf52840_s140_v7.ld`. Its
`lilygo_techo` variant just points at the 6.1.1 ones, so it is a config change in
`variants/lilygo_techo/platformio.ini`:

- `board_build.ldscript = boards/nrf52840_s140_v7.ld` (was `..._v6.ld`)
- swap the two include lines from `lib/nrf52/s140_nrf52_6.1.1_API/include{,/nrf52}` to
  `lib/nrf52/s140_nrf52_7.3.0_API/include{,/nrf52}`

**Meshtastic** (github.com/meshtastic/firmware) needs one extra step, so it is not quite
the same procedure. Its BSP fork (`meshtastic/Adafruit_nRF52_Arduino#cpp17-platform`) does
bundle the `s140_nrf52_7.3.0_API` headers, but its linker directory has no
`nrf52840_s140_v7.ld`: only a v6 for this chip, plus a v7 for the nRF52833. You have to
supply the script yourself. Copy `nrf52840_s140_v6.ld`, change `ORIGIN` from 0x26000 to
0x27000, and keep the region below the bootloader's reserved area. Seeed's fork of the same
BSP carries a working one if you would rather not write it. Then in `boards/t-echo.json`:

- `build.arduino.ldscript` from `nrf52840_s140_v6.ld` to your v7 script
- `build.softdevice.sd_version` from `6.1.1` to `7.3.0`
- `build.softdevice.sd_fwid` from `0x00B6` to `0x0123`

Both are unaffected by the API removal above unless they set `BLE_COMMON_OPT_ADV_SCHED_CFG`,
which application code almost never does. Everything else is source-compatible.

## Rebuilding from source

See [README.md](README.md). It covers the upstream checkout, the build invocation, and the
pre-flight verification step.
