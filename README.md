# Linux UIO interrupt example

Simple example of how to use the Linux UIO driver in userspace by handling a SIGIO

# Building and running

To build call `make all`. Make sure you have `linux-headers` installed.

To run the example load the kernel module (depends on the uio driver being loaded) and run the userspace binary

```sh
# modeprobe uio
# insmod uio_interrupt_emulator
# ./userspace_app
```
