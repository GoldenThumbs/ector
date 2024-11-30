#ifndef ECT_ENGINE_H
#define ECT_ENGINE_H

#include "ect_types.h"

#include "core/keymap.h"

typedef struct EctEngine_t
{
   const char* app_name;
   bool quit;
} EctEngine;

typedef struct EctEngineDesc_t
{
   const char* app_name;
   struct {
      const char* title;
      i32 width, height;
   } window;
} EctEngineDesc;

EctEngine EctInitEngine(EctEngineDesc* desc);
void EctFreeEngine(void);

bool EctShouldQuit(EctEngine* engine);
bool EctCheckKey(EctKey key, u8 desired_state, bool exclusive);
bool EctCheckKeyMods(EctKey key, u8 desired_state, u8 modifiers, bool exclusive);
u8 EctKeyState(EctKey key);

void EctGetFrameSize(i32* width, i32* height);

#endif
