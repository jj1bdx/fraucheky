# Fraucheky make rules.

ifeq ($(BFDNAME_OBJ),)
BFDNAME_OBJ = elf32-little
endif

ifneq ($(BFDARCH),)
ARG_BFDARCH = -B $(BFDARCH)
endif

OBJCOPY_BINARY_DATA=$(OBJCOPY) -I binary -O $(BFDNAME_OBJ) $(ARG_BFDARCH) \
	--rename-section .data=.rodata.file,alloc,load,readonly,data,contents

$(BUILDDIR)/COPYING.o: COPYING
	$(OBJCOPY_BINARY_DATA) $< $@

$(BUILDDIR)/README.o: README
	$(OBJCOPY_BINARY_DATA) $< $@

$(BUILDDIR)/INDEX.o: INDEX
	$(OBJCOPY_BINARY_DATA) $< $@

distclean::
	-rm -f README INDEX COPYING \
	       fraucheky-vid-pid-ver.c.inc fraucheky-usb-strings.c.inc disk-on-rom.h
