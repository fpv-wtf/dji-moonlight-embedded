#include "video.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int sockfd = -1;
int frame_counter = 0;

typedef struct connect_header_s
{
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
} connect_header_t;

static int
dji_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    if (videoFormat != VIDEO_FORMAT_H264)
    {
        printf("dji_setup: Unsupported video format %d\n", videoFormat);
        return 1;
    }

    printf("dji: %d x %d @ %d FPS\n", width, height, redrawRate);
    printf("dji: context = %p\n", context);
    printf("dji: drFlags = 0x%x\n", drFlags);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(42069);
    server_addr.sin_addr.s_addr = inet_addr("192.168.42.5");
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    printf("dji: Connecting to %s:%d...\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    int connect_ret = 0;
    while ((connect_ret = connect(sockfd, &server_addr, sizeof(server_addr))) < 0)
    {
        printf("connect failed: %d: %s\n", sockfd, strerror(errno));
        sleep(1);
    }

    printf("dji: Connected!\n");

    connect_header_t header;
    header.magic = 0x42069;
    header.width = width;
    header.height = height;
    header.fps = redrawRate;

    send(sockfd, &header, sizeof(header), 0);

    return 0;
}

static int dji_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
    // printf("dji frameNumber: %d, frameType: %d, fullLength: %d\n",
    //        decodeUnit->frameNumber, decodeUnit->frameType, decodeUnit->fullLength);

    uint8_t *buf = malloc(decodeUnit->fullLength);

    PLENTRY entry = decodeUnit->bufferList;
    uint32_t length = 0;
    while (entry != NULL)
    {
        memcpy(buf + length, entry->data, entry->length);
        length += entry->length;
        entry = entry->next;
    }

    // printf("dji_submit_decode_unit: buff = %p, length = %d\n", buf, length);

    send(sockfd, &length, sizeof(length), 0);
    send(sockfd, buf, length, 0);

    free(buf);

    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_dji = {
    .setup = dji_setup,
    .submitDecodeUnit = dji_submit_decode_unit,
    .capabilities = 0};
