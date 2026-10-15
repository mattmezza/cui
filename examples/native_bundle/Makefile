# No CUI installation, Python, CMake, network access, or implementation macros.
.DEFAULT_GOAL := all
CC ?= cc
AR ?= ar
PKG_CONFIG ?= pkg-config
CFLAGS ?= -O2 -g
CPPFLAGS += -Ivendor/include -Ivendor/third_party/clay
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic

# Check system prerequisites before compilation. CUI itself is in this bundle.
ifeq ($(shell $(PKG_CONFIG) --exists 'sdl3 >= 3.2' freetype2 harfbuzz && echo yes),)
$(error Install SDL3 >= 3.2, FreeType and HarfBuzz development packages; ensure pkg-config can find sdl3 freetype2 harfbuzz)
endif
NATIVE_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl3 freetype2 harfbuzz)
LDLIBS += $(shell $(PKG_CONFIG) --libs sdl3 freetype2 harfbuzz) -lm

include sources.mk
PORTABLE_OBJECTS := $(PORTABLE_SOURCES:%.c=build/%.o)
TEXT_OBJECTS := $(TEXT_SOURCES:%.c=build/%.o)
BIDI_OBJECTS := $(BIDI_SOURCES:%.c=build/%.o) $(SHEENBIDI_SOURCES:%.c=build/%.o)
SDL3_OBJECTS := $(SDL3_SOURCES:%.c=build/%.o)
OBJECTS := $(PORTABLE_OBJECTS) $(TEXT_OBJECTS) $(BIDI_OBJECTS) $(SDL3_OBJECTS)
LIBRARIES := build/vendor/libcui_sdl3.a build/vendor/libcui_text.a build/vendor/libcui_bidi.a build/vendor/libcui.a

all: build/native_starter

build/native_starter: build/main.o $(LIBRARIES)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/vendor/libcui.a: $(PORTABLE_OBJECTS)
	$(AR) rcs $@ $^
build/vendor/libcui_text.a: $(TEXT_OBJECTS)
	$(AR) rcs $@ $^
build/vendor/libcui_bidi.a: $(BIDI_OBJECTS)
	$(AR) rcs $@ $^
build/vendor/libcui_sdl3.a: $(SDL3_OBJECTS)
	$(AR) rcs $@ $^

$(TEXT_OBJECTS): CPPFLAGS += -DCUI_TEXT_BIDI
$(BIDI_OBJECTS): CPPFLAGS += -Ivendor/third_party/sheenbidi/Headers
$(SHEENBIDI_SOURCES:%.c=build/%.o): CPPFLAGS += -DSB_CONFIG_UNITY -Ivendor/third_party/sheenbidi/Source
# Narrow suppressions match CMake for unmodified pinned upstream SheenBidi.
$(SHEENBIDI_SOURCES:%.c=build/%.o): UPSTREAM_WARNINGS := -Wno-unused-parameter -Wno-unused-value -Wno-unused-function -Wno-implicit-fallthrough

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(NATIVE_CFLAGS) $(CFLAGS) $(UPSTREAM_WARNINGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d) build/main.d
.PHONY: all
