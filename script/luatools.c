#include <assert.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "luatools.h"

/*****************************************************************************
 * Pointer wrapping
 *****************************************************************************/
struct wrap
  {
    void *ptr;
  };

int
luatool_wrap_unique (lua_State *lua, void *ptr, const char *type)
{
  struct wrap *wrap;

  if (ptr == NULL)
    {
      lua_pushnil (lua);
      return 1;
    }

  lua_getfield (lua, LUA_REGISTRYINDEX, "luaX.wrappers");
  if (lua_isnil (lua, -1))
    {
      lua_pop (lua, 1);
      lua_newtable (lua);
      lua_pushstring (lua, "kv");
      lua_setfield (lua, -2, "__mode");
      lua_pushvalue (lua, -1);
      lua_setfield (lua, LUA_REGISTRYINDEX, "luaX.wrappers");
    }

  lua_pushlightuserdata (lua, ptr);
  lua_gettable (lua, -2);
  if (!lua_isnil (lua, -1))
    {
      wrap = luatool_checkudata (lua, -1, type);
      assert (wrap->ptr == ptr);
    }
  else
    {
      /* pop nil */
      lua_pop (lua, 1);

      /* wrap pointer */
      wrap = lua_newuserdata (lua, sizeof (struct wrap));
      wrap->ptr = ptr;
      luaL_newmetatable (lua, type);
      lua_setmetatable (lua, -2);

      /* duplicate wrapper at TOS and store in wrappers table */
      lua_pushlightuserdata (lua, ptr);
      lua_pushvalue (lua, -2);
      lua_settable (lua, -4);
    }
  /* delete wrappers table from stack leaving wrapped pointer at TOS */
  lua_replace (lua, -2);
  return 1;
}

int
luatool_wrap (lua_State *lua, void *ptr, const char *type)
{
  struct wrap *wrap;

  if (ptr == NULL)
    {
      lua_pushnil (lua);
      return 1;
    }

  /* wrap pointer */
  wrap = lua_newuserdata (lua, sizeof (struct wrap));
  wrap->ptr = ptr;
  luaL_getmetatable (lua, type);
  lua_setmetatable (lua, -2);
  return 1;
}

void *
luatool_checktype (lua_State *lua, int index, const char *type)
{
  struct wrap *wrap;

  wrap = luaL_checkudata (lua, index, type);
  luaL_argcheck (lua, wrap->ptr != NULL, index, NULL);
  return wrap->ptr;
}

void *
luatool_totype (lua_State *lua, int index, const char *type)
{
  struct wrap *wrap;

  wrap = luatool_checkudata (lua, index, type);
  return wrap != NULL ? wrap->ptr : NULL;
}

void *
luatool_checkudata (lua_State *lua, int index, const char *type)
{
  void *udata = lua_touserdata (lua, index);
  int eq;

  if (udata != NULL && lua_getmetatable (lua, index))
    {
      luaL_getmetatable (lua, type);
      eq = lua_rawequal (lua, -1, -2);
      lua_pop (lua, 2);
      return eq ? udata : NULL;
    }
  return NULL;
}

