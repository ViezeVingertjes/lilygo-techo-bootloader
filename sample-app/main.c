/*
 * T-Echo SoftDevice S140 7.3.0 proof-of-life sample.
 *
 * Linked at 0x27000 (S140 7.x application base) and started by the UF2
 * bootloader, which routes exceptions to this app through the MBR ->
 * SoftDevice forwarding chain (sd_softdevice_vector_table_base_set).
 * VTOR must stay 0 for that to work - the vendored MDK startup
 * (gcc_startup_nrf52840.S + SystemInit) never touches it.
 *
 * The app enables the SoftDevice, checks the running firmware ID, and
 * starts non-connectable BLE advertising as "TEcho-SD730".
 *
 * LED status (LEDs are active low):
 *   blue slow blink (1 Hz)  - SD 7.3.0 enabled, version verified, advertising
 *   red fast blink  (5 Hz)  - an sd_* call failed
 *   red solid               - SoftDevice fault / HardFault
 *
 * Demo only: blink timing busy-polls SysTick, so the CPU never sleeps.
 * Fine for a proof of life, not for running on battery long-term.
 */

#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "ble_gap.h"

/* Absolute pin numbers, same port*32+pin encoding the bootloader uses.
 * REV-2021-03-26 board; on the early REV-2020-12-12 the red LED is P0.13. */
#define PIN_LED_RED   (1 * 32 + 3)   /* P1.03 */
#define PIN_LED_BLUE  (0 * 32 + 14)  /* P0.14 */
#define PIN_LED_PWR   (0 * 32 + 12)  /* P0.12, green power LED */

#define S140_730_FWID 0x0123

/* RAM ORIGIN from techo_app.ld - the linker script owns this value. */
extern uint32_t __app_ram_base;

static NRF_GPIO_Type *pin_port(uint32_t pin) { return pin < 32 ? NRF_P0 : NRF_P1; }

static void pin_output(uint32_t pin, uint32_t value)
{
  NRF_GPIO_Type *port = pin_port(pin);
  if (value) port->OUTSET = 1UL << (pin & 31);
  else       port->OUTCLR = 1UL << (pin & 31);
  port->PIN_CNF[pin & 31] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
                            (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
}

static void led_write(uint32_t pin, int on)
{
  NRF_GPIO_Type *port = pin_port(pin);
  if (on) port->OUTCLR = 1UL << (pin & 31); /* active low */
  else    port->OUTSET = 1UL << (pin & 31);
}

static void delay_ms(uint32_t ms)
{
  SysTick->LOAD = (SystemCoreClock / 1000) - 1; /* 1 ms tick */
  SysTick->VAL  = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
  while (ms--) {
    while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) { }
  }
  SysTick->CTRL = 0;
}

static void die_blinking_red(void)
{
  led_write(PIN_LED_BLUE, 0);
  for (;;) {
    led_write(PIN_LED_RED, 1);
    delay_ms(100);
    led_write(PIN_LED_RED, 0);
    delay_ms(100);
  }
}

static void die_solid_red(void)
{
  led_write(PIN_LED_BLUE, 0);
  led_write(PIN_LED_RED, 1);
  for (;;) { }
}

static void sd_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
  (void) id; (void) pc; (void) info;
  die_solid_red();
}

/* Overrides the weak alias in gcc_startup_nrf52840.S */
void HardFault_Handler(void)
{
  die_solid_red();
}

int main(void)
{
  /* Sticky reset-reason bits: the bootloader leaves them for the app to
   * clear (see bootloader src/main.c). Write-1-to-clear. */
  NRF_POWER->RESETREAS = NRF_POWER->RESETREAS;

  pin_output(PIN_LED_RED, 1);  /* off (active low) */
  pin_output(PIN_LED_BLUE, 1);
  /* Power LED rail: board_teardown() reset every pin before the app
   * started, so nothing is driven until we do it here. */
  pin_output(PIN_LED_PWR, 1);

  /* Enable the SoftDevice on the internal RC oscillator (calibrated),
   * same LF clock source the bootloader uses - works on every board rev. */
  nrf_clock_lf_cfg_t clock_cfg = {
    .source       = NRF_CLOCK_LF_SRC_RC,
    .rc_ctiv      = 16,
    .rc_temp_ctiv = 2,
    .accuracy     = NRF_CLOCK_LF_ACCURACY_500_PPM,
  };
  if (sd_softdevice_enable(&clock_cfg, sd_fault_handler) != NRF_SUCCESS) {
    die_blinking_red();
  }

  /* Enable the BLE stack with the default configuration. */
  uint32_t app_ram_base = (uint32_t) &__app_ram_base;
  if (sd_ble_enable(&app_ram_base) != NRF_SUCCESS) {
    die_blinking_red();
  }

  /* Verify the running SoftDevice really is S140 7.3.0 (FWID 0x0123). */
  ble_version_t ver;
  if (sd_ble_version_get(&ver) != NRF_SUCCESS || ver.subversion_number != S140_730_FWID) {
    die_blinking_red();
  }

  /* Non-connectable advertising: flags + complete local name "TEcho-SD730". */
  static uint8_t adv_data[] = {
    2, BLE_GAP_AD_TYPE_FLAGS, BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED,
    12, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, 'T','E','c','h','o','-','S','D','7','3','0',
  };
  ble_gap_adv_data_t gap_adv_data = {
    .adv_data      = { .p_data = adv_data, .len = sizeof(adv_data) },
    .scan_rsp_data = { .p_data = NULL, .len = 0 },
  };
  ble_gap_adv_params_t adv_params;
  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
  adv_params.interval        = 320; /* 200 ms in 0.625 ms units */
  adv_params.duration        = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED;
  adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
  adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;

  uint8_t adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
  if (sd_ble_gap_adv_set_configure(&adv_handle, &gap_adv_data, &adv_params) != NRF_SUCCESS) {
    die_blinking_red();
  }
  if (sd_ble_gap_adv_start(adv_handle, BLE_CONN_CFG_TAG_DEFAULT) != NRF_SUCCESS) {
    die_blinking_red();
  }

  /* Success: blue slow blink forever. */
  for (;;) {
    led_write(PIN_LED_BLUE, 1);
    delay_ms(500);
    led_write(PIN_LED_BLUE, 0);
    delay_ms(500);
  }
}
