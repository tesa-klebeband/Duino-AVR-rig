CPP = g++
LINKER = -lcurl -lcrypto -lpthread
BUILD_DIR = build

all: prep duino-avr-rig

prep:
	mkdir -p $(BUILD_DIR)

duino-avr-rig: src/*.cpp
	$(CPP) -Ofast $^ -o $(BUILD_DIR)/$@ $(LINKER)