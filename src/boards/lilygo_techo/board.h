/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ha Thach for Adafruit Industries
 * Copyright (c) 2021 Linar Yusupov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _LILYGO_TECHO_H
#define _LILYGO_TECHO_H

// T-Echo is supplied on VDDH; GPIO must run at 3.3V (default REGOUT0 is 1.8V)
#define UICR_REGOUT0_VALUE UICR_REGOUT0_VOUT_3V3

/*------------------------------------------------------------------*/
/* LED
 *------------------------------------------------------------------*/
#define LEDS_NUMBER       2
//#define LED_PRIMARY_PIN PINNUM(0, 13)  /* REV-2020-12-12: RED  */
#define LED_PRIMARY_PIN   PINNUM(1, 3)   /* REV-2021-03-26: RED  */
#define LED_SECONDARY_PIN PINNUM(0, 14)  /* REV-2020-12-12: BLUE */
#define LED_STATE_ON      0

#define LED_PWR_ON        PINNUM(0, 12)
//#define LED_PWR_EN      PINNUM(1,  1)  /* REV-2020-12-12       */
#define LED_PWR_EN        PINNUM(0, 13)  /* REV-2021-03-26       */

/*------------------------------------------------------------------*/
/* BUTTON
 *------------------------------------------------------------------*/
#define BUTTON_DFU     PINNUM(1, 10)  /* USB UF2 and CDC */
#define BUTTON_DFU_OTA PINNUM(0, 11)  /* BLE OTA         */
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

//--------------------------------------------------------------------+
// BLE OTA
//--------------------------------------------------------------------+
#define BLEDIS_MANUFACTURER "LilyGo"
#define BLEDIS_MODEL        "T-Echo v1"

//--------------------------------------------------------------------+
// USB
//--------------------------------------------------------------------+
// Shared VID/PID with feather_nrf52840_express: kept on purpose to match the
// factory LilyGo T-Echo bootloader (0.5.0/0.6.1) identity, so bootloader-update
// UF2 verification (BOOTLOADER_BOARD_ID) and existing tooling keep working on
// devices in the field. Consequence: board-ID checks cannot tell a T-Echo from
// a Feather nRF52840 Express. Never flash Feather bootloader updates here.
#define USB_DESC_VID          0x239A
#define USB_DESC_UF2_PID      0x0029
#define USB_DESC_CDC_ONLY_PID 0x002A

//------------- UF2 -------------//
#define UF2_PRODUCT_NAME "LilyGo T-Echo"
#define UF2_VOLUME_LABEL "TECHOBOOT"
#define UF2_BOARD_ID     "nRF52840-TEcho-v1"
#define UF2_INDEX_URL    "http://www.lilygo.cn/claprod_view.aspx?Id=1387"

#endif // _LILYGO_TECHO_H
