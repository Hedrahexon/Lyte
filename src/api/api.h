#ifndef API_H
#define API_H

#include "lib/lua-5.4.8/lua.h"
#include "lib/lua-5.4.8/lauxlib.h"
#include "lib/lua-5.4.8/lualib.h"

#define API_TYPE_FONT "Font"

void api_load_libs(lua_State *L);

#endif
