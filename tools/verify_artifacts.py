#!/usr/bin/env python3
"""Verify a built lilygo_techo + S140 7.3.0 bootloader before it touches hardware.

Checks the merged hex's memory layout (MBR, SoftDevice extent, firmware ID, bootloader
placement, UICR) and the DFU package's manifest. A wrong SoftDevice or a bootloader at
the wrong address bricks the device in a way only SWD recovers, so this runs before
flashing rather than after.

Usage: tools/verify_artifacts.py <build-dir> [--sd-hex <s140_nrf52_7.3.0_softdevice.hex>]

  <build-dir>  directory holding *_s140_7.3.0.{hex,zip}. After a build that is
               Adafruit_nRF52_Bootloader/_build/build-lilygo_techo, or a
               directory of downloaded release assets
  --sd-hex     optional: the vendored SoftDevice hex, to byte-compare the merged
               image's SoftDevice region against it
"""
import argparse
import glob
import json
import struct
import sys
import zipfile

from intelhex import IntelHex

APP_BASE = 0x27000  # S140 7.x application base (6.1.1 was 0x26000)
BOOTLOADER_ADDR = 0xF4000
MBR_PARAMS_PAGE = 0xFE000
S140_730_FWID = 0x0123

ok = True


def check(name, cond, detail=""):
    global ok
    print(f"{'PASS' if cond else 'FAIL'}  {name}  {detail}")
    if not cond:
        ok = False


def one(build_dir, pattern):
    hits = glob.glob(f"{build_dir}/{pattern}")
    if len(hits) != 1:
        sys.exit(f"expected exactly one {pattern} in {build_dir}, found {len(hits)}")
    return hits[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("build_dir")
    ap.add_argument("--sd-hex")
    args = ap.parse_args()

    merged = one(args.build_dir, "*_s140_7.3.0.hex")
    package = one(args.build_dir, "*_s140_7.3.0.zip")
    print(f"merged hex: {merged}\nDFU package: {package}\n")

    ih = IntelHex(merged)
    segs = ih.segments()
    print("Merged hex segments:")
    for s, e in segs:
        print(f"  0x{s:08X} - 0x{e:08X}  ({e - s} bytes)")

    sp = struct.unpack("<I", ih.gets(0x0, 4))[0]
    rv = struct.unpack("<I", ih.gets(0x4, 4))[0]
    check("MBR vector table at 0x0", 0x20000000 <= sp <= 0x20040000 and rv < 0x1000,
          f"SP=0x{sp:08X} Reset=0x{rv:08X}")

    # SoftDevice info struct: size at 0x3008, firmware ID at 0x300C.
    sd_size = struct.unpack("<I", ih.gets(0x3008, 4))[0]
    fwid = struct.unpack("<H", ih.gets(0x300C, 2))[0]
    check("SD firmware ID == 0x0123 (S140 7.3.0)", fwid == S140_730_FWID, f"FWID=0x{fwid:04X}")
    check("SD size <= app base", sd_size <= APP_BASE, f"sd_size=0x{sd_size:06X}")

    sd_end = max(e for s, e in segs if s < APP_BASE and s < 0x10000000)
    check("SD/MBR content ends below app base", sd_end <= APP_BASE, f"end=0x{sd_end:06X}")

    app_segs = [(s, e) for s, e in segs if APP_BASE <= s and e <= BOOTLOADER_ADDR]
    check("no content in the application region", not app_segs, str(app_segs))

    bl_segs = [(s, e) for s, e in segs if BOOTLOADER_ADDR <= s < 0x100000]
    check("bootloader present at 0xF4000", any(s == BOOTLOADER_ADDR for s, e in bl_segs),
          str([(hex(s), hex(e)) for s, e in bl_segs]))

    bsp = struct.unpack("<I", ih.gets(BOOTLOADER_ADDR, 4))[0]
    brv = struct.unpack("<I", ih.gets(BOOTLOADER_ADDR + 4, 4))[0]
    check("bootloader vector table at 0xF4000",
          0x20000000 <= bsp <= 0x20040000 and BOOTLOADER_ADDR <= brv < MBR_PARAMS_PAGE,
          f"SP=0x{bsp:08X} Reset=0x{brv:08X}")

    uicr_bl = struct.unpack("<I", ih.gets(0x10001014, 4))[0]
    uicr_mbr = struct.unpack("<I", ih.gets(0x10001018, 4))[0]
    check("UICR.NRFFW[0] (bootloader address)", uicr_bl == BOOTLOADER_ADDR, f"0x{uicr_bl:08X}")
    check("UICR.NRFFW[1] (MBR params page)", uicr_mbr == MBR_PARAMS_PAGE, f"0x{uicr_mbr:08X}")
    print("INFO  UICR.REGOUT0 is not in the hex: board_init() programs it at runtime "
          "from UICR_REGOUT0_VALUE (3.3V for this VDDH-supplied board)")

    if args.sd_hex:
        sd = IntelHex(args.sd_hex)
        mismatch = 0
        for s, e in sd.segments():
            if s < 0x1000:
                continue  # the MBR region comes from the bootloader's own mbr hex
            if any(ih[a] != sd[a] for a in range(s, e)):
                mismatch += 1
        check("merged SD region byte-identical to the vendored SoftDevice hex", mismatch == 0,
              f"{mismatch} mismatching segments")

    z = zipfile.ZipFile(package)
    print("\nDFU package contents:", z.namelist())
    m = json.loads(z.read("manifest.json"))
    sb = m["manifest"].get("softdevice_bootloader")
    check("package updates SoftDevice + bootloader together", sb is not None)
    if sb:
        ip = sb["init_packet_data"]
        check("device type == 0x0052", ip["device_type"] == 0x52, str(ip["device_type"]))
        check("device revision == 52840", ip["device_revision"] == 52840, str(ip["device_revision"]))
        # 0xFFFE = DFU_SOFTDEVICE_ANY: what lets a device on 6.1.1 accept this 7.3.0 package.
        check("sd-req == [0xFFFE] (any installed SoftDevice)", ip["softdevice_req"] == [0xFFFE],
              str(ip["softdevice_req"]))
        check("manifest sd_size matches S140 7.3.0", 0x25000 < sb["sd_size"] <= 0x26000,
              str(sb["sd_size"]))
        print(f"INFO  bl_size={sb['bl_size']} sd_size={sb['sd_size']}")

    print("\n" + ("ALL CHECKS PASSED" if ok else "!!! FAILURES PRESENT - DO NOT FLASH !!!"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
