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

static int dji_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    if (videoFormat != VIDEO_FORMAT_H264)
    {
        printf("dji_setup: Unsupported video format %d\n", videoFormat);
        return 1;
    }

    printf("dji_setup: %d x %d @ %d FPS\n", width, height, redrawRate);
    printf("dji_setup: context = %p\n", context);
    printf("dji_setup: drFlags = 0x%x\n", drFlags);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5123);
    server_addr.sin_addr.s_addr = inet_addr("192.168.42.5");
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    int connect_ret = 0;
    while ((connect_ret = connect(sockfd, &server_addr, sizeof(server_addr))) < 0)
    {
        printf("connect failed: %d: %s\n", sockfd, strerror(errno));
        sleep(1);
    }

    return 0;
}

static void dji_cleanup()
{
    printf("dji_cleanup\n");
}

static int dji_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
    printf("dji_submit_decode_unit: frameNumber: %d, frameType: %d, fullLength: %d\n",
           decodeUnit->frameNumber, decodeUnit->frameType, decodeUnit->fullLength);

    uint8_t *buf = malloc(decodeUnit->fullLength);

    PLENTRY entry = decodeUnit->bufferList;
    uint32_t length = 0;
    while (entry != NULL)
    {
        memcpy(buf + length, entry->data, entry->length);
        length += entry->length;
        entry = entry->next;
    }

    printf("dji_submit_decode_unit: buff = %p, length = %d\n", buf, length);

    send(sockfd, &length, sizeof(length), 0);
    send(sockfd, buf, length, 0);

    free(buf);

    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_dji = {
    .setup = dji_setup,
    .cleanup = dji_cleanup,
    .submitDecodeUnit = dji_submit_decode_unit,
    .capabilities = 0};
