# ----------------------------------------------------------------------------
#         ATMEL Microcontroller Software Support
# ----------------------------------------------------------------------------
# Copyright (c) 2008, Atmel Corporation
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice,
# this list of conditions and the disclaimer below.
#
# Atmel's name may not be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
# DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------


#-------------------------------------------------------------------------------
#  User-modifiable options
#-------------------------------------------------------------------------------

# Chip & board used for compilation
# (can be overriden by adding CHIP=chip and BOARD=board to the command-line)
CHIP  = at91sam7x512
BOARD = at91sam7x-ek

# Trace level used for compilation
# (can be overriden by adding TRACE_LEVEL=#number to the command-line)
# TRACE_LEVEL_DEBUG      5
# TRACE_LEVEL_INFO       4
# TRACE_LEVEL_WARNING    3
# TRACE_LEVEL_ERROR      2
# TRACE_LEVEL_FATAL      1
# TRACE_LEVEL_NO_TRACE   0
TRACE_LEVEL = 0

# Optimization level, put in comment for debugging
OPTIMIZATION = -march=armv4t -mcpu=arm7tdmi -Os -g0 -fomit-frame-pointer #-falign-functions -falign-jumps -falign-loops -falign-labels -freorder-blocks -ftree-vect-loop-version -fomit-frame-pointer -funroll-loops -finline-functions -funswitch-loops -fpredictive-commoning -fgcse-after-reload -fipa-cp-clone

#-g

# AT91 library directory
AT91LIB = ../at91lib

# Compile for all memories available on the board (this sets $(MEMORIES))
include $(AT91LIB)/boards/$(BOARD)/board.mak

# Output file basename
OUTPUT = lightbox-$(BOARD)-$(CHIP)

# Output directories
BIN = bin
OBJ = obj

#-------------------------------------------------------------------------------
#  Tools
#-------------------------------------------------------------------------------

# Tool suffix when cross-compiling
CROSS_COMPILE = arm-none-eabi-

# Compilation tools
CC = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy

# Flags
INCLUDES = -I$(AT91LIB)/boards/$(BOARD) -I$(AT91LIB)/peripherals
INCLUDES += -I$(AT91LIB)/drivers -I$(AT91LIB)/components -I$(AT91LIB) -I.

CFLAGS = -Wall -mlong-calls -mstructure-size-boundary=8 -ffunction-sections
CFLAGS += $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -DTRACE_LEVEL=$(TRACE_LEVEL)
CFLAGS += -DREENTRANT_SYSCALLS_PROVIDED

ASFLAGS = $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -D__ASSEMBLY__
LDFLAGS = $(OPTIMIZATION) -nostartfiles -Wl,--gc-sections

#-------------------------------------------------------------------------------
#  Files
#-------------------------------------------------------------------------------

# Directories where source files can be found
COMP = $(AT91LIB)/components
DRIVERS = $(AT91LIB)/drivers
PERIPH = $(AT91LIB)/peripherals
BOARDS = $(AT91LIB)/boards
UTILITY = $(AT91LIB)/utility
# uIP library directory
UIPLIB = ../external_libs/ethernet/uip

VPATH += $(UTILITY)
VPATH += $(PERIPH)/dbgu $(PERIPH)/usart $(PERIPH)/pio $(PERIPH)/irq $(PERIPH)/tc
VPATH += $(PERIPH)/emac $(PERIPH)/rstc
VPATH += $(PERIPH)/cp15
VPATH += $(DRIVERS)/macb
VPATH += $(BOARDS)/$(BOARD) $(BOARDS)/$(BOARD)/$(CHIP)
VPATH += $(UIPLIB)/uip
VPATH += $(UIPLIB)/lib
#VPATH += $(UIPLIB)/apps/dhcpc
#VPATH += $(UIPLIB)/apps/telnetd

INCLUDES += -I$(UTILITY)
INCLUDES += -I$(UIPLIB)/uip
INCLUDES += -I$(UIPLIB)/lib
# INCLUDES += -I$(UIPLIB)/apps/dhcpc
#INCLUDES += -I$(UIPLIB)/apps/telnetd

# Objects built from C source files
C_OBJECTS = stdio.o
C_OBJECTS += string.o
C_OBJECTS += board_memories.o board_lowlevel.o
C_OBJECTS += dbgu.o usart.o pio.o aic.o tc.o cp15.o
C_OBJECTS += emac.o macb.o rstc.o

#UIP:
#lib
C_OBJECTS += memb.o
#uip
C_OBJECTS += psock.o timer.o uip.o  uip_arp.o
#main
C_OBJECTS += main.o clock-arch.o tapdev.o
#dhcp
# C_OBJECTS += dhcpc.o
#telnetd
C_OBJECTS += lightbox.o

# Objects built from Assembly source files
ASM_OBJECTS = board_cstartup.o
ASM_OBJECTS += cp15_asm_gcc.o

# Append OBJ and BIN directories to output filename
OUTPUT := $(BIN)/$(OUTPUT)

#-------------------------------------------------------------------------------
#  Rules
#-------------------------------------------------------------------------------

all: $(BIN) $(OBJ) $(MEMORIES)

$(BIN) $(OBJ):
	mkdir $@

define RULES
C_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(C_OBJECTS))
ASM_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(ASM_OBJECTS))

$(1): $$(ASM_OBJECTS_$(1)) $$(C_OBJECTS_$(1))
	$(CC) $(LDFLAGS) -T"$(AT91LIB)/boards/$(BOARD)/$(CHIP)/$$@.lds" -o $(OUTPUT)-$$@.elf $$^
	$(OBJCOPY) -O binary $(OUTPUT)-$$@.elf $(OUTPUT)-$$@.bin
	$(SIZE) $$^ $(OUTPUT)-$$@.elf

$$(C_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.c Makefile $(OBJ) $(BIN)
	$(CC) $(CFLAGS) -D$(1) -c -o $$@ $$<

$$(ASM_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.S Makefile $(OBJ) $(BIN)
	$(CC) $(ASFLAGS) -D$(1) -c -o $$@ $$<

debug_$(1): $(1)
	perl ../resources/gdb/debug.pl $(OUTPUT)-$(1).elf

endef

$(foreach MEMORY, $(MEMORIES), $(eval $(call RULES,$(MEMORY))))

clean:
	-rm -f $(OBJ)/*.o $(BIN)/*.bin $(BIN)/*.elf

