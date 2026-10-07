#---------------------------------------------------------------------------------
# X3SI Xbox 360 LibXenon build
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON in your environment. export DEVKITXENON=<path to devkitXenon>")
endif

include $(DEVKITXENON)/rules

TARGET := X3SI
BUILD := build
SOURCES := source
INCLUDES :=

CFLAGS := -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS := $(CFLAGS)
LDFLAGS := -g $(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(notdir $@).map
LIBS := -lxenon -lm -lfat

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(CURDIR)/$(SOURCES)
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(notdir $(wildcard $(SOURCES)/*.c))
CPPFILES := $(notdir $(wildcard $(SOURCES)/*.cpp))
sFILES := $(notdir $(wildcard $(SOURCES)/*.s))
SFILES := $(notdir $(wildcard $(SOURCES)/*.S))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  -I$(CURDIR)/$(BUILD) \
                  -I$(LIBXENON_INC)

export LIBPATHS := -L$(LIBXENON_LIB)

.PHONY: all clean
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
