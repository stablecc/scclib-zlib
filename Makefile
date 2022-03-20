BASE = ../..

CPPFLAGS += -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN -DWITH_IPP -I src

NAME = importzlib

OBJZ = src/adler32.c src/crc32.c src/deflate.c src/infback.c src/inffast.c src/inflate.c src/inftrees.c src/trees.c src/zutil.c
OBJG = src/compress.c src/uncompr.c src/gzclose.c src/gzlib.c src/gzread.c src/gzwrite.c

SRCS = $(OBJZ) $(OBJG)

include $(BASE)/import/ipp/make.mk
include $(BASE)/make/sl.mk
