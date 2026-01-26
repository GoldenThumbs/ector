#include "util/extra_types.h"
#include "util/matrix.h"
#include "util/quaternion.h"
#include <string.h>
#include <util/types.h>
#include <util/keymap.h>
#include <engine.h>
#include <graphics.h>
#include <renderer.h>
#include <default_modules.h>

#include <stb_image.h>

#include <stdio.h>

void ParticleRenderFunc(Renderer* renderer, Drawable self);
Geometry Plane(Graphics* graphics);

typedef struct ParticleDrawable_t
{
   Shader shader;
   Shader particle_shader;
   Texture color_texture;
   Geometry geometry;
   Buffer particle_ssbo;
   color8 color;
   Transform3D transform;

   u32 particle_count;

} ParticleDrawable;

const char particle_vertex_code[] =
   "#version 430 core\n"
   "layout(location=0) in vec3 vrt_position;\n"
   "layout(location=2) in vec2 vrt_texcoord;\n"
   "layout(std140, binding=1) uniform CameraUBO\n"
   "{\n"
   "   mat4 mat_view;\n"
   "   mat4 mat_proj;\n"
   "   mat4 mat_invview;\n"
   "   mat4 mat_invproj;\n"
   "   vec2 u_near_far;\n"
   "   uvec2 u_screen_size;\n"
   "   vec4 u_proj_info;\n"
   "};\n"
   "layout(std140, binding=2) uniform ModelUBO\n"
   "{\n"
   "   mat4 mat_model;\n"
   "   mat4 mat_invmodel;\n"
   "   mat4 mat_mvp;\n"
   "   mat4 mat_normal_model_u_color;\n"
   "};\n"
   "layout(std430, binding=3) restrict buffer ParticleSSBO\n"
   "{\n"
   "   mat4 mat_particle[];\n"
   "};\n"
   "out vec2 v2f_texcoord;\n"
   "void main()\n"
   "{\n"
   "   v2f_texcoord = vrt_texcoord;\n"
   "   gl_Position = mat_proj * mat_view * mat_particle[gl_InstanceID] * vec4(vrt_position, 1.0);\n"
   "}\n";

const char particle_fragment_code[] =
   "#version 430 core\n"
   "layout(binding=0) uniform sampler2D tex_color;\n"
   "layout(std140, binding=1) uniform CameraUBO\n"
   "{\n"
   "   mat4 mat_view;\n"
   "   mat4 mat_proj;\n"
   "   mat4 mat_invview;\n"
   "   mat4 mat_invproj;\n"
   "   vec2 u_near_far;\n"
   "   uvec2 u_screen_size;\n"
   "   vec4 u_proj_info;\n"
   "};\n"
   "layout(std140, binding=2) uniform ModelUBO\n"
   "{\n"
   "   mat4 mat_model;\n"
   "   mat4 mat_invmodel;\n"
   "   mat4 mat_mvp;\n"
   "   mat4 mat_normal_model_u_color;\n"
   "};\n"
   "float PixelID(vec2 screen_coord)\n"
   "{\n"
   "   screen_coord = floor(screen_coord);\n"
   "   vec2 m2 = mod(screen_coord, 2.0);\n"
   "   return abs(m2.x * 0.667 - m2.y);\n"
   "}\n"
   "float Bayer(vec2 screen_coord)\n"
   "{\n"
   "   float res = PixelID(screen_coord * 0.5) * 0.2;\n"
   "   res += PixelID(screen_coord) * 0.8;\n"
   "   return res;\n"
   "}\n"
   "in vec2 v2f_texcoord;\n"
   "out vec4 frg_color;\n"
   "void main()\n"
   "{\n"
   "   vec4 color = texture(tex_color, v2f_texcoord) * mat_normal_model_u_color[3];\n"
   "   vec3 final_color = color.rgb;\n"
   "   if (color.a < Bayer(vec2(gl_FragCoord.xy)) * 0.1)\n"
   "      discard;\n"
   "   frg_color.rgb = final_color * color.a;\n"
   "   frg_color.a = color.a;\n"
   "}\n";

const char particle_compute_code[] =
   "#version 430 core\n"
   "layout(local_size_x=1, local_size_y=1, local_size_z=1) in;\n"
   "layout(std140, binding=0) uniform GlobalUBO\n"
   "{\n"
   "   float u_time;\n"
   "};\n"
   "layout(std140, binding=1) uniform CameraUBO\n"
   "{\n"
   "   mat4 mat_view;\n"
   "   mat4 mat_proj;\n"
   "   mat4 mat_invview;\n"
   "   mat4 mat_invproj;\n"
   "   vec2 u_near_far;\n"
   "   uvec2 u_screen_size;\n"
   "   vec4 u_proj_info;\n"
   "};\n"
   "layout(std140, binding=2) uniform ModelUBO\n"
   "{\n"
   "   mat4 mat_model;\n"
   "   mat4 mat_invmodel;\n"
   "   mat4 mat_mvp;\n"
   "   mat4 mat_normal_model_u_color;\n"
   "};\n"
   "layout(std430, binding=3) buffer ParticleSSBO\n"
   "{\n"
   "   mat4 mat_particle[];\n"
   "};\n"
   "uint RandomHash(inout uint seed)\n"
   "{\n"
   "   seed ^= seed << 13u;\n"
	"   seed ^= seed >> 17u;\n"
	"   seed ^= seed << 5u;\n"
   "   return seed;\n"
   "}\n"
   "float Random(inout uint seed) { return float(RandomHash(seed)) * (1.0 / float(0xFFFFFFFFu)); }\n"
   "vec4 MakeQuat(vec3 axis, float angle)\n"
   "{\n"
   "   float a = angle * 0.5;\n"
   "   float b = sin(a);\n"
   "   float c = cos(a);\n"
   "   vec4 res = vec4(0.0);\n"
   "   res.xyz = axis * b;\n"
   "   res.w = c;\n"
   "   return res;\n"
   "}\n"
   "vec4 MulQuat(vec4 a, vec4 b)\n"
   "{\n"
   "   vec4 res = vec4(0.0);\n"
   "   res.xyz = (\n"
   "      ((b.xyz * a.w) + (a.xyz * b.w)) +\n"
   "      cross(a.xyz, b.xyz)\n"
   "   );\n"
   "   res.w = a.w * b.w - dot(a.xyz, b.xyz);\n"
   "   return res;\n"
   "}\n"
   "vec4 LookingAt(vec3 origin, vec3 target)\n"
   "{\n"
   "   const vec3 front = vec3(0, 0,-1);\n"
   "   const vec3 up = vec3(0, 1, 0);\n"
   "   vec3 pointing_dir = normalize(target - origin);\n"
   "   vec3 tan_dir = normalize(pointing_dir - (up * dot(pointing_dir, up)));\n"
   "   vec4 yaw_q = vec4(0, 0, 0, 1);\n"
   "   vec4 pitch_q = vec4(0, 0, 0, 1);\n"
   "   vec3 yaw_axis = normalize(cross(front, tan_dir));\n"
   "   if (dot(yaw_axis, yaw_axis) >= 0.000001)\n"
   "   {\n"
   "      float cos_angle = dot(front, tan_dir);\n"
   "      float angle = -acos(cos_angle);\n"
   "      yaw_q = MakeQuat(yaw_axis, angle);\n"
   "   } else {\n"
   "      yaw_axis = up;\n"
   "      float cos_angle = dot(front, tan_dir);\n"
   "      float angle = -acos(cos_angle);\n"
   "      yaw_q = MakeQuat(yaw_axis, angle);\n"
   "   }\n"
   "   if (abs(dot(pointing_dir, up)) > 0.001)\n"
   "   {\n"
   "      vec3 pitch_axis = normalize(cross(yaw_axis, tan_dir));\n"
   "      if (dot(pitch_axis, pitch_axis) > 0.0001)\n"
   "      {\n"
   "         pitch_axis = pitch_axis * sign(dot(pointing_dir, yaw_axis));\n"
   "         float cos_angle = dot(pointing_dir, tan_dir);\n"
   "         float angle = acos(cos_angle);\n"
   "         pitch_q = MakeQuat(pitch_axis, angle);\n"
   "      }\n"
   "   }\n"
   "   return normalize(MulQuat(yaw_q, pitch_q));\n"
   "}\n"
   "mat3 QuatToMat3(vec4 quaternion)\n"
   "{\n"
   "   float n = dot(quaternion, quaternion);\n"
   "   float s = 2.0f * ((n > 0.0) ? (1.0 / n) : 0.0);\n"
   "   float x = quaternion.x;\n"
   "   float y = quaternion.y;\n"
   "   float z = quaternion.z;\n"
   "   float w = quaternion.w;\n"
   "   return mat3(\n"
   "      1.0-(y*y + z*z) * s, (x*y - w*z) * s, (x*z + w*y) * s,\n"
   "      (x*y + w*z) * s, 1.0-(x*x + z*z) * s, (y*z - w*x) * s,\n"
   "      (x*z - w*y) * s, (y*z + w*x) * s, 1.0-(x*x + y*y) * s\n"
   "   );\n"
   "}\n"
   "mat3 LookingAtMatrix(vec3 origin, vec3 target)\n"
   "{\n"
   "   return QuatToMat3(LookingAt(origin, target));\n"
   "}\n"
   "vec3 GetOrigin(mat4 matrix)\n"
   "{\n"
   "   return (matrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;\n"
   "}\n"
   "mat3 TransposeBasis(mat4 matrix)\n"
   "{\n"
   "   return mat3(\n"
   "      vec3(matrix[0].x, matrix[1].x, matrix[2].x),\n"
   "      vec3(matrix[0].y, matrix[1].y, matrix[2].y),\n"
   "      vec3(matrix[0].z, matrix[1].z, matrix[2].z)\n"
   "   );\n"
   "}\n"
   "mat4 NormalizedBasis(mat4 matrix)\n"
   "{\n"
   "   matrix[0].xyz = normalize(matrix[0].xyz);\n"
   "   matrix[1].xyz = normalize(matrix[1].xyz);\n"
   "   matrix[2].xyz = normalize(matrix[2].xyz);\n"
   "   return matrix;\n"
   "}\n"
   "void main()\n"
   "{\n"
   "   uint seed = (gl_WorkGroupID.x * 3 + 1);\n"
   "   float timer = u_time * 0.001;\n"
   "   float x = (sin(Random(seed) * 128.0 + timer * 11.0) *-cos(Random(seed) * 128.0 + timer * 25.0));\n"
   "   float y = (sin(Random(seed) * 128.0 + timer * 17.0) * sin(Random(seed) * 128.0 + timer * 21.0));\n"
   "   float z = (cos(Random(seed) * 128.0 + timer * 13.0) * sin(Random(seed) * 128.0 + timer * 24.0));\n"
   "   float len = (inversesqrt(x*x + y*y + z*z) * 3.0 + 180.0 * Random(seed));\n"
   "   x *= len;\n"
   "   y *= len;\n"
   "   z *= len;\n"

   "   mat4 model = NormalizedBasis(mat_model);\n"
   "   vec3 particle = GetOrigin(model) + mat3(model) * vec3(x, y, z);\n"
   "   vec3 camera = GetOrigin(mat_invview);\n"
   "   float view_depth = -(mat_view * vec4(particle, 1.0)).z;\n"
   "   float z_scaler = clamp(mix(view_depth, 1.0, 0.98), 0.0, 5.0);\n"
   "   mat3 model_scale = TransposeBasis(model) * mat3(mat_model) * z_scaler;\n"
   "   model = mat4(LookingAtMatrix(particle, camera) * model_scale);\n"
   "   model[3].xyz = particle;\n"

   "   mat_particle[gl_WorkGroupID.x] = model;\n"
   "}\n";

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   Engine_SetRawMouseInput(engine, true);
   Engine_SetMouseMode(engine, MOUSE_DISABLE_CURSOR);

   Module_Defaults(engine, 0, NULL);

   Graphics* graphics = Engine_FetchModule(engine, "graphics");
   Graphics_SetClearColor(graphics, (color8){ 25, 25, 25, 255 });

   Renderer* renderer = Engine_FetchModule(engine, "renderer");

   Renderer_RegisterDrawableType(renderer, "ParticleDrawable", &(DrawableTypeDesc){
      .data_size = sizeof(ParticleDrawable),
      .render_func = ParticleRenderFunc
   });

   // NOTE: all angles are in half-turns (50 == 90 degrees, 100 == 180, etc...)
   struct {
      vec3 origin;
      vec3 euler;
      const f32 move_speed;
      const f32 look_speed;
      const Key key_forward;
      const Key key_backward;
      const Key key_left;
      const Key key_right;
   } player = {
      VEC3(0, 1, 3),
      VEC3(0, 0, 0),
      8,
      0.1f,
      KEY_W,
      KEY_S,
      KEY_A,
      KEY_D
   };

   i32 directory_length = strnlen(argv[0], 2048);
   while (directory_length > 0)
   {
      char directory_char = argv[0][--directory_length];
      if (directory_char == '/' || directory_char == '\\')
         break;
   }

   const char* image_name = "assets/textures/bubble.png";
   i32 image_path_length = snprintf(NULL, 0, "%.*s/%s", directory_length, argv[0], image_name) + 1;
   char* image_path = malloc(image_path_length * sizeof(char));
   snprintf(image_path, image_path_length, "%.*s/%s", directory_length, argv[0], image_name);
   printf("%s\n", image_path);

   resolution2d image_size = { 0 };
   int image_channel_count = 0;

   u8* image = stbi_load(image_path, &image_size.width, &image_size.height, &image_channel_count, 4);

   Drawable floor_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* floor_data = Renderer_DrawableData(renderer, floor_object);
   floor_data->geometry = Renderer_PlaneGeometry(renderer);
   floor_data->color.hex = 0xFF888888;
   floor_data->transform.scale = VEC3(10, 1, 10);
   floor_data->transform.rotation = Util_IdentityQuat();
   floor_data->transform.origin.y -= 1.0f;
   floor_data->material.shader = Renderer_BasicShader(renderer);

   Drawable box_object = Renderer_CreateDrawable(renderer, GEOMETRY_DRAWABLE_TYPE);
   GeometryDrawable* box_data = Renderer_DrawableData(renderer, box_object);
   box_data->geometry = Renderer_BoxGeometry(renderer);
   box_data->color.hex = 0xFF000088;
   box_data->transform.scale = VEC3(0.6f, 0.6f, 0.6f);
   box_data->transform.rotation = Util_IdentityQuat();
   box_data->transform.origin.y -= 0.6f;
   box_data->material.shader = Renderer_BasicShader(renderer);

   Drawable particle_object = Renderer_CreateDrawable(renderer, "ParticleDrawable");
   ParticleDrawable* particle_data = Renderer_DrawableData(renderer, particle_object);
   particle_data->particle_count = 16000;
   particle_data->geometry = Plane(graphics);
   particle_data->color.hex = 0xFFFFFFFF;
   particle_data->transform.scale = VEC3(0.08f, 0.08f, 1.0f);
   particle_data->transform.rotation = Util_MakeQuat(VEC3(0, 1, 0), 25.0f);
   particle_data->transform.origin.y += 1.8f;
   particle_data->shader = Graphics_CreateShader(graphics, particle_vertex_code, particle_fragment_code);
   particle_data->color_texture = Graphics_CreateTexture(graphics, image, (TextureDesc){ image_size, 1, 1, GFX_TEXTURETYPE_2D, GFX_TEXTUREFORMAT_RGBA_U8_NORM });
   particle_data->particle_shader = Graphics_CreateComputeShader(graphics, particle_compute_code);
   particle_data->particle_ssbo = Graphics_CreateBuffer(graphics, NULL, particle_data->particle_count, sizeof(mat4x4), GFX_DRAWMODE_STATIC_COPY, GFX_BUFFERTYPE_STORAGE);

   Graphics_GenerateTextureMipmaps(graphics, particle_data->color_texture);

   Buffer global_ubo = Graphics_CreateBuffer(graphics, NULL, 1, sizeof(f32), GFX_DRAWMODE_DYNAMIC, GFX_BUFFERTYPE_UNIFORM);
   
   f32 global_timer = 0.0f;
   f64 fps_timer = 0.0f;
   u32 frames_rendered = 0;

   stbi_image_free(image);

   while(!Engine_CheckExitConditions(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_RequestExit(engine);

      f64 frame_delta = Engine_GetFrameDelta(engine);
      vec2 mouse_delta = Engine_GetMouseDelta(engine);

      particle_data->transform.rotation = Util_MulQuat(Util_MakeQuat(VEC3(0, 1, 0), 1.0f * (f32)frame_delta), particle_data->transform.rotation);

      global_timer += (f32)frame_delta;
      Graphics_UpdateBuffer(graphics, global_ubo, &global_timer, 1, sizeof(f32));

      {
         player.euler.y -= mouse_delta.x * player.look_speed;
         player.euler.y += (player.euler.y > 200) ?-200 : ((player.euler.y < 0) ? 200 : 0);
         player.euler.x -= mouse_delta.y * player.look_speed;
         player.euler.x = M_MAX(-50, M_MIN( 50, player.euler.x));

         f32 yaw_sin = M_SIN(player.euler.y);
         f32 yaw_cos = M_COS(player.euler.y);

         vec3 move_vec = VEC3(0, 0, 0);
         if (Engine_CheckKey(engine, player.key_forward, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3(-yaw_sin, 0,-yaw_cos)
            );
         }

         if (Engine_CheckKey(engine, player.key_backward, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3( yaw_sin, 0, yaw_cos)
            );
         }

         if (Engine_CheckKey(engine, player.key_left, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3(-yaw_cos, 0, yaw_sin)
            );
         }

         if (Engine_CheckKey(engine, player.key_right, KEY_IS_DOWN))
         {
            move_vec = Util_AddVec3(
               move_vec,
               VEC3( yaw_cos, 0,-yaw_sin)
            );
         }

         move_vec = Util_NormalizeVec3(move_vec);
         player.origin = Util_AddVec3(
            player.origin,
            Util_ScaleVec3(move_vec, player.move_speed * (f32)frame_delta)
         );
      }

      resolution2d size = Engine_GetFrameSize(engine);
      Graphics_Viewport(graphics, size);

      Graphics_UseBuffer(graphics, global_ubo, 0);

      Renderer_SetViewAndProjectionMatrix(
         renderer,
         Util_ViewMatrix(player.origin, player.euler, 0.0f),
         Util_PerspectiveMatrix(50.0f, (f32)size.height / (f32)size.width, 0.05f, 100.0f)
      );

      Renderer_Render(renderer, size);
      
      Engine_Present(engine);

      fps_timer += frame_delta;

      frames_rendered++;
      if (fps_timer >= 1.0f)
      {
         printf("%u fps\n", frames_rendered);
         frames_rendered = 0;
         fps_timer -= 1.0f;
      }
   }

   Engine_Free(engine);
   return 0;
}

void ParticleRenderFunc(Renderer* renderer, Drawable self)
{
   ParticleDrawable* drawable_data = Renderer_DrawableData(renderer, self);

   Graphics* graphics = Renderer_Graphics(renderer);
   Buffer camera_buffer = Renderer_CameraBuffer(renderer);
   Buffer model_buffer = Renderer_ModelBuffer(renderer);
   mat4x4 view_projection = Renderer_GetViewAndProjectionMatrix(renderer);

   ModelData model_data = { 0 };
   model_data.mat_model = Util_TransformationMatrix(drawable_data->transform);
   model_data.mat_invmodel = Util_InverseMat4(model_data.mat_model);
   model_data.mat_mvp = Util_MulMat4(view_projection, model_data.mat_model);
   model_data.u_color = Util_Vec4FromColor(drawable_data->color);

   mat4x4 mat_normal_model = Util_TransposeMat4(model_data.mat_invmodel);
   model_data.mat_normal_model[0].xyz = mat_normal_model.v[0].xyz;
   model_data.mat_normal_model[1].xyz = mat_normal_model.v[1].xyz;
   model_data.mat_normal_model[2].xyz = mat_normal_model.v[2].xyz;

   Graphics_SetBlending(graphics, GFX_BLENDMODE_ADD);

   Graphics_UpdateBuffer(graphics, model_buffer, &model_data, 1, sizeof(ModelData));
   Graphics_UseBuffer(graphics, model_buffer, 2);

   Graphics_UseBuffer(graphics, camera_buffer, 1);

   Graphics_UseBuffer(graphics, drawable_data->particle_ssbo, 3);

   Graphics_Dispatch(graphics, drawable_data->particle_shader, drawable_data->particle_count, 1, 1, (UniformBlockList){ .count = 0 });
   Graphics_DispatchBarrier();

   Graphics_SetTextureInterpolation(graphics, drawable_data->color_texture, (TextureInterpolation){ 1, GFX_TEXTUREFILTER_BILINEAR_LINEAR_MIPMAPS, GFX_TEXTUREWRAP_CLAMP });
   Graphics_BindTexture(graphics, drawable_data->color_texture, 0);

   Graphics_DrawInstanced(graphics, drawable_data->shader, drawable_data->geometry, drawable_data->particle_count, (UniformBlockList){ .count = 0 });

   Graphics_SetBlending(graphics, GFX_BLENDMODE_NONE);
}

Geometry Plane(Graphics* graphics)
{
   Mesh mesh = Mesh_EmptyMesh(MESH_PRIMITIVE_TRIANGLE);
   mesh.attribute_count = 3;
   mesh.attributes[0] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[1] = MESH_ATTRIBUTE_3_CHANNEL;
   mesh.attributes[2] = MESH_ATTRIBUTE_2_CHANNEL;
   
   MeshInterface mesh_interface = Mesh_NewInterface(&mesh);

   mat4x4 t = Util_MulMat4(Util_RotationMatrix(VEC3(1, 0, 0), 50), Util_ScalingMatrix(VEC3(2, 1, 2)));
   mesh_interface = Mesh_AddQuad(1, 1, t, mesh_interface);
   mesh_interface = Mesh_GenNormals(mesh_interface);
   mesh_interface = Mesh_GenTexcoords(mesh_interface);

   Geometry plane = Graphics_CreateGeometry(graphics, mesh, GFX_DRAWMODE_STATIC);
   Mesh_Free(&mesh);

   return plane;
}
