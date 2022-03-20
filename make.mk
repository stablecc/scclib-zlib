ifndef IMPORT_ZLIB
IMPORT_ZLIB := 1

DOXY_SRC += import/zlib

BLDLIBS += $(BASE)/import/zlib

CPPFLAGS += -I $(BASE)/import/zlib

include $(BASE)/import/ipp/make.mk

ifeq ($(BLDTYPE),debug)
SLIBS := -limportzlibd $(SLIBS)
else
SLIBS := -limportzlib $(SLIBS)
endif

endif
