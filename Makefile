CPPFLAGS=-D_LARGEFILE64_SOURCE
CFLAGS=-Os

all: copy_usb autostart

clean:
	rm -f copy_usb autostart
