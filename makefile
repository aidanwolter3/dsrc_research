#Taken from https://github.com/eehusky/Stellaris-GCC then modified

#Target Binary Name
TARGET      = dsrc

# List all the source files here
SOURCES += dsrc.c
SOURCES += startup.c
SOURCES += syscalls.c
SOURCES += common.c
SOURCES += console.c
SOURCES	+= mikro.c
SOURCES += wifi_mcw1001a_mikro.c
SOURCES += gps_l80_mikro.c
SOURCES += task_manager.c

#SOURCES  = $(wildcard src/*.c)

# Includes are located in the Include directory
INCLUDES    = -Isrc/include

# Path to the root of your ARM toolchain
TOOL        = /usr/local/sat

# Path to the root of your StellarisWare folder
TIVAWARE    = ../tivaware

# Location of a loader script, doesnt matter which one, they're the same
LD_SCRIPT   = lm4c123gxl.ld

# Object File Directory, keeps things tidy
OBJECTS     = $(patsubst %.o, .obj/%.o,$(SOURCES:.c=.o))
ASMS        = $(patsubst %.s, .obj/%.s,$(SOURCES:.c=.s))

# FPU Type
#FPU         = hard
FPU         = softfp

# Remove # to enable verbose
#VERBOSE     = 1

# Flag Definitions
###############################################################################
CFLAGS     += -mthumb
CFLAGS     += -mcpu=cortex-m4
CFLAGS     += -mfloat-abi=$(FPU)
CFLAGS     += -mfpu=fpv4-sp-d16
CFLAGS     += -Os
CFLAGS     += -ffunction-sections
CFLAGS     += -fdata-sections
CFLAGS     += -MD
CFLAGS     += -std=c99
CFLAGS     += -Wall
CFLAGS     += -pedantic
CFLAGS     += -DPART_TM4C123GH6PM
CFLAGS     += -Dgcc
CFLAGS     += -DTARGET_IS_TM4C123_RB1
CFLAGS     += -fsingle-precision-constant
CFLAGS     += -I$(TIVAWARE) $(INCLUDES)

CFLAGS     += -g
LDFLAGS    += -T $(LD_SCRIPT)
LDFLAGS    += --entry ResetISR
LDFLAGS    += --gc-sections
LDFLAGS    += -Map .obj/$(TARGET).map
LDFLAGS    += --cref
LDFLAGS    += -nostdlib
###############################################################################


# Tool Definitions
###############################################################################
CC          = $(TOOL)/bin/arm-none-eabi-gcc
LD          = $(TOOL)/bin/arm-none-eabi-ld
AR          = $(TOOL)/bin/arm-none-eabi-ar
AS          = $(TOOL)/bin/arm-none-eabi-as
NM          = $(TOOL)/bin/arm-none-eabi-nm
OBJCOPY     = $(TOOL)/bin/arm-none-eabi-objcopy
OBJDUMP     = $(TOOL)/bin/arm-none-eabi-objdump
RANLIB      = $(TOOL)/bin/arm-none-eabi-ranlib
STRIP       = $(TOOL)/bin/arm-none-eabi-strip
SIZE        = $(TOOL)/bin/arm-none-eabi-size
READELF     = $(TOOL)/bin/arm-none-eabi-readelf
DEBUG       = $(TOOL)/bin/arm-none-eabi-gdb
FLASH       = lm4flash
CP          = cp -p
RM          = rm -rf
MV          = mv
MKDIR       = mkdir -p
##############################################################################

LIBGCC			= $(TOOL)/lib/gcc/arm-none-eabi/4.7.3/thumb/cortex-m4/libgcc.a
LIBC				= $(TOOL)/arm-none-eabi/lib/thumb/cortex-m4/libc.a
LIBM				= $(TOOL)/arm-none-eabi/lib/thumb/cortex-m4/libm.a
DRIVER_LIB	= $(TIVAWARE)/driverlib/gcc/libdriver.a
LIBS        = '$(LIBM)' '$(LIBC)' '$(LIBGCC)' '$(DRIVER_LIB)'


# Command Definitions
###############################################################################

#default to green device, because blue is better
all: clean
all: CFLAGS += -DSELECT_GREEN_DEVICE
all: build

build: dirs bin/$(TARGET).bin size

blue: clean
blue: CFLAGS += -DSELECT_BLUE_DEVICE
blue: flash

green: clean
green: CFLAGS += -DSELECT_GREEN_DEVICE
green: flash

screen: flash
	screen /dev/tty.usbmodem0E20F341 115200

openocd:
	openocd -f board/ek-tm4c123gxl.cfg

debug: all
	arm-none-eabi-gdb bin/${TARGET}.axf -ex "target extended-remote :3333; monitor reset halt; monitor reset init;"
	#ps | grep openocd | grep -v grep | awk '{print "kill -9 " $1}' | sh

asm: $(ASMS)

# Compiler Command
.obj/%.o: src/%.c
	@if [ 'x${VERBOSE}' = x ];                                                \
	 then                                                                     \
	     echo "CC           ${<}";                                            \
	 else                                                                     \
	     echo $(CC) -c $(CFLAGS) -o $@ $<;                                    \
	 fi
	@$(MKDIR) $(dir $@)
	@$(CC) -c $(CFLAGS) -o $@ $<

# Create Assembly
.obj/%.s: src/%.c
	@if [ 'x${VERBOSE}' = x ];                                                \
	 then                                                                     \
	     echo "CC -S        ${<}";                                            \
	 else                                                                     \
	     echo $(CC) -S -c $(CFLAGS) -o $@ $<;                                 \
	 fi
	@$(MKDIR) $(dir $@)
	@$(CC) -S -c $(CFLAGS) -o $@ $<

# Linker Command
.obj/$(TARGET).out: $(OBJECTS)
	@if [ 'x${VERBOSE}' = x ];                                                \
	 then                                                                     \
	     echo "LD           $@";                                              \
	 else                                                                     \
	     echo $(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS);\
	 fi
	@$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS);

# Create the Final Image
bin/$(TARGET).bin: .obj/$(TARGET).out
	@if [ 'x${VERBOSE}' = x ];                                                \
	 then                                                                     \
	     echo "OBJCOPY      ${<} $@";                                         \
	 else                                                                     \
	     echo $(OBJCOPY) -O binary .obj/$(TARGET).out bin/$(TARGET).bin;      \
	 fi
	@$(OBJCOPY) -O binary .obj/$(TARGET).out bin/$(TARGET).bin

# Calculate the Size of the Image
size: .obj/$(TARGET).out
	@if [ 'x${VERBOSE}' = x ];                                                \
	 then                                                                     \
	     echo "SIZE      ${<}";                                               \
	 else                                                                     \
	     echo $(SIZE) $<;                                                     \
	 fi
	@$(SIZE) $<


# Create the Directories we need
dirs:
	@$(MKDIR) src/include
	@$(MKDIR) bin
	@$(MKDIR) .obj

# Cleanup
clean:
	-$(RM) .obj/*
	-$(RM) bin/*

# Flash The Board
flash: build
	@if [ 'x${VERBOSE}' = x ];                                                \
	 then                                                                     \
	     echo "  FLASH    bin/$(TARGET).bin";                                 \
	 else                                                                     \
	     echo $(FLASH) bin/$(TARGET).bin;                                \
	 fi
	@$(FLASH) bin/$(TARGET).bin

# Redo, Clean->Compile Fresh Image, and Install It.
redo: clean all install
###############################################################################

