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
   uS num_bytes = ftell(input_file);

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
