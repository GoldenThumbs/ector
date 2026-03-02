#include "util/types.h"

#include "util/files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

memblob Util_LoadFileIntoMemory(const char* file_path, bool read_as_binary)
{
   memblob memory = { .size = 0, .data = NULL };

   if (file_path == NULL)
      return memory;

   FILE* input_file = NULL;

   if (read_as_binary)
      input_file = fopen(file_path, "rb");
   else
      input_file = fopen(file_path, "r");

   if (input_file == NULL)
      return memory;

   fseek(input_file, 0, SEEK_END);
   uS num_bytes = ftell(input_file) + (uS)(!read_as_binary);

   u8* data = calloc(num_bytes, sizeof(u8));

   if (data == NULL)
      return memory;

   fseek(input_file, 0, SEEK_SET);
   uS num_bytes_read = fread(data, sizeof(u8), num_bytes, input_file);

   fclose(input_file);

   memory.size = num_bytes;
   memory.data = data;

   return memory;
}

char* Util_MakeFilePath(const char* base_path, const char* file_name)
{
   if (base_path == NULL || file_name == NULL)
      return NULL;

   i32 directory_length = strnlen(base_path, PATH_CHARACTER_LIMIT);
   while (directory_length > 0)
   {
      char directory_char = base_path[--directory_length];
      if (directory_char == '/' || directory_char == '\\')
         break;
   }

   i32 full_path_length = snprintf(NULL, 0, "%.*s/%s", directory_length, base_path, file_name) + 1;
   char* file_path = malloc(full_path_length * sizeof(char));
   if (file_path == NULL)
      return NULL;

   snprintf(file_path, full_path_length, "%.*s/%s", directory_length, base_path, file_name);

   return file_path;
}


memblob Util_PrependShaderDefines(memblob shader_data, const char* defines[], const u32 define_count, const char* extra)
{
   if (shader_data.data == NULL || shader_data.size == 0)
      return (memblob){ 0 };

   if (defines == NULL && define_count != 0)
      return (memblob){ 0 };

   char* shader_code = shader_data.data;
   char* total_defines = NULL;
   uS total_defines_size = 0;

   const char* template = "#define %s\n";

   for (u32 define_i = 0; define_i < define_count; define_i++)
   {
      i32 define_size = snprintf(NULL, 0, template, defines[define_i]);

      char* tmp_defines = realloc(total_defines, total_defines_size + define_size + 1);
      if (tmp_defines == NULL)
         break;

      total_defines = tmp_defines;
      char* tmp_define = total_defines + total_defines_size;
      snprintf(tmp_define, define_size + 1, template, defines[define_i]);

      total_defines_size += define_size;
   }

   const char* glsl_shader_stub = "#version 430 core\n%s%s\n%s\n";
   char* inserted_code = (extra != NULL) ? (char*)extra : "";
   char* inserted_defs = (total_defines != NULL) ? total_defines : "";

   i32 shader_size = snprintf(NULL, 0, glsl_shader_stub, inserted_defs, inserted_code, shader_code) + 1;

   char* full_shader_code = malloc(shader_size);
   if (full_shader_code == NULL)
      return (memblob){ 0 };

   snprintf(full_shader_code, shader_size, glsl_shader_stub, inserted_defs, inserted_code, shader_code);
   if (total_defines != NULL)
      free(total_defines);

   return (memblob){ full_shader_code, shader_size };
}
