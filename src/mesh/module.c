// #include "util/types.h"

#include "mesh.h"

#include <stdlib.h>

void Mesh_Free(Mesh* mesh)
{
   if (mesh->vertex_buffer != NULL)
   {
      free(mesh->vertex_buffer);
      mesh->vertex_buffer = NULL;
   }

   if (mesh->index_buffer != NULL)
   {
      free(mesh->index_buffer);
      mesh->index_buffer = NULL;
   }

   if (mesh->attributes != NULL)
   {
      free(mesh->attributes);
      mesh->attributes = NULL;
   }

   mesh->vertex_count = 0;
   mesh->index_count = 0;
   mesh->attribute_count = 0;
}