CPPFLAGS=-D_LARGEFILE64_SOURCE
CFLAGS=-Os -g

TARGETS=copy_usb autostart autostart-gui

.PHONY: all
all: $(TARGETS)

autostart-gui: UserInterface.o
autostart-gui: LDLIBS=-lfltk

.PHONY: clean
clean:
	rm -f *.o $(TARGETS)
