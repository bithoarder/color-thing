all:: build/test.ihex

DEVICE = attiny45
ifeq ($(DEVICE),attiny45)
	DEVICE_ROM = 4096
	DEVICE_RAM = 256
else
ifeq ($(DEVICE),attiny85)
	DEVICE_ROM = 8192
	DEVICE_RAM = 512
endif
	$(error unknown device)
endif
CC = avr-gcc
CXX = avr-g++
OBJCOPY = avr-objcopy
CC_FLAGS = -MMD -MP --pipe -Wall -Werror -ffunction-sections -Os -Ivusb/usbdrv -Ivusb/libs-device -I. -mmcu=$(DEVICE) -DF_CPU=16500000 -DDEBUG_LEVEL=0
CXX_FLAGS = $(CC_FLAGS)
LD_FLAGS = -mmcu=$(DEVICE) -Wl,--relax -Wl,--gc-sections -Wl,--print-gc-sections

SOURCES = main.cpp \
	debug.cpp \
	uart_bitbang.s \
	vusb/usbdrv/usbdrv.c \
	vusb/usbdrv/usbdrvasm.S \
	vusb/libs-device/osccal.c

build/%.o: %.c makefile
	@mkdir -p $$(dirname $@)
	$(CC) $(CC_FLAGS) -c -o $@ $<

build/%.o: %.cpp makefile
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXX_FLAGS) -S -o $@.s $<
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

build/%.o: %.S makefile
	@mkdir -p $$(dirname $@)
	$(CC) $(CC_FLAGS) -x assembler-with-cpp -c -o $@ $<

build/%.o: %.s makefile
	@mkdir -p $$(dirname $@)
	$(CC) $(CC_FLAGS) -x assembler-with-cpp -c -o $@ $<

%.ihex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $^ $@
	@./checksize $^ $(DEVICE_ROM) $(DEVICE_RAM)

build/test.elf:	$(addprefix build/, $(addsuffix .o, $(basename $(SOURCES))))
	$(CC) $(LD_FLAGS) -o $@ -Wl,-Map -Wl,$@.map $^

flash:: build/test.ihex
	avrdude -v -v -v -v -v -cdragon_dw -Pusb -p$(DEVICE) -U flash:w:$^

debugwire::
	avrdude -cdragon_isp -Pusb -p$(DEVICE) -U hfuse:w:0x1d:m -U lfuse:w:0xe1:m

-include $(addprefix build/, $(addsuffix .d, $(basename $(SOURCES))))

