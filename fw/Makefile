# Makefile for linux with arduino-cli

CORE=arduino:avr
BOARD=uno
PORT=/dev/ttyACM0
URL_CH55XDUINO=https://github.com/DeqingSun/ch55xduino/blob/ch55xduino/package_ch55xduino_mcs51_index.json

all:
	@echo "'make upload BOARD=uno' for Arduino Uno"
	@echo "'make upload BOARD=nano' for Arduino Nano"
	@echo "'make upload BOARD=leonardo' for Arduino Leonardo"
	@echo "'make upload BOARD=ch552' for CH552/CH554 (CH55xDuino)"

compile:
ifeq ($(BOARD),ch552)
	arduino-cli core install CH55xDuino:mcs51 --additional-urls $(URL_CH55XDUINO)
	arduino-cli compile -b CH55xDuino:mcs51:ch552 pp
else
	arduino-cli core install $(CORE)
	arduino-cli compile -b $(CORE):$(BOARD) pp
endif

upload: compile
ifeq ($(BOARD),ch552)
	arduino-cli upload -b CH55xDuino:mcs51:ch552 pp
else
	arduino-cli upload -p $(PORT) -b $(CORE):$(BOARD) pp
endif

