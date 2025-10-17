obj-m += uio_interrupt_emulator.o

app: userspace_app.c
	$(CC) userspace_app.c -o userspace_app

all: app
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f userspace_app
