#include "sock.h"

uint32_t _crc32(const uint8_t *data, size_t length) {
    uint32_t crc = ~0U;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1));
        }
    }
    return ~crc;
}

void inflate_buffer(const uint8_t *input, size_t input_length, uint8_t *output, size_t *output_length) {
    z_stream stream;
    memset(&stream, 0, sizeof(z_stream));

    // Initialize inflate for raw deflate stream
    if (inflateInit2(&stream, -15) != Z_OK) {
        fprintf(stderr, "inflateInit2 failed\n");
        return;
    }

    stream.next_in = (Bytef *)input;
    stream.avail_in = input_length;
    stream.next_out = output;
    stream.avail_out = *output_length;

    if (inflate(&stream, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "Inflate failed\n");
        inflateEnd(&stream);
        return;
    }

    *output_length = stream.total_out;
    inflateEnd(&stream);
}

void *udp_listener(void *arg) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    uint8_t buffer[BUFFER_SIZE];
    uint8_t decompressed_buffer[BUFFER_SIZE];

    SharedBuffer* shared_data = (SharedBuffer*) arg;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
        perror("Failed to set non-blocking mode");
        close(sockfd);
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return NULL;
    }

    printf("UDP listener thread started on port %d\n", PORT);
    running = true;

    while (running) {
        ssize_t received;
        size_t total_received = 0;
        uint32_t payload_length = 0;

        while (total_received < HEADER_SIZE) {
            received = recvfrom(sockfd, buffer + total_received, BUFFER_SIZE - total_received, 0, (struct sockaddr *)&client_addr, &client_len);

            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(1000);
                    continue;
                } else {
                    perror("recvfrom failed");
                    break;
                }
            }

            total_received += received;
        }

        if (total_received < HEADER_SIZE) {
            continue;
        }

        uint32_t magic = *(uint32_t *)buffer;
        if (magic != MAGIC) {
            fprintf(stderr, "Invalid magic (%x), dropping\n", magic);
            continue;
        }

        payload_length = *(uint32_t *)(buffer + CRC32_SIZE);
        if (payload_length > MAX_PAYLOAD_LEN) {
            fprintf(stderr, "Invalid payload length (%x), dropping\n", payload_length);
            continue;
        }

        size_t expected_size = payload_length + HEADER_SIZE + CRC32_SIZE;
        while (total_received < expected_size) {
            received = recvfrom(sockfd, buffer + HEADER_SIZE + total_received, expected_size - total_received, 0,
                                (struct sockaddr *)&client_addr, &client_len);

            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(1000);
                    continue;
                } else {
                    perror("recvfrom failed");
                    break;
                }
            }
            total_received += received;
        }

        if (total_received < expected_size) {
            fprintf(stderr, "Incomplete payload received, dropping\n");
            continue;
        }

        uint32_t received_crc = *(uint32_t *)(buffer + HEADER_SIZE + payload_length);
        uint32_t computed_crc = _crc32(buffer + HEADER_SIZE, payload_length);

        if (received_crc != computed_crc) {
            fprintf(stderr, "CRC mismatch (%x != %x), dropping\n", received_crc, computed_crc);
            continue;
        }

        uint8_t *payload = buffer + HEADER_SIZE;

        // decompress packet
        size_t decompressed_length = sizeof(decompressed_buffer) - 1;
        inflate_buffer(payload, payload_length, decompressed_buffer, &decompressed_length);
        decompressed_buffer[decompressed_length] = '\x00';

        // send packet data to main thread
        pthread_mutex_lock(&shared_data->mutex);
        memcpy(shared_data->buffer, decompressed_buffer, decompressed_length + 1);
        shared_data->size = decompressed_length;
        shared_data->data_available = true;
        pthread_cond_signal(&shared_data->cond);
        pthread_mutex_unlock(&shared_data->mutex);
    }

    close(sockfd);
    return NULL;
}
