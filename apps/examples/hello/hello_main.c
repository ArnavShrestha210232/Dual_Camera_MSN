#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdbool.h>
#include <pthread.h>
#include "ring_buffer.h"

#define MAX_FRAME_SIZE 16000
#define UART4_DEV "/dev/ttyS1"

static uint8_t  g_frame_buf[NUM_CAMERAS][MAX_FRAME_SIZE];
static uint32_t g_frame_len[NUM_CAMERAS];

static int configure_uart4(const char *dev_path)
{
    int fd = open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return -1;

    struct termios t;
    tcgetattr(fd, &t);
    cfsetispeed(&t, B115200);
    cfsetospeed(&t, B115200);
    t.c_cflag = CS8 | CREAD | CLOCAL;
    t.c_iflag = 0;
    t.c_oflag = 0;
    t.c_lflag = 0;
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &t);

    return fd;
}

int hello_main(int argc, char *argv[])
{
    struct image_chunk_s chunk;
    memset(g_frame_len, 0, sizeof(g_frame_len));

    int uart4_fd = configure_uart4(UART4_DEV);

    pthread_mutex_lock(&g_console_mutex);
    printf("[SUBSCRIBER] Forwarding full frame JPEG data to UART4 and UART2...\n");
    fflush(stdout);
    pthread_mutex_unlock(&g_console_mutex);

    bool cam_finished[NUM_CAMERAS] = {false};
    int cameras_completed = 0;

    while (cameras_completed < NUM_CAMERAS)
    {
        for (int cam = 0; cam < NUM_CAMERAS; cam++)
        {
            if (cam_finished[cam]) continue;

            while (rb_read(cam, &chunk) == 0)
            {
                int cam_id = cam + 1;

                if (chunk.is_meta)
                {
                    g_frame_len[cam] = 0;

                    pthread_mutex_lock(&g_console_mutex);
                    printf("[CAM %d META] %.*s\n", cam_id, chunk.len, chunk.data);
                    fflush(stdout);
                    pthread_mutex_unlock(&g_console_mutex);

                    if (uart4_fd >= 0) {
                        ssize_t w = 0;
                        while (w < chunk.len) {
                            ssize_t ret = write(uart4_fd, &chunk.data[w], chunk.len - w);
                            if (ret > 0) w += ret;
                            else usleep(1000);
                        }
                    }
                    continue;
                }

                if (g_frame_len[cam] + chunk.len <= MAX_FRAME_SIZE)
                {
                    memcpy(&g_frame_buf[cam][g_frame_len[cam]], chunk.data, chunk.len);
                    g_frame_len[cam] += chunk.len;
                }

                if (chunk.is_last)
                {
                    /* 1. Transmit frame buffer to UART4 reliably */
                    if (uart4_fd >= 0)
                    {
                        ssize_t total_written = 0;
                        while (total_written < g_frame_len[cam])
                        {
                            ssize_t ret = write(uart4_fd, &g_frame_buf[cam][total_written], g_frame_len[cam] - total_written);
                            if (ret > 0)
                            {
                                total_written += ret;
                            }
                            else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                            {
                                usleep(1000);
                            }
                        }

                        /* 2. Check if this is the final camera image and append 4x FFD9 */
                        if (cameras_completed + 1 == NUM_CAMERAS)
                        {
                            uint8_t extra_eoi[] = {0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9};
                            write(uart4_fd, extra_eoi, sizeof(extra_eoi));
                        }
                    }

                    /* 3. Print Hex Dump atomically under console mutex */
                    pthread_mutex_lock(&g_console_mutex);

                    printf("[CAM %d FULL JPEG] Reassembled %lu Bytes -> Transmitting to UART4 and UART2...\n",
                           cam_id, (unsigned long)g_frame_len[cam]);
                    printf("[CAM %d HEX DUMP START]\n", cam_id);
                    for (uint32_t i = 0; i < g_frame_len[cam]; i++)
                    {
                        printf("%02X", g_frame_buf[cam][i]);
                    }
                    printf("\n[CAM %d HEX DUMP END]\n\n", cam_id);
                    fflush(stdout);

                    pthread_mutex_unlock(&g_console_mutex);

                    g_frame_len[cam] = 0;
                    cam_finished[cam] = true;
                    cameras_completed++;
                    break;
                }
            }
        }
        usleep(10000);
    }

    if (uart4_fd >= 0) close(uart4_fd);

    pthread_mutex_lock(&g_console_mutex);
    printf("[SUBSCRIBER] All cameras processed. Exiting cleanly.\n");
    fflush(stdout);
    pthread_mutex_unlock(&g_console_mutex);

    return 0;
}