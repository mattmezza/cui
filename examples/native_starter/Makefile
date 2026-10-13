CC ?= cc
PKG_CONFIG ?= pkg-config
BUILD_DIR ?= build
CFLAGS ?= -O2 -g
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags cui-sdl3)
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(PKG_CONFIG) --static --libs cui-sdl3)
all: $(BUILD_DIR)/native_starter $(BUILD_DIR)/test_starter_app
$(BUILD_DIR):
	mkdir -p $@
$(BUILD_DIR)/native_starter: main.c app.c app.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c app.c $(LDFLAGS) $(LDLIBS) -o $@
$(BUILD_DIR)/test_starter_app: test_app.c app.c app.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) test_app.c app.c $(LDFLAGS) $(LDLIBS) -o $@
test: $(BUILD_DIR)/test_starter_app
	$(BUILD_DIR)/test_starter_app
.PHONY: all test
