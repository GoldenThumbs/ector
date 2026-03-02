#include "util/extra_types.h"
#include "util/types.h"
#include "util/math.h"

#include "image.h"

#include <stb_image.h>

#include <stdlib.h>
#include <string.h>

Image Image_CreateImage(memblob memory, u8 image_type, resolution2d slice_size, bool is_srgb)
{
   if (memory.data == NULL || memory.size == 0)
      return (Image){ .data = NULL };

   bool is_hdr = (bool)stbi_is_hdr_from_memory(memory.data, memory.size);

   Image image = { 0 };
   i32 image_channels = 0;
   if (is_hdr)
      image.data = (u8*)stbi_loadf_from_memory(memory.data, memory.size, &image.size.width, &image.size.height, &image_channels, 0);
   else
      image.data = stbi_load_from_memory(memory.data, memory.size, &image.size.width, &image.size.height, &image_channels, 0);

   if (image.data == NULL)
      return (Image){ .data = NULL };

   image.image_format = (is_hdr) ? IMG_FORMAT_F32 : (IMG_FORMAT_U8 + (u8)is_srgb);
   image.channel_count = (u8)image_channels;
   image.mipmap_count = 1;
   image.depth = 1;

   u8* slice_data = NULL;
   if (slice_size.width > 0 && slice_size.height > 0)
   {
      resolution2d slices;
      slices.width = image.size.width / slice_size.width;
      slices.height = image.size.height / slice_size.height;

      i32 slice_count = slices.width * slices.height;
      i32 pixels_per_slice = slice_size.width * slice_size.height;

      uS bytes_per_channel = (is_hdr) ? sizeof(f32) : sizeof(u8);
      uS bytes_per_pixel = bytes_per_channel * (uS)image_channels;
      uS bytes_per_slice = (uS)pixels_per_slice * bytes_per_pixel;
      uS bytes_per_slice_line = (uS)slice_size.width * bytes_per_pixel;
      uS bytes_per_line = (uS)image.size.width * bytes_per_pixel;
      uS bytes_per_slice_row = (uS)slice_size.height * bytes_per_line;

      if (pixels_per_slice * slice_count == image.size.width * image.size.height)
         slice_data = malloc(bytes_per_slice * (uS)slice_count);

      if (slice_data != NULL)
      {
         image.depth = slice_count;

         i32 slice_idx = 0;
         for (i32 slice_y = 0; slice_y < slices.height; slice_y++)
         {
            uS y_ofs = (uS)slice_y * bytes_per_slice_row;

            for (i32 slice_x = 0; slice_x < slices.width; slice_x++)
            {
               uS slice_ofs = (uS)slice_idx * bytes_per_slice;
               uS x_ofs = y_ofs + (uS)slice_x * bytes_per_slice_line;

               for (i32 line_i = 0; line_i < slice_size.height; line_i++)
               {
                  uS slice_line_ofs = slice_ofs + (uS)line_i * bytes_per_slice_line;
                  uS line_ofs = x_ofs + (uS)line_i * bytes_per_line;
                  
                  u8* ptr_src = image.data + line_ofs;
                  u8* ptr_dst = slice_data + slice_line_ofs;

                  memcpy(ptr_dst, ptr_src, bytes_per_slice_line);

               }

               slice_idx++;

            }

         }

         stbi_image_free(image.data);
         image.data = slice_data;
         image.size = slice_size;

         if (image.image_type != IMG_TYPE_3D)
         {
            image.size.height *= image.depth;
            image.depth = 1;

         }
      }

   }

   return image;
}

void Image_Free(Image* image)
{
   if (image == NULL || image->data == NULL)
      return;

   free(image->data);
   image->data = NULL;
}

void Image_GenerateMipmaps(Image* image)
{
   if (image == NULL || image->data == NULL)
      return;

   bool is_hdr = (image->image_format == IMG_FORMAT_F32);

   uS bytes_per_channel = (is_hdr) ? sizeof(f32) : sizeof(u8);
   uS bytes_per_pixel = bytes_per_channel * (uS)image->channel_count;
   uS total_bytes = bytes_per_pixel * (uS)(image->size.width * image->size.height);

   u8* data = malloc(total_bytes);
   if (data == NULL)
      return;

   data = memcpy(data, image->data, total_bytes);
   uS prev_offset = 0;

   u8 mipmap_count = 1;
   resolution2d size = image->size;
   while (size.width > 1 || size.height > 1)
   {
      resolution2d prev_size = size;
      size.width = M_MAX(size.width / 2, 1);
      size.height = M_MAX(size.height / 2, 1);

      uS mip_bytes = bytes_per_pixel * (uS)(size.width * size.height);

      u8* tmp_data = realloc(data, total_bytes + mip_bytes);
      if (tmp_data == NULL)
      {
         free(data);
         return;
      }

      data = tmp_data;

      u8* prev_data = data + prev_offset;
      u8* mip_data = data + total_bytes;

      prev_offset = total_bytes;

      total_bytes += mip_bytes;
      mipmap_count++;

      for (i32 y_i = 0; y_i < size.height; y_i++)
         for (i32 x_i = 0; x_i < size.width; x_i++)
      {
         i32 prev_y = M_MIN(y_i * 2, prev_size.height - 1);
         i32 prev_x = M_MIN(x_i * 2, prev_size.width - 1);

         i32 mip_idx = y_i * size.width + x_i;

         u8* mip_pixel = mip_data + (uS)mip_idx * bytes_per_pixel;

         for (i32 channel_i = 0; channel_i < image->channel_count; channel_i++)
         {
            u8* mip_channel = mip_pixel + (uS)channel_i * bytes_per_channel;

            f32 value = 0.0f;
            for (i32 offset_y = 0; offset_y < 2; offset_y++)
               for (i32 offset_x = 0; offset_x < 2; offset_x++)
            {
               i32 ofs_x = M_MIN(prev_x + offset_x, prev_size.width - 1);
               i32 ofs_y = M_MIN(prev_y + offset_y, prev_size.height - 1);
               i32 ofs_prev_idx = ofs_y * prev_size.width + ofs_x;

               u8* prev_ofs_pixel = prev_data + (uS)ofs_prev_idx * bytes_per_pixel;
               u8* prev_ofs_channel = prev_ofs_pixel + (uS)channel_i * bytes_per_channel;

               if (is_hdr)
                  value += *((f32*)prev_ofs_channel);
               else
                  value += BYTE_TO_F32(*prev_ofs_channel);

            }

            value *= 0.25f;

            if (is_hdr)
               memcpy(mip_channel, &value, bytes_per_channel);
            else
               (*mip_channel) = F32_TO_BYTE(M_MIN(value, 1.0));

         }
      }

   }

   free(image->data);
   image->data = data;
   image->mipmap_count = mipmap_count;

}
