#pragma once
#include <stdint.h>

#define DJI_HEADER_MAGIC 0x00042069

typedef struct connect_header_s
{
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
} connect_header_t;
