#ifndef _SOCK_H_
#define _SOCK_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <zlib.h>

#include "shared.h"

#define PORT 8888
#define MAGIC 0xdeadface
#define MAX_PAYLOAD_LEN 65535
#define HEADER_SIZE 8
#define CRC32_SIZE 4
#define BUFFER_SIZE (MAX_PAYLOAD_LEN + HEADER_SIZE + CRC32_SIZE) // Magic (4) + Length (4) + Payload + CRC32 (4)

extern volatile bool running;

void *udp_listener(void *arg);

#endif