// #ifndef __RING_BUFFER_H 
// #define __RING_BUFFER_H 

// #include <stdint.h> 

// #define CHUNK_SIZE 256 
// #define RING_BUF_LEN 32 
// #define NUM_CAMERAS 2 

// struct image_chunk_s { 
//   uint16_t len; 
//   uint16_t frame_id; 
//   uint8_t is_last; 
//   uint8_t is_meta; 
//   uint8_t data[CHUNK_SIZE]; 
// }; 

// void rb_init(void); 
// int rb_write(int cam_index, const struct image_chunk_s *c); 
// int rb_read(int cam_index, struct image_chunk_s *c);

// #endif /* __RING_BUFFER_H */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <pthread.h>

#define NUM_CAMERAS 2
#define CHUNK_SIZE 512
#define RING_BUF_LEN 32

struct image_chunk_s {
    uint8_t data[CHUNK_SIZE];
    uint16_t len;
    uint16_t frame_id;
    uint8_t is_last;
    uint8_t is_meta;
};

/* Global mutex shared across all tasks to lock stdout and stderr */
extern pthread_mutex_t g_console_mutex;

void rb_init(void);
int rb_write(int cam_index, const struct image_chunk_s *chunk);
int rb_read(int cam_index, struct image_chunk_s *chunk);

#endif /* RING_BUFFER_H */