# LilyGo T-Echo: bootloader 0.11.0 + SoftDevice S140 7.3.0

The T-Echo ships with the Adafruit UF2 bootloader **0.6.1 bundled with S140 6.1.1**, a
2018 SoftDevice. This repo holds what you need to run it on **bootloader 0.11.0 with S140
7.3.0** instead, with built artifacts published as release assets.

It isn't a fork. The board definition is four small files that drop into a stock
`Adafruit_nRF52_Bootloader` checkout. [FLASHING.md](FLASHING.md) covers the flashing
procedure, the recovery paths, and what the upgrade does and doesn't buy you.

## Why this isn't upstream

Upstream vendors S140 7.3.0 already (`SD_VERSION=7.3.0`, used by the Seeed XIAO and
T1000-E boards) but has **no T-Echo board definition**. The only one in the wild lives in
[lyusupov's SoftRF fork](https://github.com/lyusupov/Adafruit_nRF52_Bootloader) and still
builds against 6.1.1. This port is that board definition, updated for upstream 0.11.0's
API and pinned to 7.3.0.

## Contents

| Path | What |
|---|---|
| `src/boards/lilygo_techo/` | The board port. Copy into an upstream checkout at the same path. |
| `sample-app/` | Proof-of-life app linked at 0x27000: enables SD 7.3.0, verifies FWID 0x0123, BLE-advertises as `TEcho-SD730` |
| `tools/verify_artifacts.py` | Pre-flight check of a build's memory layout and DFU manifest |
| `.github/workflows/build.yml` | Builds against pinned upstream 0.11.0, verifies, attaches artifacts to tagged releases |

Binaries are not committed. Grab them from the
[latest release](https://github.com/ViezeVingertjes/lilygo-techo-bootloader/releases/latest)
or build them below. Both come out of the same CI-verified path.

## Building

```bash
git clone --recurse-submodules https://github.com/adafruit/Adafruit_nRF52_Bootloader
cp -r src/boards/lilygo_techo Adafruit_nRF52_Bootloader/src/boards/
cd Adafruit_nRF52_Bootloader
make PYTHON=python3 BOARD=lilygo_techo all
```

Needs `arm-none-eabi-gcc`, `adafruit-nrfutil`, and Python `intelhex`. `PYTHON=python3` is
required on distros without a `python` alias, since the Makefile defaults to `python`.

Output lands in `_build/build-lilygo_techo/`. Verify it before flashing:

```bash
python3 tools/verify_artifacts.py Adafruit_nRF52_Bootloader/_build/build-lilygo_techo \
  --sd-hex Adafruit_nRF52_Bootloader/lib/softdevice/s140_nrf52_7.3.0/s140_nrf52_7.3.0_softdevice.hex
```

You can also add a row to upstream's `supported_boards.md`. It's cosmetic, it just keeps
the board index complete:

```
| lilygo_techo | LilyGo T-Echo | 0x239A:0x0029 | https://lilygo.cc/products/t-echo |
```

The `sample-app/` Makefile expects the bootloader checkout as a sibling directory. Override
with `make BL_ROOT=/path/to/Adafruit_nRF52_Bootloader`.

## Port notes

- **`SD_VERSION = 7.3.0`** in `board.mk` is the whole SoftDevice switch. Upstream vendors
  the hex and headers already.
- **Power rail** (`LED_PWR_ON`/`LED_PWR_EN`) is set up in the board's own `pinconfig.c` via
  upstream's weak `board_init2()` hook, so no shared upstream file is touched and the port
  rebases cleanly.
- **USB VID/PID `0x239A:0x0029`** is shared with the Feather nRF52840 Express. That
  collides with upstream's own guidance for new boards, but it's kept deliberately: it
  matches the factory T-Echo bootloader's identity, so existing tooling and
  bootloader-update verification keep working on devices already in the field. The
  consequence is documented in `board.h`. Never drag a Feather bootloader UF2 onto a
  T-Echo.
- **Board revision**: pins target the 2021-03-26 revision (red LED on P1.03). The early
  2020-12-12 revision (red on P0.13) has its alternates commented in `board.h`. Buttons and
  USB are identical across both, so only LED feedback differs.

## Licensing

The port is MIT, inherited from Adafruit's bootloader and Linar Yusupov's T-Echo board
definition; see [LICENSE](LICENSE) and the source headers.

Release assets contain Nordic's S140 SoftDevice binary, which is not MIT. It ships under
Nordic's own 5-clause license, included in every release as
`s140_nrf52_7.3.0_license-agreement.txt`: binary redistribution is permitted with the
notice reproduced, and use is restricted to Nordic silicon. Nordic ships 7.3.0 from
<https://www.nordicsemi.com/Products/Development-software/S140/Download>.
