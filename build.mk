# Fraucheky make rules.

OBJCOPY_BINARY_DATA=$(OBJCOPY) -I binary -O elf32-little \
	--rename-section .data=.rodata.file,alloc,load,readonly,data,contents

$(BUILDDIR)/COPYING.o: COPYING
	$(OBJCOPY_BINARY_DATA) $< $@

$(BUILDDIR)/README.o: README
	$(OBJCOPY_BINARY_DATA) $< $@

$(BUILDDIR)/INDEX.o: INDEX
	$(OBJCOPY_BINARY_DATA) $< $@

distclean::
	-rm -f README INDEX \
	       fraucheky-vid-pid-ver.c.inc fraucheky-usb-strings.c.inc 
