#ifndef ECT_FILES_H
#define ECT_FILES_H

#include "util/types.h"

#include <stdio.h>
#include <stdarg.h>

#define PATH_CHARACTER_LIMIT 4096

// will remove the top part of the base path, as it is assumed to be a file.
// just add a slash at the end if it's a directory and this won't be an issue.
// NOTE: this allocates memory. remember to free!!!!
char* Util_MakeFilePath(const char* base_path, const char* file_name);
memblob Util_LoadFileIntoMemory(const char* file_path, bool read_as_binary);

// NOTE: allocates new memory for result
memblob Util_PrependShaderDefines(memblob shader_data, const char* defines[], const u32 define_count, const char* extra);

// NOTE: NULL stream assumes standard output or standard error.
void Util_Log(FILE* stream, const char* module_name, error err, const char* message, ...);

#endif
