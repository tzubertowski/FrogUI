STATIC_LINKING := 0
AR             := ar

ifneq ($(V),1)
   Q := @
endif

ifeq ($(platform),)
platform = unix
endif

# system platform
system_platform = unix

CORE_DIR    := .
TARGET_NAME := menu
LIBM        = -lm

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

# SF2000 platform
ifeq ($(platform), sf2000)
   TARGET := _libretro_$(platform).a
   MIPS=/opt/mips32-mti-elf/2019.09-03-2/bin/mips-mti-elf-
   CC = $(MIPS)gcc
   CXX = $(MIPS)g++
   AR = $(MIPS)ar
   CFLAGS = -EL -march=mips32 -mtune=mips32 -msoft-float -G0 -mno-abicalls -fno-pic
   CFLAGS += -ffast-math -fomit-frame-pointer -ffunction-sections -fdata-sections
   CFLAGS += -O3 -DSF2000 -DNDEBUG  # -O3 for speed, NDEBUG removes asserts
   CXXFLAGS := $(CFLAGS)
   STATIC_LINKING = 1
else
   # Default unix build for testing
   EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
endif

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g -DDEBUG
   CXXFLAGS += -O0 -g -DDEBUG
else
   CFLAGS += -Os
   CXXFLAGS += -Os
endif

# Source files
SOURCES_C := frogos.c font.c render.c recent_games.c settings.c theme.c favorites.c

OBJECTS := $(SOURCES_C:.c=.o)

# Include directories
INCFLAGS := -I../..

CFLAGS   += $(fpic) -Wall -D__LIBRETRO__ $(INCFLAGS)
CXXFLAGS += $(fpic) -Wall -D__LIBRETRO__ $(INCFLAGS)

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	@$(if $(Q), $(shell echo echo LD $@),)
	$(Q)$(CC) $(fpic) $(SHARED) -o $@ $(OBJECTS) $(LDFLAGS)
endif

%.o: %.c
	@$(if $(Q), $(shell echo echo CC $<),)
	$(Q)$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean all
