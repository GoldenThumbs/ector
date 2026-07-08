#ifndef MSH_INTERNAL
#define MSH_INTERNAL

#include "util/types.h"

#include "mesh.h"

#include <stdlib.h>
#include <string.h>

// "EBMF" in hex
#define MODEL_MAGIC_ID 0x464D4245

// EBMF version
#define EBMF_VERSION 0x0001

#define EBMF_NODE_NAME_MAX 128

typedef enum MSH_MatTokenType_t
{
   MSH_MATTOK_INVALID = 0,
   MSH_MATTOK_VALID_STRING,
   MSH_MATTOK_END_LINE,
   MSH_MATTOK_START,
   MSH_MATTOK_STOP

} MSH_MatTokenType;

typedef struct MSH_MatToken_t
{
   MSH_MatTokenType token_type;
   u32 token_size;
   char* token_start;

} MSH_MatToken;

// Header for the Ector Binary Model Format
typedef struct MSH_ModelHeader_t
{
   union {
      u8 string[4];
      u32 magic;
   } identifier; // must equal "EBMF"

   u16 version;
   i16 root_bone_id;
   u32 node_count;
   u32 mesh_count;
   u32 material_count;

} MSH_ModelHeader;

typedef struct MSH_MeshHeader_t
{
   u32 index_count;
   u32 vertex_count;
   i32 node_id;
   u16 material_id;
   u8 primitive;
   u8 attribute_count;

} MSH_MeshHeader;

static inline char* MSH_CopyTokenString(MSH_MatToken token)
{
   char* string = calloc(token.token_size + 1, sizeof(char));
   if (string != NULL)
      memcpy(string, token.token_start, token.token_size);

   return string;
}

void MSH_RellocAttribute(u8* new_vertex_buffer, u8* old_vertex_buffer, uS* inout_new_size, uS* inout_new_ofs, uS old_size, uS old_ofs, uS new_bytes, uS* inout_total_bytes, const bool clear_attribute);
Material MSH_ParseNextMaterial(memblob memory, uS* char_offset);
MSH_MatToken* MSH_TokenizeMaterial(memblob memory, uS* out_buffer_size);
Mesh MSH_ParseEctorMesh(memblob memory, uS* mesh_size);

#endif
