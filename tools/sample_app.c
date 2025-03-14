#include <stdio.h>
#include <util/types.h>
#include <util/math.h>
#include <util/vec2.h>
#include <util/keymap.h>
#include <engine.h>

#include <glad/gl.h>

const char* vrtshd_fullscreen = "#version 430\n"
"out vec2 v2f_texcoord;\n"
"void main()\n"
"{\n"
"   vec2 vertex = vec2(gl_VertexID % 2, gl_VertexID / 2) * 4.0 - 1.0;\n"
"   v2f_texcoord = vertex * vec2(0.5,-0.5);\n"
"   gl_Position = vec4(vertex, 0.0, 1.0);\n"
"}\n"
;

const char* frgshd_fullscreen = "#version 430\n"
"in vec2 v2f_texcoord;\n"
"layout(location=0) uniform vec4 u_view_rect;\n"
"layout(location=1) uniform vec2 u_screen_size;\n"
"out vec4 frg_out;\n"
"float smooth_lines(vec2 coord, float thickness)\n"
"{\n"
"   vec2 dist = (abs(coord) - (thickness * u_view_rect.zw)) / u_view_rect.zw;\n"
"   return smoothstep(0.5, 0.0, min(dist.x, dist.y));\n"
"}\n"
"float smooth_grid(vec2 coord, vec2 cell_size, float thickness)\n"
"{\n"
"   vec2 cell_coord = abs(mod(coord + cell_size * 0.5, cell_size)) - cell_size * 0.5;\n"
"   vec2 s = (cell_size / u_view_rect.zw);\n"
"   float w = clamp(min(s.x, s.y) - 5.0, 0.0, 1.0);\n"
"   return smooth_lines(cell_coord, thickness) * w;\n"
"}\n"
"void main()\n"
"{\n"
"   const vec4 grid_col = vec4(0.3, 0.3, 0.28, 0.5);\n"
"   const vec4 subgrid_col = vec4(0.25, 0.2, 0.2, 0.4);\n"
"   const vec4 axis_col = vec4(0.7, 0.65, 0.3, 0.4);\n"
"   vec2 screen_coord =  u_view_rect.zw * trunc(v2f_texcoord * u_screen_size - u_view_rect.xy);\n"
"   float grid_mask[3] = float[](\n"
"      smooth_grid(screen_coord, vec2(8.0), 0.25),\n"
"      smooth_grid(screen_coord, vec2(128.0), 0.5),\n"
"      smooth_lines(screen_coord, 0.75)\n"
"   );\n"
"   frg_out = mix(vec4(0), subgrid_col, grid_mask[0]);\n"
"   frg_out = mix(frg_out, grid_col, grid_mask[1]);\n"
"   frg_out = mix(frg_out, axis_col, grid_mask[2]);\n"
"}\n"
;

int main(int argc, char* argv[])
{
   Engine* engine = Engine_Init(
      &(EngineDesc){ .app_name = "Game", .window.title = "Game Window" }
   );

   u32 shd_vrt = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(shd_vrt, 1, &vrtshd_fullscreen, NULL);
   glCompileShader(shd_vrt);

   i32 vrt_sucess = 1;
   glGetShaderiv(shd_vrt, GL_COMPILE_STATUS, &vrt_sucess);
   if (!vrt_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetShaderInfoLog(shd_vrt, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "[VERT]\n%s\n", shd_log);
   }

   u32 shd_frg = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(shd_frg, 1, &frgshd_fullscreen, NULL);
   glCompileShader(shd_frg);

   i32 frg_sucess = 1;
   glGetShaderiv(shd_frg, GL_COMPILE_STATUS, &frg_sucess);
   if (!frg_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetShaderInfoLog(shd_frg, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "[FRAG]\n%s\n", shd_log);
   }

   u32 shd = glCreateProgram();
   glAttachShader(shd, shd_vrt);
   glAttachShader(shd, shd_frg);
   glLinkProgram(shd);

   i32 shd_sucess = 1;
   glGetProgramiv(shd, GL_LINK_STATUS, &shd_sucess);
   if (!shd_sucess)
   {
      char shd_log[1024] = { 0 };
      glGetProgramInfoLog(shd, sizeof(shd_log), NULL, shd_log);
      fprintf(stderr, "%s\n", shd_log);
   }

   glDeleteShader(shd_vrt);
   glDeleteShader(shd_frg);

   u32 vao;
   glGenVertexArrays(1, &vao);

   glUseProgram(shd);

   vec2 offset = VEC2(0, 0);
   f32 zoom = 1.0;
   while(!Engine_ShouldQuit(engine))
   {
      if (Engine_CheckKey(engine, KEY_ESCAPE, KEY_IS_DOWN))
         Engine_Quit(engine);

      if (Engine_CheckKey(engine, KEY_E, KEY_IS_DOWN))
      {
         offset = Util_AddVec2(offset, Engine_GetMouseDelta(engine));
      }

      if (Engine_CheckKey(engine, KEY_W, KEY_IS_DOWN))
      {
         zoom = M_MIN(zoom + zoom * 0.001f, 8.0f);
      }

      if (Engine_CheckKey(engine, KEY_S, KEY_IS_DOWN))
      {
         zoom = M_MAX(zoom - zoom * 0.001f, 0.125f);
      }
      
      size2i screen_size = Engine_GetSize(engine);
      // fprintf(stdout, "%i %i\n", screen_size.width, screen_size.height);
      glViewport(0, 0, screen_size.width, screen_size.height);

      glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glUniform4fv(0, 1, (f32[4]){ offset.x, offset.y, zoom, zoom });
      glUniform2fv(1, 1, (f32[2]){ (f32)screen_size.width, (f32)screen_size.height });
      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLES, 0, 3);

      Engine_Present(engine);
   }

   glDeleteProgram(shd);
   glDeleteVertexArrays(1, &vao);

   Engine_Free(engine);
   return 0;
}
