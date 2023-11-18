#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <libusb-1.0/libusb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "dji.h"
#include "video.h"

#define USB_VID 0x2ca3
#define USB_PID 0x1f
#define USB_CONFIG 1
#define USB_INTERFACE 3
#define USB_ENDPOINT_OUT 0x03

#define ACCEPTABLE_FRAME_TIME 50000000

static libusb_device_handle *dev = NULL;

static struct timespec last_frame_time = {0, 0};
// static struct timespec last_key_time = {0, 0};

static uint8_t buf[1000000];

static int
dji_usb_setup(int videoFormat, int width, int height, int redrawRate, void *context, int drFlags)
{
    if (videoFormat != VIDEO_FORMAT_H264)
    {
        printf("dji_usb: Unsupported video format %d\n", videoFormat);
        return 1;
    }

    printf("dji_usb: %d x %d @ %d FPS\n", width, height, redrawRate);
    printf("dji_usb: context = %p\n", context);
    printf("dji_usb: drFlags = 0x%x\n", drFlags);

    int r = libusb_init(NULL);
    if (r < 0)
    {
        fprintf(stderr, "unable to init libusb: %s\n", libusb_strerror(r));
        exit(1);
    }

    dev = libusb_open_device_with_vid_pid(NULL, USB_VID, USB_PID);
    if (dev == NULL)
    {
        libusb_exit(NULL);
        fprintf(stderr, "unable to open device, or device not found\n");
        if (geteuid() != 0)
        {
            fprintf(stderr, "try running as root (with sudo)\n");
        }
        exit(1);
    }

    r = libusb_claim_interface(dev, USB_INTERFACE);
    if (r != 0)
    {
        fprintf(stderr, "unable to claim interface: (%d) %s\n", r, libusb_strerror(r));
        exit(1);
    }

    connect_header_t header;
    header.magic = DJI_HEADER_MAGIC;
    header.width = width;
    header.height = height;
    header.fps = redrawRate;

    r = libusb_bulk_transfer(dev, USB_ENDPOINT_OUT, (unsigned char *)&header, sizeof(header), NULL, 0);
    if (r != 0)
    {
        fprintf(stderr, "unable to send header: (%d) %s\n", r, libusb_strerror(r));
        exit(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &last_frame_time);
    // last_key_time = last_frame_time;

    return 0;
}

static void dji_usb_cleanup(void)
{
    libusb_release_interface(dev, USB_INTERFACE);
    libusb_close(dev);
    libusb_exit(NULL);
}

static int dji_usb_submit_decode_unit(PDECODE_UNIT decodeUnit)
{
    PLENTRY entry = decodeUnit->bufferList;
    uint32_t length = 0;
    while (entry != NULL)
    {
        if (length + entry->length > sizeof(buf))
        {
            printf("dji_usb: Frame too large: %d >= 1Mb\n", length + entry->length);
            return DR_NEED_IDR;
        }

        memcpy(buf + length, entry->data, entry->length);
        length += entry->length;
        entry = entry->next;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t diff = (now.tv_sec - last_frame_time.tv_sec) * 1000000000 +
                    (now.tv_nsec - last_frame_time.tv_nsec);
    last_frame_time = now;

    if (diff >= ACCEPTABLE_FRAME_TIME && decodeUnit->frameType != FRAME_TYPE_IDR)
    {
        printf("dji_net: Dropping frame: %lums >= 50ms\n", diff / 1000000);
    }
    else
    {
        uint8_t frame_type = decodeUnit->frameType;
        libusb_bulk_transfer(dev, USB_ENDPOINT_OUT, &length, sizeof(length), NULL, 0);
        libusb_bulk_transfer(dev, USB_ENDPOINT_OUT, &frame_type, sizeof(frame_type), NULL, 0);
        libusb_bulk_transfer(dev, USB_ENDPOINT_OUT, buf, length, NULL, 0);
    }

    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_dji_usb = {
    .cleanup = dji_usb_cleanup,
    .setup = dji_usb_setup,
    .submitDecodeUnit = dji_usb_submit_decode_unit,
    .capabilities = 0};
