# T-Echo S140 7.3.0 proof-of-life sample app
# Reuses the SoftDevice / MDK / CMSIS files vendored in the bootloader clone.

BL_ROOT ?= ../Adafruit_nRF52_Bootloader
SD_INC    = $(BL_ROOT)/lib/softdevice/s140_nrf52_7.3.0/s140_nrf52_7.3.0_API/include
MDK       = $(BL_ROOT)/lib/nrfx/mdk
CMSIS_INC = $(BL_ROOT)/lib/tinyusb/lib/CMSIS_5/CMSIS/Core/Include
UF2CONV   = $(BL_ROOT)/lib/uf2/utils/uf2conv.py

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
PYTHON3 ?= python3

# __START=main + __STARTUP_CLEAR_BSS: the vendored MDK startup handles
# .data copy, .bss zero and SystemInit (errata workarounds), then jumps
# straight to main - no newlib crt0 needed.
CFLAGS  = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft \
          -Os -g -Wall -Wextra -ffunction-sections -fdata-sections \
          -DNRF52840_XXAA -D__START=main -D__STARTUP_CLEAR_BSS \
          -I$(SD_INC) -I$(SD_INC)/nrf52 -I$(MDK) -I$(CMSIS_INC)
LDFLAGS = -T techo_app.ld -L$(MDK) -nostartfiles --specs=nano.specs -Wl,--gc-sections

SRC = main.c $(MDK)/system_nrf52840.c $(MDK)/gcc_startup_nrf52840.S

BUILD = _build
NAME  = techo_sd730_sample

all: $(BUILD)/$(NAME).uf2

$(BUILD)/$(NAME).elf: $(SRC) techo_app.ld
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)
	arm-none-eabi-size $@

$(BUILD)/$(NAME).hex: $(BUILD)/$(NAME).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(NAME).uf2: $(BUILD)/$(NAME).hex
	$(PYTHON3) $(UF2CONV) -f 0xADA52840 -c -o $@ $<

clean:
	rm -rf $(BUILD)

.PHONY: all clean
