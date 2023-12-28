
XC8=/Applications/microchip/xc8/v2.40/bin/xc8
PIC=18F47Q43

all: build/hello-serial.hex

build/hello-serial.hex: hello-serial.c
	mkdir -p build
	cp hello-serial.c build/
	(cd build && $(XC8) --chip=$(PIC) hello-serial.c)

clean::
	rm -rf build
