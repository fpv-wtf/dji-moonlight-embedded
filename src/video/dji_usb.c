#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <libusb-1.0/libusb.h>
#include <netinet/in.h> co
#include <sys/socket.h>

#include "dji.h"
#include "video.h"

#define USB_VID 0x2ca3
#define USB_PID 0x1f
#define USB_CONFIG 1
#define USB_INTERFACE 3
#define USB_ENDPOINT_OUT 0x03

libusb_device_handle *dev = NULL;

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

    r = libusb_set_configuration(dev, USB_CONFIG);
    if (r != 0)
    {
        fprintf(stderr, "unable to set configuration: (%d) %s\n", r, libusb_strerror(r));
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
    uint8_t *buf = malloc(decodeUnit->fullLength);

    PLENTRY entry = decodeUnit->bufferList;
    uint32_t length = 0;
    while (entry != NULL)
    {
        memcpy(buf + length, entry->data, entry->length);
        length += entry->length;
        entry = entry->next;
    }

    libusb_bulk_transfer(dev, USB_ENDPOINT_OUT, &length, sizeof(length), NULL, 0);
    libusb_bulk_transfer(dev, USB_ENDPOINT_OUT, buf, length, NULL, 0);

    free(buf);

    return DR_OK;
}

DECODER_RENDERER_CALLBACKS decoder_callbacks_dji_usb = {
    .cleanup = dji_usb_cleanup,
    .setup = dji_usb_setup,
    .submitDecodeUnit = dji_usb_submit_decode_unit,
    .capabilities = 0};
