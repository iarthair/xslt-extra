#ifndef _luatools_h
#define _luatools_h

void luatool_require (lua_State *lua, const char *module);

#define luatool_wrap_pointer(l,p,t) luatool_wrap_unique((l),(p),(t))
int luatool_wrap_unique (lua_State *lua, void *ptr, const char *type);
int luatool_wrap (lua_State *lua, void *ptr, const char *type);
void *luatool_checktype (lua_State *lua, int index, const char *type);
void *luatool_totype (lua_State *lua, int index, const char *type);

void *luatool_checkudata (lua_State *lua, int index, const char *type);

#endif
