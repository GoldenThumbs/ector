#include "util/types.h"
#include "util/array.h"

#include "renderer.h"
#include "renderer/internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

void Renderer_RegisterDrawableType(Renderer* renderer, const char* name, const DrawableTypeDesc* desc)
{
   if (renderer == NULL || name == NULL)
      return;

   DrawableRenderFunc render_func = NULL;
   DrawableFunc on_create_func = NULL;
   DrawableFunc on_remove_func = NULL;
   DrawableFunc on_enable_func = NULL;
   DrawableFunc on_disable_func = NULL;
   uS data_size = 0;

   if (desc != NULL)
   {
      render_func = desc->render_func;
      on_create_func = desc->on_create_func;
      on_remove_func = desc->on_remove_func;
      on_enable_func = desc->on_enable_func;
      on_disable_func = desc->on_disable_func;
      data_size = desc->data_size;

   }

   uS name_length = strnlen(name, RNDR_NAME_MAX);

   if (name_length < 2 || RNDR_GetDrawableTypeIndex(renderer, name) != RNDR_INVALID_TYPE_IDX)
      return;

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);

   uS name_mem_bytes = (name_length + 1) * sizeof(char);
   uS mem_bytes = sizeof(rndr_Drawable) + data_size;

   SET_ARRAY_LENGTH(renderer->drawable_types, drawable_type_count + 1);

   renderer->drawable_types[drawable_type_count].render = render_func;
   renderer->drawable_types[drawable_type_count].on_create = on_create_func;
   renderer->drawable_types[drawable_type_count].on_remove = on_remove_func;
   renderer->drawable_types[drawable_type_count].on_enable = on_enable_func;
   renderer->drawable_types[drawable_type_count].on_disable = on_disable_func;

   renderer->drawable_types[drawable_type_count].visible_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].freed_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].active_drawable_root = RNDR_INVALID_LIST_LINK;
   renderer->drawable_types[drawable_type_count].culled_drawable_count = 0;

   renderer->drawable_types[drawable_type_count].type_size = (u32)mem_bytes;

   renderer->drawable_types[drawable_type_count].name = malloc(name_mem_bytes);
   memcpy(renderer->drawable_types[drawable_type_count].name, name, name_mem_bytes);

   renderer->drawable_types[drawable_type_count].drawable_buffer = Util_CreateArrayOfLength(4, mem_bytes);

   for (u32 slot_i = 0; slot_i < SURF_MAX_TEXTURES; slot_i++)
      Graphics_BindTexture(renderer->graphics, renderer->built_in.texture.white, slot_i);

}

u16 Renderer_GetDrawableTypeIndexFromName(Renderer* renderer, const char* drawable_type_name)
{
   return RNDR_GetDrawableTypeIndex(renderer, drawable_type_name);
}

Drawable Renderer_CreateDrawable(Renderer* renderer, const char* drawable_type_name)
{
   Drawable drawable_handle = { 0 };
   drawable_handle.id = INVALID_HANDLE_ID;
   drawable_handle.drawable_type_idx = RNDR_INVALID_TYPE_IDX;

   if (renderer == NULL || drawable_type_name == NULL)
      return drawable_handle;

   u16 drawable_type_idx = RNDR_GetDrawableTypeIndex(renderer, drawable_type_name);
   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, drawable_type_idx);
   if (drawable_type == NULL)
      return drawable_handle;

   u32 drawable_count = Util_ArrayLength(drawable_type->drawable_buffer);
   u16 drawable_idx = (u16)drawable_count;

   if (drawable_type->freed_drawable_root == RNDR_INVALID_LIST_LINK)
   {
      SET_ARRAY_LENGTH(drawable_type->drawable_buffer, drawable_count + 1);

   } else {
      drawable_idx = drawable_type->freed_drawable_root;

      rndr_Drawable* free_drawable = RNDR_DrawableAtIndex(drawable_type, drawable_idx);
      drawable_type->freed_drawable_root = free_drawable->next_freed;

      if (free_drawable->next_freed != RNDR_INVALID_LIST_LINK)
      {
         rndr_Drawable* next_free_drawable = RNDR_DrawableAtIndex(drawable_type, free_drawable->next_freed);
         next_free_drawable->prev_freed = RNDR_INVALID_LIST_LINK;

      }

      free_drawable->compare.ref++;

   }

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, drawable_idx);
   u16 ref = drawable->compare.ref;

   memset(drawable, 0, (uS)drawable_type->type_size);
   drawable->compare.handle = drawable_idx;
   drawable->compare.ref = ref;
   drawable->drawable_type_idx = drawable_type_idx;
   drawable->enabled = true;
   drawable->prev_active = RNDR_INVALID_LIST_LINK;

   drawable->next_active = drawable_type->active_drawable_root;
   drawable_type->active_drawable_root = drawable_idx;

   if (drawable->next_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_active);
      next_drawable->prev_active = drawable_idx;

   }

   drawable_handle.id = drawable->compare.id;
   drawable_handle.drawable_type_idx = drawable_type_idx;

   if (drawable_type->on_create != NULL)
      drawable_type->on_create(renderer, drawable_handle);

   if (drawable_type->on_enable != NULL)
      drawable_type->on_enable(renderer, drawable_handle);

   return drawable_handle;
}

void Renderer_RemoveDrawable(Renderer* renderer, Drawable res_drawable)
{
   if (renderer == NULL || res_drawable.id == INVALID_HANDLE_ID)
      return;

   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, res_drawable.drawable_type_idx);
   if (drawable_type == NULL)
      return;

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, res_drawable.handle);
   if (drawable == NULL || drawable->enabled == false)
      return;

   if (drawable_type->on_disable != NULL)
      drawable_type->on_disable(renderer, res_drawable);

   if (drawable_type->on_remove != NULL)
      drawable_type->on_remove(renderer, res_drawable);

   drawable->enabled = false;

   if (drawable->next_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_active);
      next_drawable->prev_active = drawable->prev_active;

   }

   if (drawable->prev_active != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* last_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->prev_active);
      last_drawable->next_active = drawable->next_active;

   } else
      drawable_type->active_drawable_root = drawable->next_active;

   drawable->prev_freed = RNDR_INVALID_LIST_LINK;
   drawable->next_freed = drawable_type->freed_drawable_root;

   if (drawable->next_freed != RNDR_INVALID_LIST_LINK)
   {
      rndr_Drawable* next_free_drawable = RNDR_DrawableAtIndex(drawable_type, drawable->next_freed);
      next_free_drawable->prev_freed = res_drawable.handle;

   }
}

void* Renderer_DrawableData(Renderer* renderer, Drawable res_drawable)
{
   if (renderer == NULL || res_drawable.id == INVALID_HANDLE_ID)
      return NULL;

   rndr_Drawable* drawable = RNDR_GetDrawable(renderer, res_drawable);
   if (drawable == NULL)
      return NULL;

   return (void*)drawable->data;
}

void* Renderer_GetDrawableDataFromIndex(Renderer* renderer, u16 drawable_type_idx, u16 drawable_idx)
{
   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, drawable_type_idx);
   if (drawable_type == NULL)
      return NULL;

   u32 drawable_count = Util_ArrayLength(drawable_type->drawable_buffer);
   if (drawable_count <= (u32)drawable_idx)
      return NULL;

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, drawable_idx);
   if (drawable == NULL)
      return NULL;

   return (void*)drawable->data;
}

u16 RNDR_GetDrawableTypeIndex(Renderer* renderer, const char* drawable_type_name)
{
   if (renderer == NULL || drawable_type_name == NULL)
      return RNDR_INVALID_TYPE_IDX;

   u32 drawable_type_count = Util_ArrayLength(renderer->drawable_types);
   for (u32 type_i = 0; type_i < drawable_type_count; type_i++)
   {
      if (strncmp(drawable_type_name, renderer->drawable_types[type_i].name, RNDR_NAME_MAX) == 0)
         return (u16)type_i;

   }

   return RNDR_INVALID_TYPE_IDX;
}

rndr_DrawableType* RNDR_GetDrawableType(Renderer* renderer, u16 drawable_type_idx)
{
   if (renderer == NULL || drawable_type_idx == RNDR_INVALID_TYPE_IDX)
      return NULL;

   if (Util_ArrayLength(renderer->drawable_types) <= drawable_type_idx)
      return NULL;

   return &renderer->drawable_types[drawable_type_idx];
}

rndr_Drawable* RNDR_GetDrawable(Renderer* renderer, Drawable res_drawable)
{
   if (renderer == NULL || res_drawable.id == INVALID_HANDLE_ID)
      return NULL;

   rndr_DrawableType* drawable_type = RNDR_GetDrawableType(renderer, res_drawable.drawable_type_idx);
   if (drawable_type == NULL)
      return NULL;

   u32 drawable_count = Util_ArrayLength(drawable_type->drawable_buffer);
   if (drawable_count <= (u32)res_drawable.handle)
      return NULL;

   rndr_Drawable* drawable = RNDR_DrawableAtIndex(drawable_type, res_drawable.handle);
   if (drawable->compare.id != res_drawable.id)
      return NULL;

   return drawable;
}

void RNDR_RegisterDefaultDrawables(Renderer* renderer)
{
   Renderer_RegisterDrawableType(renderer, EMPTY_DRAWABLE_TYPE, NULL);
   Renderer_RegisterDrawableType(renderer, GEOMETRY_DRAWABLE_TYPE, &(DrawableTypeDesc){
      .data_size = sizeof(GeometryDrawable),
      .on_create_func = RNDR_GeometryOnCreateFunc,
      .render_func = RNDR_GeometryRenderFunc
   });

}
