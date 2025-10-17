#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define UIO_DEVICE "/dev/uio0"

static int uio_fd;
static volatile sig_atomic_t irq_count;
static volatile sig_atomic_t do_work;

void interrupt_handler(int signum) {
    if (signum == SIGIO) {
        uint32_t tmp_count;
        ssize_t read_bytes;
        read_bytes = read(uio_fd, &tmp_count, sizeof(irq_count));
        if (read_bytes < 0) {
            return;
        }
        irq_count = tmp_count;

        // only set a work flag to not hold the interrupt
        do_work = 1;

        uint32_t enable = 1;
        if (write(uio_fd, &enable, sizeof(enable)) < 0) {
            perror("Failed to re-enable interrupt");
        }
    }
}

int main() {
    struct sigaction action;
    int flags;

    uio_fd = open(UIO_DEVICE, O_RDWR);
    if (uio_fd < 0) {
        perror("Failed to open: " UIO_DEVICE);
        return EXIT_FAILURE;
    }

    memset(&action, 0, sizeof(action));
    action.sa_handler = interrupt_handler;

    if (sigaction(SIGIO, &action, NULL) < 0) {
        perror("Failed to register signal handler");
        close(uio_fd);
        return EXIT_FAILURE;
    }

    if (fcntl(uio_fd, F_SETOWN, getpid()) < 0) {
        perror("Failed to set owner");
        close(uio_fd);
        return EXIT_FAILURE;
    }

    flags = fcntl(uio_fd, F_GETFL);
    if (fcntl(uio_fd, F_SETFL, flags | O_ASYNC) < 0) {
        perror("Failed to enable async I/O");
        close(uio_fd);
        return EXIT_FAILURE;
    }

    uint32_t enable = 1;
    write(uio_fd, &enable, sizeof(enable));

    printf("Setup complete. Waiting for interrupts. Press CTRL+C to exit\n");

    while (1) {
        pause();
        if (do_work) {
            do_work = 0;

            printf("Received interrupt, ready for work! irq_count: %d\n", irq_count);
        }
    }

    close(uio_fd);
    return EXIT_SUCCESS;
}
