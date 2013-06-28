#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/ipu.h>

#define WIDTH 256
#define HEIGHT 256
#define QUARTER_HEIGHT (HEIGHT / 4)

int main(int argc, char** argv)
{
    int ipu_handle;
    int output_handle;
    void *input_dma, *overlay_dma, *output_dma;
    void *input_mem, *overlay_mem, *output_mem;
    struct ipu_task task;
    uint8_t *pixel;
    int x, y;

    memset(&task, 0, sizeof(task));

    if (argc != 2) {
        fprintf(stderr, "usage fsl-ipu-test <output.raw>\n");
        return 1;
    }

    output_handle = open(argv[1], O_RDWR | O_CREAT, 0644);
    if (output_handle == -1) {
        fprintf(stderr, "failed to open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    ipu_handle = open("/dev/mxc_ipu", O_RDWR);
    if (ipu_handle == -1) {
        perror("failed to open /dev/mxc_ipu");
        return 1;
    }

    input_dma = WIDTH * HEIGHT * 4;
    overlay_dma = WIDTH * HEIGHT * 4;
    output_dma = WIDTH * HEIGHT * 4;
    
    if (ioctl(ipu_handle, IPU_ALLOC, &input_dma) == -1) {
        perror("failed to IPU_ALLOC input_dma");
        return 1;
    }

    if (ioctl(ipu_handle, IPU_ALLOC, &overlay_dma) == -1) {
        perror("failed to IPU_ALLOC overlay_dma");
        return 1;
    }

    if (ioctl(ipu_handle, IPU_ALLOC, &output_dma) == -1) {
        perror("failed to IPU_ALLOC output_dma");
        return 1;
    }

    input_mem = mmap(0,
                     WIDTH * HEIGHT * 4,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     ipu_handle,
                     (off_t) input_dma);
    if (input_mem == MAP_FAILED) {
        perror("failed to mmap input_mem");
        return 1;
    }

    overlay_mem = mmap(0,
                       WIDTH * HEIGHT * 4,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       ipu_handle,
                       (off_t) overlay_dma);
    if (overlay_mem == MAP_FAILED) {
        perror("failed to mmap overlay_mem");
        return 1;
    }

    output_mem = mmap(0,
                      WIDTH * HEIGHT * 4,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED,
                      ipu_handle,
                      (off_t) output_dma);
    if (output_mem == MAP_FAILED) {
        perror("failed to mmap output_mem");
        return 1;
    }

    pixel = input_mem;

    for (y = 0; y < QUARTER_HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            *pixel = 0x00; pixel++;
            *pixel = 0xFF; pixel++;
            *pixel = 0x00; pixel++;
            *pixel = 0xFF; pixel++;
        }
    }

    for (y = QUARTER_HEIGHT; y < QUARTER_HEIGHT * 2; y++) {
        for (x = 0; x < WIDTH; x++) {
            *pixel = 0xFF; pixel++;
            *pixel = 0xFF; pixel++;
            *pixel = 0x00; pixel++;
            *pixel = 0xFF; pixel++;
        }
    }

    for (y = QUARTER_HEIGHT * 2; y < QUARTER_HEIGHT * 3; y++) {
        for (x = 0; x < WIDTH; x++) {
            *pixel = 0xFF; pixel++;
            *pixel = 0x00; pixel++;
            *pixel = 0x00; pixel++;
            *pixel = 0xFF; pixel++;
        }
    }

    for (y = QUARTER_HEIGHT * 3; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            *pixel = 0x00; pixel++;
            *pixel = 0x00; pixel++;
            *pixel = 0xFF; pixel++;
            *pixel = 0xFF; pixel++;
        }
    }

    memset(overlay_mem, 0, WIDTH * HEIGHT * 4);
    memset(output_mem, 0, WIDTH * HEIGHT * 4);

    pixel = overlay_mem;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            if (x < WIDTH / 2) {
                *pixel = 0xFF; pixel++;
                *pixel = 0x00; pixel++;
                *pixel = 0x00; pixel++;
                *pixel = 0x7F; pixel++;
            } else {
                *pixel = 0xFF; pixel++;
                *pixel = 0x00; pixel++;
                *pixel = 0x00; pixel++;
                *pixel = 0x00; pixel++;
            }
        }
    }

    task.input.width = WIDTH;
    task.input.height = HEIGHT;
    task.input.format = IPU_PIX_FMT_RGBA32;
    task.input.paddr = (dma_addr_t) input_dma;

    task.overlay.width = WIDTH;
    task.overlay.height = HEIGHT;
    task.overlay.format = IPU_PIX_FMT_RGBA32;
    task.overlay.paddr = (dma_addr_t) overlay_dma;
    task.overlay_en = 1;

    task.output.width = WIDTH;
    task.output.height = HEIGHT;
    task.output.format = IPU_PIX_FMT_RGBA32;
    task.output.paddr = (dma_addr_t) output_dma;

    if (ioctl(ipu_handle, IPU_QUEUE_TASK, &task) == -1) {
        perror("IPU_QUEUE_TASK failed");
        return 1;
    }
    
    /* IPU Does not correctly set the alpha channel on the output. */
    pixel = output_mem;
    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            pixel++; pixel++; pixel++;
            *pixel = 0xFF; pixel++;
        }
    }

    if (write(output_handle, output_mem, WIDTH * HEIGHT * 4) == -1) {
        perror("failed to write image to output file");
        return 1;
    }

    printf("completed ipu overlay output to raw image %s\n", argv[1]);

    munmap(input_mem, WIDTH * HEIGHT * 4);
    munmap(overlay_mem, WIDTH * HEIGHT * 4);
    munmap(output_mem, WIDTH * HEIGHT * 4);

    ioctl(ipu_handle, IPU_FREE, &input_dma);
    ioctl(ipu_handle, IPU_FREE, &overlay_dma);
    ioctl(ipu_handle, IPU_FREE, &output_dma);

    close(output_handle);
    close(ipu_handle);

    return 0;
}

