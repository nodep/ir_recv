TARGET  = ir_recv
DEVICE  = atmega88
F_CPU   = 12000000
AVRDUDE = avrdude -c stk500v2 -P avrdoper -p $(DEVICE)
#AVRDUDE = avrdude -c dragon_isp -P usb -B1 -p $(DEVICE)

CFLAGS  = -I. -Iusbdrv -flto
CFLAGS += -DDBGPRINT

LFLAGS  = -Wl,--relax -flto
# LFLAGS += -u vfprintf -lprintf_min

#VPATH   = ../../common:..:../../mcu-lib

OBJECTS = $(TARGET).o ir_decoder.o ir_panasonic.o ir_samsung.o avrdbg.o vusb.o reports.o usbdrv/usbdrv.o usbdrv/usbdrvasm.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

hex: $(TARGET).hex

$(TARGET).hex: $(OBJECTS)
	$(COMPILE) -o $(TARGET).elf $(OBJECTS) $(LFLAGS)
	rm -f $(TARGET).hex $(TARGET).eep.hex
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-size $(TARGET).elf

flash: all $(TARGET).hex
	$(AVRDUDE) -V -U flash:w:$(TARGET).hex:i

clean:
	rm -f *.hex *.elf $(OBJECTS)

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.o:
	$(COMPILE) -c $< -o $@

all: clean hex
