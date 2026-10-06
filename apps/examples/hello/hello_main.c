// #include <stdio.h>
// #include <stdint.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <termios.h>
// #include <stdbool.h>
// #include <pthread.h>
// #include "ring_buffer.h"

// #define MAX_FRAME_SIZE 16000
// #define UART4_DEV "/dev/ttyS1"

// static uint8_t  g_frame_buf[NUM_CAMERAS][MAX_FRAME_SIZE];
// static uint32_t g_frame_len[NUM_CAMERAS];

// static int configure_uart4(const char *dev_path)
// {
//     int fd = open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
//     if (fd < 0) return -1;

//     struct termios t;
//     tcgetattr(fd, &t);
//     cfsetispeed(&t, B115200);
//     cfsetospeed(&t, B115200);
//     t.c_cflag = CS8 | CREAD | CLOCAL;
//     t.c_iflag = 0;
//     t.c_oflag = 0;
//     t.c_lflag = 0;
//     t.c_cc[VMIN] = 0;
//     t.c_cc[VTIME] = 0;
//     tcsetattr(fd, TCSANOW, &t);

//     return fd;
// }

// int hello_main(int argc, char *argv[])
// {
//     struct image_chunk_s chunk;
//     memset(g_frame_len, 0, sizeof(g_frame_len));

//     int uart4_fd = configure_uart4(UART4_DEV);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] Forwarding full frame JPEG data to UART4 and UART2...\n");
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     bool cam_finished[NUM_CAMERAS] = {false};
//     int cameras_completed = 0;

//     while (cameras_completed < NUM_CAMERAS)
//     {
//         for (int cam = 0; cam < NUM_CAMERAS; cam++)
//         {
//             if (cam_finished[cam]) continue;

//             while (rb_read(cam, &chunk) == 0)
//             {
//                 int cam_id = cam + 1;

//                 if (chunk.is_meta)
//                 {
//                     g_frame_len[cam] = 0;

//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[CAM %d META] %.*s\n", cam_id, chunk.len, chunk.data);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);

//                     if (uart4_fd >= 0) {
//                         ssize_t w = 0;
//                         while (w < chunk.len) {
//                             ssize_t ret = write(uart4_fd, &chunk.data[w], chunk.len - w);
//                             if (ret > 0) w += ret;
//                             else usleep(1000);
//                         }
//                     }
//                     continue;
//                 }

//                 if (g_frame_len[cam] + chunk.len <= MAX_FRAME_SIZE)
//                 {
//                     memcpy(&g_frame_buf[cam][g_frame_len[cam]], chunk.data, chunk.len);
//                     g_frame_len[cam] += chunk.len;
//                 }

//                 if (chunk.is_last)
//                 {
//                     /* 1. Transmit frame buffer to UART4 reliably */
//                     if (uart4_fd >= 0)
//                     {
//                         ssize_t total_written = 0;
//                         while (total_written < g_frame_len[cam])
//                         {
//                             ssize_t ret = write(uart4_fd, &g_frame_buf[cam][total_written], g_frame_len[cam] - total_written);
//                             if (ret > 0)
//                             {
//                                 total_written += ret;
//                             }
//                             else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
//                             {
//                                 usleep(1000);
//                             }
//                         }

//                         /* 2. Check if this is the final camera image and append 4x FFD9 */
//                         if (cameras_completed + 1 == NUM_CAMERAS)
//                         {
//                             uint8_t extra_eoi[] = {0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9};
//                             write(uart4_fd, extra_eoi, sizeof(extra_eoi));
//                         }
//                     }

//                     /* 3. Print Hex Dump atomically under console mutex */
//                     pthread_mutex_lock(&g_console_mutex);

//                     printf("[CAM %d FULL JPEG] Reassembled %lu Bytes -> Transmitting to UART4 and UART2...\n",
//                            cam_id, (unsigned long)g_frame_len[cam]);
//                     printf("[CAM %d HEX DUMP START]\n", cam_id);
//                     for (uint32_t i = 0; i < g_frame_len[cam]; i++)
//                     {
//                         printf("%02X", g_frame_buf[cam][i]);
//                     }
//                     printf("\n[CAM %d HEX DUMP END]\n\n", cam_id);
//                     fflush(stdout);

//                     pthread_mutex_unlock(&g_console_mutex);

//                     g_frame_len[cam] = 0;
//                     cam_finished[cam] = true;
//                     cameras_completed++;
//                     break;
//                 }
//             }
//         }
//         usleep(10000);
//     }

//     if (uart4_fd >= 0) close(uart4_fd);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] All cameras processed. Exiting cleanly.\n");
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     return 0;
// }
// #include <stdio.h>
// #include <stdint.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <termios.h>
// #include <stdbool.h>
// #include <pthread.h>
// #include "ring_buffer.h"

// #define MAX_FRAME_SIZE 16000
// #define UART4_DEV "/dev/ttyS1"

// static uint8_t  g_frame_buf[NUM_CAMERAS][MAX_FRAME_SIZE];
// static uint32_t g_frame_len[NUM_CAMERAS];

// static int configure_uart4(const char *dev_path)
// {
//     int fd = open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
//     if (fd < 0) return -1;

//     struct termios t;
//     tcgetattr(fd, &t);
//     cfsetispeed(&t, B115200);
//     cfsetospeed(&t, B115200);
//     t.c_cflag = CS8 | CREAD | CLOCAL;
//     t.c_iflag = 0;
//     t.c_oflag = 0;
//     t.c_lflag = 0;
//     t.c_cc[VMIN] = 0;
//     t.c_cc[VTIME] = 0;
//     tcsetattr(fd, TCSANOW, &t);

//     return fd;
// }

// int hello_main(int argc, char *argv[])
// {
//     struct image_chunk_s chunk;
//     memset(g_frame_len, 0, sizeof(g_frame_len));

//     int uart4_fd = configure_uart4(UART4_DEV);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] Forwarding full frame JPEG data to UART4 and UART2...\n");
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     bool cam_finished[NUM_CAMERAS] = {false};
//     int cameras_completed = 0;

//     while (cameras_completed < NUM_CAMERAS)
//     {
//         for (int cam = 0; cam < NUM_CAMERAS; cam++)
//         {
//             if (cam_finished[cam]) continue;

//             while (rb_read(cam, &chunk) == 0)
//             {
//                 int cam_id = cam + 1;

//                 if (chunk.is_meta)
//                 {
//                     g_frame_len[cam] = 0;

//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[CAM %d META] %.*s\n", cam_id, chunk.len, chunk.data);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);

//                     if (uart4_fd >= 0) {
//                         ssize_t w = 0;
//                         while (w < chunk.len) {
//                             ssize_t ret = write(uart4_fd, &chunk.data[w], chunk.len - w);
//                             if (ret > 0) w += ret;
//                             else usleep(1000);
//                         }
//                         tcdrain(uart4_fd);
//                     }
//                     continue;
//                 }

//                 if (g_frame_len[cam] + chunk.len <= MAX_FRAME_SIZE)
//                 {
//                     memcpy(&g_frame_buf[cam][g_frame_len[cam]], chunk.data, chunk.len);
//                     g_frame_len[cam] += chunk.len;
//                 }

//                 if (chunk.is_last)
//                 {
//                     /* 1. Transmit frame buffer to UART4 reliably */
//                     if (uart4_fd >= 0)
//                     {
//                         ssize_t total_written = 0;
//                         while (total_written < g_frame_len[cam])
//                         {
//                             ssize_t ret = write(uart4_fd, &g_frame_buf[cam][total_written], g_frame_len[cam] - total_written);
//                             if (ret > 0)
//                             {
//                                 total_written += ret;
//                             }
//                             else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
//                             {
//                                 usleep(1000);
//                             }
//                         }

//                         /* 2. Check if this is the final camera image and append 4x FFD9 */
//                         if (cameras_completed + 1 == NUM_CAMERAS)
//                         {
//                             uint8_t extra_eoi[] = {0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9};
//                             write(uart4_fd, extra_eoi, sizeof(extra_eoi));
//                         }

//                         /* Wait until all data bits physically clear the UART pins */
//                         tcdrain(uart4_fd);
//                     }

//                     /* 3. Print Hex Dump atomically under console mutex */
//                     pthread_mutex_lock(&g_console_mutex);

//                     printf("[CAM %d FULL JPEG] Reassembled %lu Bytes -> Transmitting to UART4 and UART2...\n",
//                            cam_id, (unsigned long)g_frame_len[cam]);
//                     printf("[CAM %d HEX DUMP START]\n", cam_id);
//                     for (uint32_t i = 0; i < g_frame_len[cam]; i++)
//                     {
//                         printf("%02X", g_frame_buf[cam][i]);
//                     }
//                     printf("\n[CAM %d HEX DUMP END]\n\n", cam_id);
//                     fflush(stdout);

//                     pthread_mutex_unlock(&g_console_mutex);

//                     g_frame_len[cam] = 0;
//                     cam_finished[cam] = true;
//                     cameras_completed++;

//                     /* 4. Pause 2 seconds to allow OBC (STM32WL) to process CAM1 and re-arm RX buffer */
//                     if (cameras_completed < NUM_CAMERAS)
//                     {
//                         pthread_mutex_lock(&g_console_mutex);
//                         printf("[SUBSCRIBER] CAM %d sent. Delaying 5s for OBC before CAM %d...\n", 
//                                cam_id, cam_id + 1);
//                         fflush(stdout);
//                         pthread_mutex_unlock(&g_console_mutex);

//                         sleep(5);
//                     }

//                     break;
//                 }
//             }
//         }
//         usleep(10000);  
//     }

//     if (uart4_fd >= 0) close(uart4_fd);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] All cameras processed. Exiting cleanly.\n");
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     return 0;
// // }
// #include <stdio.h>
// #include <stdint.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <termios.h>
// #include <stdbool.h>
// #include <pthread.h>
// #include "ring_buffer.h"

// #define MAX_FRAME_SIZE 16000
// #define UART4_DEV "/dev/ttyS1"

// static uint8_t  g_frame_buf[NUM_CAMERAS][MAX_FRAME_SIZE];
// static uint32_t g_frame_len[NUM_CAMERAS];

// static int configure_uart4(const char *dev_path)
// {
//     int fd = open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
//     if (fd < 0) return -1;

//     struct termios t;
//     tcgetattr(fd, &t);
//     cfsetispeed(&t, B115200);
//     cfsetospeed(&t, B115200);
//     t.c_cflag = CS8 | CREAD | CLOCAL;
//     t.c_iflag = 0;
//     t.c_oflag = 0;
//     t.c_lflag = 0;
//     t.c_cc[VMIN] = 0;
//     t.c_cc[VTIME] = 0;
//     tcsetattr(fd, TCSANOW, &t);

//     return fd;
// }

// int hello_main(int argc, char *argv[])
// {
//     struct image_chunk_s chunk;
//     memset(g_frame_len, 0, sizeof(g_frame_len));

//     int uart4_fd = configure_uart4(UART4_DEV);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] Forwarding full frame JPEG data to UART4 and UART2...\n");
//     if (uart4_fd < 0)
//     {
//         printf("[SUBSCRIBER ERROR] Failed to open UART4 (%s): %s\n", UART4_DEV, strerror(errno));
//     }
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     bool frame_ready[NUM_CAMERAS] = {false};
//     bool frame_sent[NUM_CAMERAS]  = {false};
//     int cameras_sent = 0;

//     while (cameras_sent < NUM_CAMERAS)
//     {
//         /* --- STEP 1: Drain ring buffers for ANY active camera --- */
//         for (int cam = 0; cam < NUM_CAMERAS; cam++)
//         {
//             if (frame_ready[cam] || frame_sent[cam]) continue;

//             while (rb_read(cam, &chunk) == 0)
//             {
//                 int cam_id = cam + 1;

//                 if (chunk.is_meta)
//                 {
//                     g_frame_len[cam] = 0;

//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[CAM %d META] %.*s\n", cam_id, chunk.len, chunk.data);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);
//                     continue;
//                 }

//                 if (g_frame_len[cam] + chunk.len <= MAX_FRAME_SIZE)
//                 {
//                     memcpy(&g_frame_buf[cam][g_frame_len[cam]], chunk.data, chunk.len);
//                     g_frame_len[cam] += chunk.len;
//                 }

//                 if (chunk.is_last)
//                 {
//                     frame_ready[cam] = true;
//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[CAM %d READ COMPLETE] Reassembled %lu bytes in memory.\n", 
//                            cam_id, (unsigned long)g_frame_len[cam]);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);
//                     break;
//                 }
//             }
//         }

//         /* --- STEP 2: Transmit completed frames out over UART strictly in order --- */
//         for (int cam = 0; cam < NUM_CAMERAS; cam++)
//         {
//             /* Enforce sequence: Do not send Camera 2 until Camera 1 is sent */
//             if (cam > 0 && !frame_sent[cam - 1]) break;

//             if (frame_ready[cam] && !frame_sent[cam])
//             {
//                 int cam_id = cam + 1;

//                 /* 1. Print Hex Dump to Console */
//                 pthread_mutex_lock(&g_console_mutex);
//                 printf("[CAM %d FULL JPEG] Reassembled %lu Bytes -> Transmitting to UART4...\n",
//                        cam_id, (unsigned long)g_frame_len[cam]);
//                 printf("[CAM %d HEX DUMP START]\n", cam_id);
//                 for (uint32_t i = 0; i < g_frame_len[cam]; i++)
//                 {
//                     printf("%02X", g_frame_buf[cam][i]);
//                 }
//                 printf("\n[CAM %d HEX DUMP END]\n\n", cam_id);
//                 fflush(stdout);
//                 pthread_mutex_unlock(&g_console_mutex);

//                 /* 2. Write Frame to UART4 */
//                 if (uart4_fd >= 0)
//                 {
//                     ssize_t total_written = 0;
//                     while (total_written < g_frame_len[cam])
//                     {
//                         ssize_t ret = write(uart4_fd, &g_frame_buf[cam][total_written], g_frame_len[cam] - total_written);
//                         if (ret > 0)
//                         {
//                             total_written += ret;
//                         }
//                         else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
//                         {
//                             usleep(1000);
//                         }
//                         else
//                         {
//                             break; /* Hard write error */
//                         }
//                     }

//                     /* Append extra FFD9 termination marker on the final camera */
//                     if (cam == NUM_CAMERAS - 1)
//                     {
//                         uint8_t extra_eoi[] = {0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9};
//                         write(uart4_fd, extra_eoi, sizeof(extra_eoi));
//                     }

//                     tcdrain(uart4_fd);
//                 }

//                 frame_sent[cam] = true;
//                 cameras_sent++;

//                 /* 3. Pause 5s for OBC processing before transmitting next camera */
//                 if (cameras_sent < NUM_CAMERAS)
//                 {
//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[SUBSCRIBER] CAM %d sent. Delaying 5s for OBC before CAM %d...\n", 
//                            cam_id, cam_id + 1);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);

//                     sleep(5);
//                 }
//             }
//         }

//         usleep(10000); /* Sleep 10ms between polling cycles */
//     }

//     if (uart4_fd >= 0) close(uart4_fd);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] All cameras processed. Exiting cleanly.\n");
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     return 0;
// // }
// #include <stdio.h>
// #include <stdint.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <termios.h>
// #include <stdbool.h>
// #include <pthread.h>
// #include <time.h>
// #include "ring_buffer.h"

// #define MAX_FRAME_SIZE 16000
// #define UART4_DEV "/dev/ttyS1"

// static uint8_t  g_frame_buf[NUM_CAMERAS][MAX_FRAME_SIZE];
// static uint32_t g_frame_len[NUM_CAMERAS];

// static int configure_uart4(const char *dev_path)
// {
//     int fd = open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
//     if (fd < 0) return -1;

//     struct termios t;
//     tcgetattr(fd, &t);
//     cfsetispeed(&t, B115200);
//     cfsetospeed(&t, B115200);
//     t.c_cflag = CS8 | CREAD | CLOCAL;
//     t.c_iflag = 0;
//     t.c_oflag = 0;
//     t.c_lflag = 0;
//     t.c_cc[VMIN] = 0;
//     t.c_cc[VTIME] = 0;
//     tcsetattr(fd, TCSANOW, &t);

//     return fd;
// }

// int hello_main(int argc, char *argv[])
// {
//     struct image_chunk_s chunk;
//     memset(g_frame_len, 0, sizeof(g_frame_len));

//     int uart4_fd = configure_uart4(UART4_DEV);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] Forwarding full frame JPEG data to UART4...\n");
//     if (uart4_fd < 0)
//     {
//         printf("[SUBSCRIBER ERROR] Failed to open UART4 (%s): %s\n", UART4_DEV, strerror(errno));
//     }
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     bool frame_ready[NUM_CAMERAS] = {false};
//     bool frame_sent[NUM_CAMERAS]  = {false};
//     int cameras_sent = 0;

//     while (cameras_sent < NUM_CAMERAS)
//     {
//         /* --- STEP 1: Drain ring buffers for ANY active camera --- */
//         for (int cam = 0; cam < NUM_CAMERAS; cam++)
//         {
//             if (frame_ready[cam] || frame_sent[cam]) continue;

//             while (rb_read(cam, &chunk) == 0)
//             {
//                 int cam_id = cam + 1;

//                 if (chunk.is_meta)
//                 {
//                     g_frame_len[cam] = 0;

//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[CAM %d META] %.*s\n", cam_id, chunk.len, chunk.data);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);
//                     continue;
//                 }

//                 if (g_frame_len[cam] + chunk.len <= MAX_FRAME_SIZE)
//                 {
//                     memcpy(&g_frame_buf[cam][g_frame_len[cam]], chunk.data, chunk.len);
//                     g_frame_len[cam] += chunk.len;
//                 }

//                 if (chunk.is_last)
//                 {
//                     frame_ready[cam] = true;
//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[CAM %d READ COMPLETE] Reassembled %lu bytes in memory.\n", 
//                            cam_id, (unsigned long)g_frame_len[cam]);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);
//                     break;
//                 }
//             }
//         }

//         /* --- STEP 2: Transmit completed frames out over UART strictly in order --- */
//         for (int cam = 0; cam < NUM_CAMERAS; cam++)
//         {
//             /* Enforce sequence: Do not send Camera 2 until Camera 1 is sent */
//             if (cam > 0 && !frame_sent[cam - 1]) break;

//             if (frame_ready[cam] && !frame_sent[cam])
//             {
//                 int cam_id = cam + 1;

//                 /* 1. Pre-transmission 5-Second Delay with Active Timestamp Heartbeats */
//                 pthread_mutex_lock(&g_console_mutex);
//                 printf("[CAM %d] Image captured! Holding transmission for 5 seconds and streaming timestamps...\n", cam_id);
//                 fflush(stdout);
//                 pthread_mutex_unlock(&g_console_mutex);

//                 for (int sec = 1; sec <= 5; sec++)
//                 {
//                     struct timespec ts;
//                     clock_gettime(CLOCK_REALTIME, &ts);

//                     char ts_buf[64];
//                     int ts_len = snprintf(ts_buf, sizeof(ts_buf),
//                                           "[TS CAM%d] Sec:%d Timestamp:%ld.%03ld\r\n",
//                                           cam_id, sec, (long)ts.tv_sec, ts.tv_nsec / 1000000);

//                     /* Send timestamp over UART4 */
//                     if (uart4_fd >= 0)
//                     {
//                         write(uart4_fd, ts_buf, ts_len);
//                         tcdrain(uart4_fd);
//                     }

//                     /* Print timestamp to Console */
//                     pthread_mutex_lock(&g_console_mutex);
//                     printf("[UART4 TX] %s", ts_buf);
//                     fflush(stdout);
//                     pthread_mutex_unlock(&g_console_mutex);

//                     sleep(1);
//                 }

//                 /* 2. Print Hex Dump to Console */
//                 pthread_mutex_lock(&g_console_mutex);
//                 printf("[CAM %d FULL JPEG] 5s delay elapsed -> Transmitting %lu Bytes to UART4...\n",
//                        cam_id, (unsigned long)g_frame_len[cam]);
//                 printf("[CAM %d HEX DUMP START]\n", cam_id);
//                 for (uint32_t i = 0; i < g_frame_len[cam]; i++)
//                 {
//                     printf("%02X", g_frame_buf[cam][i]);
//                 }
//                 printf("\n[CAM %d HEX DUMP END]\n\n", cam_id);
//                 fflush(stdout);
//                 pthread_mutex_unlock(&g_console_mutex);

//                 /* 3. Write Full JPEG Frame to UART4 */
//                 if (uart4_fd >= 0)
//                 {
//                     ssize_t total_written = 0;
//                     while (total_written < g_frame_len[cam])
//                     {
//                         ssize_t ret = write(uart4_fd, &g_frame_buf[cam][total_written], g_frame_len[cam] - total_written);
//                         if (ret > 0)
//                         {
//                             total_written += ret;
//                         }
//                         else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
//                         {
//                             usleep(1000);
//                         }
//                         else
//                         {
//                             break; /* Hard write error */
//                         }
//                     }

//                     /* Append extra FFD9 termination marker on the final camera */
//                     if (cam == NUM_CAMERAS - 1)
//                     {
//                         uint8_t extra_eoi[] = {0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9};
//                         write(uart4_fd, extra_eoi, sizeof(extra_eoi));
//                     }

//                     tcdrain(uart4_fd);
//                 }

//                 frame_sent[cam] = true;
//                 cameras_sent++;
//             }
//         }

//         usleep(10000); /* Sleep 10ms between polling cycles */
//     }

//     if (uart4_fd >= 0) close(uart4_fd);

//     pthread_mutex_lock(&g_console_mutex);
//     printf("[SUBSCRIBER] All cameras processed. Exiting cleanly.\n");
//     fflush(stdout);
//     pthread_mutex_unlock(&g_console_mutex);

//     return 0;
// }
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "ring_buffer.h"

#define MAX_FRAME_SIZE 16000
#define UART4_DEV "/dev/ttyS1"

extern pthread_mutex_t g_console_mutex;

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

/* Helper function: Guarantees complete non-blocking delivery over UART4 without byte drops */
static void uart4_send_all(int fd, const void *buf, size_t len)
{
    if (fd < 0 || buf == NULL || len == 0) return;

    const uint8_t *ptr = (const uint8_t *)buf;
    size_t total_written = 0;

    while (total_written < len)
    {
        ssize_t ret = write(fd, ptr + total_written, len - total_written);
        if (ret > 0)
        {
            total_written += ret;
        }
        else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            usleep(1000); /* Wait 1ms for NuttX UART TX buffer FIFO space */
        }
        else
        {
            break; /* Physical hardware error */
        }
    }
    tcdrain(fd);
}

int hello_main(int argc, char *argv[])
{
    struct image_chunk_s chunk;
    memset(g_frame_len, 0, sizeof(g_frame_len));

    int uart4_fd = configure_uart4(UART4_DEV);

    pthread_mutex_lock(&g_console_mutex);
    printf("[SUBSCRIBER] Forwarding full frame JPEG data to UART4...\n");
    if (uart4_fd < 0)
    {
        printf("[SUBSCRIBER ERROR] Failed to open UART4 (%s): %s\n", UART4_DEV, strerror(errno));
    }
    fflush(stdout);
    pthread_mutex_unlock(&g_console_mutex);

    bool frame_ready[NUM_CAMERAS] = {false};
    bool frame_sent[NUM_CAMERAS]  = {false};
    int cameras_sent = 0;

    while (cameras_sent < NUM_CAMERAS)
    {
        /* --- STEP 1: Drain ring buffers and reassemble image in memory --- */
        for (int cam = 0; cam < NUM_CAMERAS; cam++)
        {
            if (frame_ready[cam] || frame_sent[cam]) continue;

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
                    continue;
                }

                if (g_frame_len[cam] + chunk.len <= MAX_FRAME_SIZE)
                {
                    memcpy(&g_frame_buf[cam][g_frame_len[cam]], chunk.data, chunk.len);
                    g_frame_len[cam] += chunk.len;
                }

                if (chunk.is_last)
                {
                    frame_ready[cam] = true;
                    pthread_mutex_lock(&g_console_mutex);
                    printf("[CAM %d READ COMPLETE] Reassembled %lu bytes in memory.\n", 
                           cam_id, (unsigned long)g_frame_len[cam]);
                    fflush(stdout);
                    pthread_mutex_unlock(&g_console_mutex);
                    break;
                }
            }
        }

        /* --- STEP 2: Pre-transmission wait + transmit JPEG --- */
        for (int cam = 0; cam < NUM_CAMERAS; cam++)
        {
            /* Enforce sequence: CAM 1 (RGB) must be fully sent before CAM 2 (NIR) */
            if (cam > 0 && !frame_sent[cam - 1]) break;

            if (frame_ready[cam] && !frame_sent[cam])
            {
                int cam_id = cam + 1;

                /* 1. Hold transmission for 5 seconds (UART4 remains silent) */
                pthread_mutex_lock(&g_console_mutex);
                printf("[CAM %d] Image captured! Holding UART4 output for 5 seconds...\n", cam_id);
                fflush(stdout);
                pthread_mutex_unlock(&g_console_mutex);

                for (int sec = 1; sec <= 5; sec++)
                {
                    struct timespec ts;
                    clock_gettime(CLOCK_REALTIME, &ts);

                    /* Print timestamps to console ONLY (not sent to UART4) */
                    pthread_mutex_lock(&g_console_mutex);
                    printf("[CONSOLE TS CAM%d] Sec:%d Timestamp:%ld.%03ld\n",
                           cam_id, sec, (long)ts.tv_sec, ts.tv_nsec / 1000000);
                    fflush(stdout);
                    pthread_mutex_unlock(&g_console_mutex);

                    sleep(1);
                }

                /* 2. Print Hex Dump to Console */
                pthread_mutex_lock(&g_console_mutex);
                printf("[CAM %d FULL JPEG] 5s wait complete -> Transmitting %lu Bytes to UART4...\n",
                       cam_id, (unsigned long)g_frame_len[cam]);
                printf("[CAM %d HEX DUMP START]\n", cam_id);
                for (uint32_t i = 0; i < g_frame_len[cam]; i++)
                {
                    printf("%02X", g_frame_buf[cam][i]);
                }
                printf("\n[CAM %d HEX DUMP END]\n\n", cam_id);
                fflush(stdout);
                pthread_mutex_unlock(&g_console_mutex);

                /* 3. Send complete JPEG payload over UART4 */
                if (uart4_fd >= 0)
                {
                    uart4_send_all(uart4_fd, g_frame_buf[cam], g_frame_len[cam]);

                    /* Append extra FFD9 termination markers on final camera */
                    if (cam == NUM_CAMERAS - 1)
                    {
                        uint8_t extra_eoi[] = {0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9, 0xFF, 0xD9};
                        uart4_send_all(uart4_fd, extra_eoi, sizeof(extra_eoi));
                    }
                }

                frame_sent[cam] = true;
                cameras_sent++;
            }
        }

        usleep(10000); /* 10ms loop sleep */
    }

    if (uart4_fd >= 0) close(uart4_fd);

    pthread_mutex_lock(&g_console_mutex);
    printf("[SUBSCRIBER] All cameras processed. Exiting cleanly.\n");
    fflush(stdout);
    pthread_mutex_unlock(&g_console_mutex);

    return 0;
}