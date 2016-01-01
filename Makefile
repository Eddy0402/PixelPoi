MAKDIR = mk
MAK = $(wildcard $(MAKDIR)/*.mk)
include $(MAK)

ELFOUT=$(DEVICE).out
FIRMWARE=$(DEVICE).hex
OUTDIR=build

INCLUDE_DIR += \
    include

SRCDIR += \
    src \

SRC += \
    $(wildcard $(addsuffix /*.c,$(SRCDIR))) \
    $(wildcard $(addsuffix /*.cpp,$(SRCDIR))) \
    $(wildcard $(addsuffix /*.s,$(SRCDIR))) \

INCLUDES = $(addprefix -I,$(INCLUDE_DIR))
OBJS := $(addprefix $(OUTDIR)/,$(patsubst %.S, %.o, $(patsubst %.s, %.o, $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SRC))))))
DEPENDS = $(addsuffix .d,$(OBJS))

COMMONFLAGS = -I $(SUPPORT_FILE_DIRECTORY) $(INCLUDES) -mmcu=$(DEVICE) -O2 -g -Dasm=__asm__
CFLAGS = $(COMMONFLAGS) -std=c11
CXXFLAGS = $(COMMONFLAGS) -fno-exceptions -fno-rtti -std=c++11
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY) -T $(DEVICE).ld

all: $(FIRMWARE)

$(OUTDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo " CC      "$@
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUTDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo " CXX      "$@
	@$(CXX) $(CXXFLAGS) -MMD -MF $@.d -c $< -o $@

$(OUTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo " CC      "$@
	@$(CC) $(CFLAGS) -MMD -MF $@.d -c $< -o $@

$(FIRMWARE): $(ELFOUT)
	@echo " OBJCOPY " $? $@
	@$(OBJCOPY) -Oihex $? $@

$(ELFOUT): $(OBJS)
	@echo " LD      " $@
	@$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

debug: all
	$(GDB_SERVER) $(GDB_SERVER_DAT) &
	$(GDB) $(ELFOUT) -x tools/gdbscript

flash:
	LD_LIBRARY_PATH=$(FLASH_LIBDIR) $(FLASH) -w $(FIRMWARE) -v -g -z [VCC]
	LD_LIBRARY_PATH=$(FLASH_LIBDIR) $(FLASH) -r [Firmware Output.txt,MAIN]

clean:
	rm -rf $(OBJS) $(FIRMWARE) $(ELFOUT) $(DEPENDS)

.PHONY: all clean flash debug

-include $(DEPENDS)
