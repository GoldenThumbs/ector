#include "util/types.h"
#include "util/extra_types.h"
#include "util/math.h"
#include "util/vec4.h"

#include "image.h"

#include <stb_image.h>

#include <stdlib.h>
#include <string.h>

Image Image_CreateImage(memblob memory, u8 image_type, resolution2d slice_size, bool is_srgb)
{
   if (memory.data == NULL || memory.size == 0)
      return (Image){ .data = NULL };

   bool is_hdr = (bool)stbi_is_hdr_from_memory(memory.data, (i32)memory.size);

   Image image = { 0 };
   i32 image_channels = 0;
   i32 force_channels = 4;

   if (is_hdr)
      image.data = (u8*)stbi_loadf_from_memory(memory.data, (i32)memory.size, &image.size.width, &image.size.height, &image_channels, force_channels);
   else
      image.data = stbi_load_from_memory(memory.data, (i32)memory.size, &image.size.width, &image.size.height, &image_channels, force_channels);

   image_channels = M_MAX(image_channels, force_channels);

   if (image.data == NULL)
      return (Image){ .data = NULL };

   image.image_format = is_hdr ? IMG_FORMAT_F32 : (is_srgb ? IMG_FORMAT_U8_SRGB : IMG_FORMAT_U8);
   image.channel_count = (u8)force_channels;
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

   uS bytes_per_channel = is_hdr ? sizeof(f32) : sizeof(u8);
   uS bytes_per_pixel = bytes_per_channel * (uS)image->channel_count;
   uS total_bytes = bytes_per_pixel * ((uS)image->size.width * (uS)image->size.height);

   u8* data = malloc(total_bytes);
   if (data == NULL)
      return;

   data = memcpy(data, image->data, total_bytes);

   u8 mipmap_count = 1;
   resolution2d mip_size = image->size;

   uS prev_mip_offset = 0;
   while (mip_size.width > 1 || mip_size.height > 1)
   {
      resolution2d prev_mip_size = mip_size;
      mip_size.width = M_MAX(mip_size.width / 2, 1);
      mip_size.height = M_MAX(mip_size.height / 2, 1);

      uS mip_bytes = bytes_per_pixel * ((uS)mip_size.width * (uS)mip_size.height);

      u8* tmp_data = realloc(data, total_bytes + mip_bytes);
      if (tmp_data == NULL)
      {
         free(data);
         return;
      }

      data = tmp_data;

      u8* prev_mip_data = data + prev_mip_offset;
      u8* mip_data = data + total_bytes;

      prev_mip_offset = total_bytes;

      total_bytes += mip_bytes;
      mipmap_count++;

      for (i32 y_i = 0; y_i < mip_size.height; y_i++)
         for (i32 x_i = 0; x_i < mip_size.width; x_i++)
      {
         i32 mip_idx = y_i * mip_size.width + x_i;

         u8* mip_pixel = mip_data + (uS)mip_idx * bytes_per_pixel;

         i32 prev_mip_x0 = M_MIN(x_i * 2, prev_mip_size.width - 1);
         i32 prev_mip_y0 = M_MIN(y_i * 2, prev_mip_size.height - 1);
         i32 prev_mip_x1 = (prev_mip_x0 + 1) % prev_mip_size.width;
         i32 prev_mip_y1 = (prev_mip_y0 + 1) % prev_mip_size.height;

         i32 prev_mip_idx_00 = prev_mip_y0 * prev_mip_size.width + prev_mip_x0;
         i32 prev_mip_idx_10 = prev_mip_y0 * prev_mip_size.width + prev_mip_x1;
         i32 prev_mip_idx_01 = prev_mip_y1 * prev_mip_size.width + prev_mip_x0;
         i32 prev_mip_idx_11 = prev_mip_y1 * prev_mip_size.width + prev_mip_x1;

         u8* prev_mip_pixel_00 = prev_mip_data + (uS)prev_mip_idx_00 * bytes_per_pixel;
         u8* prev_mip_pixel_10 = prev_mip_data + (uS)prev_mip_idx_10 * bytes_per_pixel;
         u8* prev_mip_pixel_01 = prev_mip_data + (uS)prev_mip_idx_01 * bytes_per_pixel;
         u8* prev_mip_pixel_11 = prev_mip_data + (uS)prev_mip_idx_11 * bytes_per_pixel;

         for (i32 channel_i = 0; channel_i < image->channel_count; channel_i++)
         {
            uS channel_offset = (uS)channel_i * bytes_per_channel;

            u8* mip_channel = mip_pixel + channel_offset;

            u8* prev_mip_channel_00 = prev_mip_pixel_00 + channel_offset;
            u8* prev_mip_channel_10 = prev_mip_pixel_10 + channel_offset;
            u8* prev_mip_channel_01 = prev_mip_pixel_01 + channel_offset;
            u8* prev_mip_channel_11 = prev_mip_pixel_11 + channel_offset;

            vec4 quad_values = { 0 };

            if (is_hdr)
            {
               quad_values.x = *((f32*)prev_mip_channel_00);
               quad_values.y = *((f32*)prev_mip_channel_10);
               quad_values.z = *((f32*)prev_mip_channel_01);
               quad_values.w = *((f32*)prev_mip_channel_11);

            } else {
               quad_values.x = BYTE_TO_F32(*prev_mip_channel_00);
               quad_values.y = BYTE_TO_F32(*prev_mip_channel_10);
               quad_values.z = BYTE_TO_F32(*prev_mip_channel_01);
               quad_values.w = BYTE_TO_F32(*prev_mip_channel_11);

            }

            const vec4 quad_weights = VEC4(0.25f, 0.25f, 0.25f, 0.25f);
            f32 value = Util_DotVec4(quad_values, quad_weights);
            u8 value_sdr = F32_TO_BYTE(M_CLAMP(value, 0.0f, 1.0f));

            void* value_ptr = NULL;

            if (is_hdr)
               (*((f32*)mip_channel)) = value;
            else
               (*mip_channel) = value_sdr;

            // memcpy(mip_channel, &value_ptr, bytes_per_channel);

         }

      }

   }

   free(image->data);
   image->data = data;
   image->mipmap_count = mipmap_count;

}
