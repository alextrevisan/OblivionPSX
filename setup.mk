PREFIX = mipsel-none-elf-

PSN00B_LIB	= /usr/local/lib/libpsn00b
PSN00B_INCLUDE	= /usr/local/include/libpsn00b
GCC_BASE	= /usr/local/mipsel-none-elf



LIBDIRS		= -L$(PSN00B_LIB)/release
INCLUDE	 	= -I$(PSN00B_INCLUDE)

ELF2X		= /usr/local/bin/elf2x
MKPSXISO    = /usr/local/bin/mkpsxiso


GCC_BIN		= $(GCC_BASE)/bin/

CC			= $(GCC_BIN)$(PREFIX)gcc
CXX			= $(GCC_BIN)$(PREFIX)g++
AS			= $(GCC_BIN)$(PREFIX)as
AR			= $(GCC_BIN)$(PREFIX)ar
RANLIB		= $(GCC_BIN)$(PREFIX)ranlib
LD			= $(GCC_BIN)$(PREFIX)ld

#TOOLS

MKPSXISO_XML= iso_build/OblivionPSX.xml
BIN_FOLDER  = bin
ISO_FOLDER  = iso