#ifndef ECT_DEFAULT_LIGHTMANAGER_H
#define ECT_DEFAULT_LIGHTMANAGER_H

// #include "util/types.h"
// #include "graphics.h"
#include "renderer.h"

#define LIGHT_DRAWABLE_TYPE "LightDrawable"
#define SUN_DRAWABLE_TYPE "SunDrawable"

// 95 68 69 70 65 85 76 84
#define DEFAULTLIGHTMANAGER_ID 0x5F44454641554C54u

typedef struct DefaultLightManager_t DefaultLightManager;

LightManagerInfo DefaultLightManager_Info(Renderer* renderer);

DefaultLightManager* DefaultLightManager_Init(Renderer* renderer);
void DefaultLightManager_Free(DefaultLightManager* lightmanager);

void DefaultLightManager_PreRender(DefaultLightManager* lightmanager, Renderer* renderer);
// void DefaultLightManager_OnRender(DefaultLightManager* lightmanager, Renderer* renderer);

Drawable DefaultLightManager_CreateLight(Renderer* renderer);

void DefaultLightManager_SetLightOrigin(Renderer* renderer, Drawable light_drawable, vec3 origin);
void DefaultLightManager_SetLightRadius(Renderer* renderer, Drawable light_drawable, f32 radius);
void DefaultLightManager_SetLightColor(Renderer* renderer, Drawable light_drawable, color8 color);
void DefaultLightManager_SetLightBrightness(Renderer* renderer, Drawable light_drawable, f32 brightness);
void DefaultLightManager_SetLightSpotlightAngle(Renderer* renderer, Drawable light_drawable, f32 spotlight_angle);
void DefaultLightManager_SetLightSpotlightSoftness(Renderer* renderer, Drawable light_drawable, f32 spotlight_softness);
void DefaultLightManager_SetLightAngles(Renderer* renderer, Drawable light_drawable, f32 azimuth_angle, f32 zenith_angle);


#endif
