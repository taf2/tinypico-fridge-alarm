ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: detector
upload: detector
	pio run -j 2 --target upload && sleep 1;  pio device monitor

pico:
	pio run -e pico -j 2 --target upload && sleep 1;  pio device monitor

tinypico:
	pio run -e tinypico -j 2 --target upload && sleep 1;  pio device monitor

tinys3:
	pio run -e um_tinys3 -j 2 --target upload --upload-port /dev/cu.usbmodem14101 -v && sleep 1;  pio device monitor

detector: targets
	pio run

targets: srcfiles/main.cpp
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "TinyPICO Helper Library"

clean:
	pio run --target clean
