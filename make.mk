ifndef IMPORT_ZLIB
IMPORT_ZLIB := 1

BLDLIBS += $(BASE)/import/zlib

CPPFLAGS += -I $(BASE)/import/zlib/include

ifeq ($(IPP),on)
include $(BASE)/import/ipp/make.mk
endif

ifeq ($(BLDTYPE),debug)
SLIBS := -limportzlibd $(SLIBS)
else
SLIBS := -limportzlib $(SLIBS)
endif

endif
