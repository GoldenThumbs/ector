#ifndef ECT_IMAGE_H
#define ECT_IMAGE_H

#include "util/types.h"

enum {
   IMG_TYPE_2D = 0,
   IMG_TYPE_3D
};

enum {
   // Standard Color Formats
   IMG_FORMAT_U8 = 0,
   IMG_FORMAT_U8_SRGB,
   IMG_FORMAT_F32
};

typedef struct Image_t
{
   u8* data;

   resolution2d size;
   i32 depth;
   u8 mipmap_count;
   u8 channel_count;
   u8 image_type;
   u8 image_format;

} Image;

Image Image_CreateImage(memblob memory, u8 image_type, resolution2d slice_size, bool is_srgb);
void Image_Free(Image* image);

void Image_GenerateMipmaps(Image* image);

#endif
