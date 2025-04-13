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

   mesh->vertex_count = 0;
   mesh->index_count = 0;
}