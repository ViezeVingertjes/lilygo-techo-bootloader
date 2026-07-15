/* T-Echo sample app for SoftDevice S140 7.x
 *
 * FLASH: app base 0x27000 (after MBR + S140 7.x), up to USER_FLASH_END
 *        (0xEA000 = bootloader at 0xF4000 minus 40 KB the bootloader
 *        reserves for app data / staged bootloader updates - UF2 blocks
 *        at or above 0xEA000 are silently dropped by the bootloader).
 * RAM:   app starts at 0x20006000; everything below is left to the
 *        SoftDevice (default BLE config needs less; sd_ble_enable()
 *        reports the real requirement).
 *
 * Section layout comes from the vendored MDK script (INCLUDE below);
 * pass -L <mdk dir> to the linker so it is found.
 */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x27000, LENGTH = 0xEA000 - 0x27000
  RAM  (rwx) : ORIGIN = 0x20006000, LENGTH = 0x20040000 - 0x20006000
}

/* Single source of truth for the value passed to sd_ble_enable(). */
__app_ram_base = ORIGIN(RAM);

INCLUDE "nrf_common.ld"
