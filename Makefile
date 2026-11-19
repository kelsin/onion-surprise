TARGET   = App/Surprise/surprise
SRCS     = $(wildcard src/*.c) vendor/cjson/cJSON.c
CC       = $(CROSS_COMPILE)gcc
CFLAGS   = -O2 -Wall -Wextra -std=gnu18 -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 \
           -mfloat-abi=hard -march=armv7ve \
           -I$(PREFIX)/include -Ivendor/cjson -Isrc
LDFLAGS  = -L$(PREFIX)/lib -lSDL -lSDL_ttf -lSDL_image -lm -lpthread

# Host unit tests: pure (non-SDL) modules only. Add sources here.
TEST_SRCS = src/paths.c src/games.c src/launch.c
TEST_BIN  = build/tests
HOST_CC  ?= cc

all: $(TARGET)

$(TARGET): $(SRCS) $(wildcard src/*.h)
	@mkdir -p $(dir $(TARGET))
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)
	$(CROSS_COMPILE)strip $@

test:
ifeq ($(strip $(TEST_SRCS)),)
	@echo "no tests yet"
else
	@mkdir -p build
	$(HOST_CC) -O0 -g -Wall -std=gnu11 -Ivendor/cjson -Isrc $(wildcard tests/*.c) $(TEST_SRCS) vendor/cjson/cJSON.c -o $(TEST_BIN)
	./$(TEST_BIN)
endif

# Native build for the headless preview (run inside toolchain/preview.Dockerfile).
PREVIEW_BIN = build/preview/surprise
preview-bin: $(SRCS) $(wildcard src/*.h)
	@mkdir -p build/preview/res
	$(HOST_CC) -O2 -Wall -Wextra -std=gnu18 -Ivendor/cjson -Isrc $(SRCS) -o $(PREVIEW_BIN) -lSDL -lSDL_ttf -lSDL_image -lm
	cp App/Surprise/res/font.ttf build/preview/res/font.ttf

clean:
	rm -rf $(TARGET) build

.PHONY: all test clean preview-bin
