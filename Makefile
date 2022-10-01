ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

all: detector
upload: detector
	pio run -j 2 --target upload && sleep 1;  pio device monitor

pico: targets
	pio run -e pico -j 2 --target upload && sleep 1;  pio device monitor

tinypico: targets
	pio run -e tinypico -j 2 --target upload && sleep 1;  pio device monitor

tinys3: targets
	pio run -e um_tinys3 -j 2 --target upload  && sleep 1;  pio device monitor

detector: targets
	pio run

targets: srcfiles/main.cpp
	mkdir -p src
	ruby build.rb main.cpp

monitor:
	pio device monitor

configure:
	pio lib install "TinyPICO Helper Library"

clean:
	pio run --target clean
