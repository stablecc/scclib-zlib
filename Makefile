BASE = ../..

CPPFLAGS += -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN -I zlib-1.2.11-ipp
ifeq ($(IPP),on)
CPPFLAGS += -DWITH_IPP
endif

NAME = importzlib

OBJZ = zlib-1.2.11-ipp/adler32.c zlib-1.2.11-ipp/crc32.c zlib-1.2.11-ipp/deflate.c zlib-1.2.11-ipp/infback.c \
	zlib-1.2.11-ipp/inffast.c zlib-1.2.11-ipp/inflate.c zlib-1.2.11-ipp/inftrees.c zlib-1.2.11-ipp/trees.c zlib-1.2.11-ipp/zutil.c
OBJG = zlib-1.2.11-ipp/compress.c zlib-1.2.11-ipp/uncompr.c zlib-1.2.11-ipp/gzclose.c zlib-1.2.11-ipp/gzlib.c \
	zlib-1.2.11-ipp/gzread.c zlib-1.2.11-ipp/gzwrite.c

SRCS = $(OBJZ) $(OBJG)

ifeq ($(IPP),on)
include $(BASE)/import/ipp/make.mk
endif
include $(BASE)/make/sl.mk
