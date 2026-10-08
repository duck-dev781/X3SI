.SUFFIXES:

ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON")
endif

include $(DEVKITXENON)/rules

TARGET := ChasesDevBoxBoot
BUILD := build
SOURCES := source

CFLAGS := -O2 -Wall $(MACHDEP) $(INCLUDE)
LDFLAGS := -g $(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(TARGET).map
LIBS := -lxenon -lpng -lz -lm

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(CURDIR)/$(SOURCES)
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(notdir $(wildcard $(SOURCES)/*.c))
OFILES := $(CFILES:.c=.o) badavatar.o phoenix.o

export LD := $(CC)
export INCLUDE := -I$(CURDIR)/$(BUILD) -I$(LIBXENON_INC)
export LIBPATHS := -L$(LIBXENON_LIB)

.PHONY: all clean
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C "$(BUILD)" -f "$(CURDIR)/Makefile"

clean:
	rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32 $(OUTPUT).map

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

badavatar.o: ../assets/badavatar.png
	cd ../assets && $(OBJCOPY) -I binary -O elf32-powerpc -B powerpc badavatar.png $(CURDIR)/badavatar.o

phoenix.o: ../assets/phoenix.png
	cd ../assets && $(OBJCOPY) -I binary -O elf32-powerpc -B powerpc phoenix.png $(CURDIR)/phoenix.o

-include $(DEPENDS)

endif
