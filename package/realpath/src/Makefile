#
# Makefile for realpath
#

#TOOLCHAIN_HOME = /home/andrei/NETGEAR/WNDR3700-V1-0-4-68-GPL/staging_dir_mips
#CC = $(TOOLCHAIN_HOME)/bin/mips-linux-gcc
#CPP = $(TOOLCHAIN_HOME)/bin/mips-linux-g++
#STRIP = $(TOOLCHAIN_HOME)/bin/mips-linux-strip
#CFLAGS = -Os -O2 -D_FILE_OFFSET_BITS=64 -I$(TOOLCHAIN_HOME)/include

#LDFLAGS = -Wl,-rpath,$(TOOLCHAIN_HOME)/lib

BIN=realpath

all: ${BIN}

${BIN}:
	${CC} ${CFLAGS} -o ${BIN} realpath.c
	${STRIP} ${BIN}
	${STRIP} -R .comment -R .note -R .note.ABI-tag ${BIN}

clean:
	rm ${BIN}
