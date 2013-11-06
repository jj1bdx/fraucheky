# Fraucheky make rules.

CSRC += $(FRAUCHEKY)/fraucheky.c $(FRAUCHEKY)/usb-msc.c \
	$(FRAUCHEKY)/disk-on-rom.c

OBJS_ADD += $(BUILDDIR)/COPYING.o $(BUILDDIR)/README.o $(BUILDDIR)/INDEX.o
