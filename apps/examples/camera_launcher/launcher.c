#include <nuttx/config.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sched.h>
#include <termios.h>

#define UART4_DEV "/dev/ttyS1"
#define CMD_LEN 13

extern int camera_MSN2_main(int argc, char *argv[]);
extern int hello_main(int argc, char *argv[]);

static const uint8_t EXPECTED_CMD[CMD_LEN] = {
    0x53, 0x04, 0xCC, 0x5E, 0xBD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void dual_printf(int fd_uart4, const char *fmt, ...)
{
  char buf[256];
  va_list args;

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  printf("%s", buf);
  fflush(stdout);

  if (fd_uart4 >= 0)
  {
    write(fd_uart4, buf, strlen(buf));
  }
}

int launcher_main(int argc, char *argv[])
{
  int fd_uart4 = open(UART4_DEV, O_RDWR | O_NOCTTY);
  if (fd_uart4 < 0)
  {
    printf("[LAUNCHER] Error opening %s: %s\r\n", UART4_DEV, strerror(errno));
    return -1;
  }

  /* Configure raw terminal settings for UART4 */
  struct termios t;
  tcgetattr(fd_uart4, &t);
  
  cfsetispeed(&t, B115200);
  cfsetospeed(&t, B115200);
  
  t.c_cflag = CS8 | CREAD | CLOCAL;
  t.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);
  t.c_oflag &= ~OPOST;
  t.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  
  t.c_cc[VMIN]  = 1; /* Wait for incoming bytes */
  t.c_cc[VTIME] = 0;
  
  tcsetattr(fd_uart4, TCSANOW, &t);
  tcflush(fd_uart4, TCIFLUSH);

  printf("[LAUNCHER] Background service running on %s. Waiting for command...\r\n", UART4_DEV);

  uint8_t rx_buf[CMD_LEN];

  /* Infinite loop: keeps the background task alive permanently */
  while (1)
  {
    uint8_t header = 0;

    /* 1. Synchronize: Read byte-by-byte until start byte 0x53 is found */
    if (read(fd_uart4, &header, 1) <= 0)
    {
      usleep(10000);
      continue;
    }

    if (header != EXPECTED_CMD[0])
    {
      /* Discard noise/misaligned bytes */
      continue;
    }

    rx_buf[0] = header;
    int bytes_read = 1;

    /* 2. Read the remaining 12 bytes of the frame */
    while (bytes_read < CMD_LEN)
    {
      int ret = read(fd_uart4, &rx_buf[bytes_read], CMD_LEN - bytes_read);
      if (ret > 0)
      {
        bytes_read += ret;
      }
      else if (ret < 0 && errno != EAGAIN && errno != EINTR)
      {
        dual_printf(fd_uart4, "[LAUNCHER] Read error on UART4: %s\r\n", strerror(errno));
        break;
      }
    }

    /* 3. Validate complete 13-byte command */
    if (bytes_read == CMD_LEN && memcmp(rx_buf, EXPECTED_CMD, CMD_LEN) == 0)
    {
      dual_printf(fd_uart4, "COMMAND MATCHED! Launching camera + subscriber tasks...\r\n");

      char *camera_argv[] = { "camera_MSN2", NULL };
      char *hello_argv[]  = { "hello", NULL };

      int cam_pid = task_create("camera_MSN2", 100, 8192, camera_MSN2_main, camera_argv);
      if (cam_pid < 0)
      {
        dual_printf(fd_uart4, "ERROR: Failed to start camera task (errno %d)\r\n", errno);
      }

      int hello_pid = task_create("hello", 100, 4096, hello_main, hello_argv);
      if (hello_pid < 0)
      {
        dual_printf(fd_uart4, "ERROR: Failed to start hello task (errno %d)\r\n", errno);
      }
    }
    else
    {
      dual_printf(fd_uart4, "COMMAND MISMATCH\r\n");
    }
  }

  close(fd_uart4);
  return 0;
}