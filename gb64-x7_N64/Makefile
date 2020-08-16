ROOTDIR = $(N64_INST)
GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
HEADERPATH = $(ROOTDIR)/mips64-elf/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
ROMFIX64 = ./romfix64
LINK_FLAGS = -L$(ROOTDIR)/mips64-elf/lib -ldragon -lnosys -lc -lm -T n64.ld 
PROG_NAME = gb64-x7
FLAGS = -std=gnu99 -march=vr4300 -mtune=vr4300 -O2 -Wall -Wno-pointer-sign -lssl -lcrypto -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include -I$(ROOTDIR)/libdragon/include -Iedlib -I. -G0 
FLAGS_VT = -std=gnu99 -march=vr4300 -mtune=vr4300 -O0 -Wall -Wno-pointer-sign  -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include -I$(ROOTDIR)/libdragon/include -Iedlib -I.
ASFLAGS = -mtune=vr4300 -march=vr4300
CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy

BDIR		:=build
VPATH		:= $(BDIR)

MAKEFLAGS= -j8

SRC = $(wildcard *.c) 
SRCS = $(wildcard *.s) 
OBJ = $(SRC:.c=.o)
OBJ += $(SRCS:.s=.o)
OFILES = $(OBJ:%.o=build/%.o)


all: $(PROG_NAME).v64


$(PROG_NAME).v64: $(PROG_NAME).elf
	$(OBJCOPY) $(BDIR)/$(PROG_NAME).elf $(BDIR)/$(PROG_NAME).bin -O binary
	$(RM) -f $(PROG_NAME).v64
	$(N64TOOL) -l 2M -t "gb64-x7" -h $(HEADERPATH)/header -o $(PROG_NAME).v64 $(BDIR)/$(PROG_NAME).bin -s
	$(CHKSUM64PATH) $(PROG_NAME).v64

$(PROG_NAME).elf : $(OBJ)
	$(LD) -o $(BDIR)/$(PROG_NAME).elf $(OFILES) /usr/local/libdragon/build/system.o $(LINK_FLAGS) 

gscore.o: gscore.c
	$(CC) $(FLAGS_VT) -c gscore.c -o $(BDIR)/gscore.o

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $(BDIR)/$@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $(BDIR)/$@

clean:
	$(RM) -f *.v64 *.elf *.o
	$(RM) $(BDIR)/*.o
	$(RM) $(BDIR)/*.elf
	$(RM) $(BDIR)/*.bin
