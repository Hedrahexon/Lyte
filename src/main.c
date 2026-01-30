#include <stdlib.h>
#include <stdio.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "api/api.h"
#include "renderer.h"

#ifdef _WIN32
  #include <windows.h>
#elif __linux__
  #include <unistd.h>
#elif __APPLE__
  #include <mach-o/dyld.h>
#endif


SDL_Window *window;


// NOTE: WARNING: HiDPI support is NOT YET TESTED because I do not
//       currently have access to such a monitor.
// NOTE: from the docs, we should handle SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED
//       for supporting moving a window between monitors with different DPI values
// References:
// - https://wiki.libsdl.org/SDL3/SDL_GetDisplayContentScale
// - https://wiki.libsdl.org/SDL3/README-highdpi
static double get_scale(void) {
  // NOTE: modified when porting to SDL3 (because the previous code, calling
  //       SDL_GetDisplayDPI() would not compile anymore.
  float val = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  if (val == 0.f)
  {
    fprintf(stderr, "%s\n", SDL_GetError());
    return 1.0;
  }
  return (double)val;
}


static void get_exe_filename(char *buf, int sz) {
#if _WIN32
  int len = GetModuleFileName(NULL, buf, sz - 1);
  buf[len] = '\0';
#elif __linux__
  char path[512];
  sprintf(path, "/proc/%d/exe", getpid());
  int len = readlink(path, buf, sz - 1);
  buf[len] = '\0';
#elif __APPLE__
  unsigned size = sz;
  _NSGetExecutablePath(buf, &size);
#else
  strcpy(buf, "./lyte");
#endif
}


static void init_window_icon(void) {
#ifndef _WIN32
  #include "../icon.inl"
  SDL_PixelFormat pxformat = SDL_GetPixelFormatForMasks(32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
  SDL_Surface *surf = SDL_CreateSurfaceFrom(64, 64, pxformat, icon_rgba, 64*4);
  SDL_SetWindowIcon(window, surf);
  SDL_DestroySurface(surf);
#endif
}


int main(int argc, char **argv) {
#ifdef _WIN32
  HINSTANCE lib = LoadLibrary("user32.dll");
  int (*SetProcessDPIAware)() = (void*) GetProcAddress(lib, "SetProcessDPIAware");
  SetProcessDPIAware();
#endif

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  SDL_EnableScreenSaver();
  SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
  atexit(SDL_Quit);

  SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
  SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

  const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());

  window = SDL_CreateWindow("", (int)(dm->w * 0.8), (int)(dm->h * 0.8),
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
  init_window_icon();
  ren_init(window);

  // NOTE: added when porting to SDL3
  //   This call is required in SDL3 for Lite/Lyte's usage (It needs TextInput events and I tested original lite to confirm that they are sent with each letter KEYDOWN)
  //   (It seems that it was automatically called in SDL2, or where was that call ?)
  if (!SDL_StartTextInput(window))
  {
#ifndef _WIN32 // because win32 apps may not have a console ?
    fputs("ERROR: failed SDL_StartTextInput() - text entry may not work ! TODO: implement a fallback in Lua code, by using KEY_DOWN instead.\n", stderr);
#endif
  }

  lua_State *L = luaL_newstate();
  luaL_openlibs(L);
  api_load_libs(L);


  lua_newtable(L);
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i + 1);
  }
  lua_setglobal(L, "ARGS");

  lua_pushstring(L, "1.11");
  lua_setglobal(L, "VERSION");

  lua_pushstring(L, SDL_GetPlatform());
  lua_setglobal(L, "PLATFORM");

  lua_pushnumber(L, get_scale());
  lua_setglobal(L, "SCALE");

  char exename[2048];
  get_exe_filename(exename, sizeof(exename));
  lua_pushstring(L, exename);
  lua_setglobal(L, "EXEFILE");


  (void) luaL_dostring(L,
  "local core\n"
  "xpcall(function()\n"
  "  SCALE = tonumber(os.getenv(\"LYTE_SCALE\")) or SCALE\n"
  "  PATHSEP = package.config:sub(1, 1)\n"
  "  EXEDIR = EXEFILE:match(\"^(.+)[/\\\\].*$\")\n"
  "  local function dir_exists(path)\n"
  "    local f = io.open(path .. '/core/init.lua', 'r')\n"
  "    if f then f:close(); return true end\n"
  "    return false\n"
  "  end\n"
  "  DATADIR = '/usr/share/lyte'\n"
  "  if not dir_exists(DATADIR) then\n"
  "    DATADIR = EXEDIR .. '/data'\n"
  "  end\n"
  "  package.path = DATADIR .. '/?.lua;' .. package.path\n"
  "  package.path = DATADIR .. '/?/init.lua;' .. package.path\n"
  "  core = require('core')\n"
  "  core.init()\n"
  "  core.run()\n"
  "end, function(err)\n"
  "  print('Error: ' .. tostring(err))\n"
  "  print(debug.traceback(nil, 2))\n"
  "  if core and core.on_error then\n"
  "    pcall(core.on_error, err)\n"
  "  end\n"
  "  os.exit(1)\n"
  "end)");


  lua_close(L);
  SDL_DestroyWindow(window);

  return EXIT_SUCCESS;
}
