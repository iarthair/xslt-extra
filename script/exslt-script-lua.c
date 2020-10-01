#include <string.h>
#include <stdlib.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltutils.h>

#define cX              (const xmlChar *)

#include "script.h"

#define _unused      __attribute__((unused))

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua-xml.h"

static script_t *init_lua (const implementation_t *implementation);
static void compile_lua (const script_t *script, const char *uri, readcb_t reader, void *arg);
static int call_lua (const script_t *script, const char *uri, const char *function,
		     xmlXPathParserContext *ctxt, int nargs);
static void destroy_lua (const script_t *script);
static void register_extra (lua_State *lua);

static const struct implementation lua_implementation =
  {
    init_lua, compile_lua, call_lua, destroy_lua,
  };

//XXX - real version would load modules etc.
const implementation_t *
script_lookup_language (const char *language)
{
  return strcmp (language, "Lua") == 0 ? &lua_implementation : NULL;
}

/*****************************************************************************
 * Create & destroy the Lua context
 *****************************************************************************/

static int
register_libxslt (lua_State *lua)
{
  luaopen_xml (lua);
  register_extra (lua);
  return 1;
}

static script_t *
init_lua (const implementation_t *implementation)
{
  lua_State *lua;
  script_t *script;

  if ((script = malloc (sizeof (script_t))) == NULL
      || (lua = luaL_newstate ()) == NULL)
    {
      xsltGenericError (xsltGenericErrorContext, "Lua Initialisation Error\n");
      free (script);
      return NULL;
    }

  luaL_openlibs (lua);
  luaL_requiref (lua, "libxslt", register_libxslt, 0);
  lua_pop (lua, 1);

  script->implementation = (implementation_t *) implementation;
  script->state = lua;
  return script;
}

static void
destroy_lua (const script_t *script)
{
  lua_State *lua = script->state;

  if (lua != NULL)
    lua_close (lua);
}

struct ctx
  {
    readcb_t reader;
    void *arg;
  };

static void
compile_lua (const script_t *script, const char *uri, readcb_t reader, void *arg)
{
  lua_State *lua = script->state;
  int top;
  const char *name;

  top = lua_gettop (lua);

  if (lua_load (lua, (lua_Reader) reader, arg, "<func:script>", NULL) != 0)
    xsltGenericError (xsltGenericErrorContext,
		      "Lua Parse Error: %s\n", lua_tostring (lua, -1));
  else if (lua_pcall (lua, 0, 1, 0) != 0)
    xsltGenericError (xsltGenericErrorContext,
		      "Lua Execute Error: %s\n", lua_tostring (lua, -1));
  else
    {
      lua_pushvalue (lua, -1);
      /* save table */
      lua_setfield (lua, LUA_REGISTRYINDEX, uri);
      /* iterate table */
      lua_pushnil (lua);
      while (lua_next (lua, -2) != 0)
        {
	  /* key at -2, value at -1 */
	  if (lua_isstring (lua, -2) && lua_isfunction (lua, -1))
	    {
	      name = lua_tostring (lua, -2);
	      script_register_hook (name, uri);
	    }
	  lua_pop (lua, 1); /* remove value, keep key */
	}
    }

  lua_settop (lua, top);
}

/*****************************************************************************
 * 
 *****************************************************************************/
static int
call_lua (const script_t *script, const char *uri, const char *function,
	  xmlXPathParserContext *ctxt, int nargs)
{
  lua_State *lua = script->state;
  int i, top;
  xmlXPathObject *obj;
  xmlNodeSet *nodeset;
  xmlNode *node;

  top = lua_gettop (lua);

  /* save the XPath context in the registry */
  lua_pushlightuserdata (lua, ctxt->context);
  lua_setfield (lua, LUA_REGISTRYINDEX, "XPath.ctxt");

  /* Look up Lua function f = REGISTRY[uri][function] and call it */
  lua_getfield (lua, LUA_REGISTRYINDEX, uri);
  lua_pushstring (lua, function);
  lua_gettable (lua, -2);
  if (!lua_isfunction (lua, -1))
    {
      xsltGenericError (xsltGenericErrorContext, "Invalid Lua function\n");
      lua_settop (lua, top);
      return 0;
    }

  /* pop and convert the XPath arguments */
  for (i = 0; i < nargs; i++)
    {
      obj = valuePop (ctxt);
      switch (obj->type)
        {
	case XPATH_NODESET:
	case XPATH_XSLT_TREE:
	  luaX_pushnodeset (lua, xmlXPathNodeSetMerge (NULL, obj->nodesetval));
	  break;
	case XPATH_BOOLEAN:
	  lua_pushboolean (lua, obj->boolval);
	  break;
	case XPATH_NUMBER:
	  lua_pushnumber (lua, obj->floatval);
	  break;
	case XPATH_STRING:
	  lua_pushstring (lua, (char *) obj->stringval);
	  break;
	default:
	  xsltGenericError (xsltGenericErrorContext,
	  		    "pushing unknown argument type %d\n", obj->type);
	  break;
	}
      xmlXPathFreeObject (obj);
    }
  /* reverse arguments on the Lua stack */
  for (i = -nargs; i < -1; i++)
    lua_insert (lua, i);

  //XXX attach the XPath context somehow so that it's available to
  //   the Lua callable C functions.
  /* call the Lua function and convert the result as required */
  if (lua_pcall (lua, nargs, 1, 0) != 0)
    xsltGenericError (xsltGenericErrorContext, "Lua Error: %s\n",
		      lua_tostring (lua, -1));
  else
    switch (lua_type (lua, -1))
      {
      case LUA_TNUMBER:
	valuePush (ctxt, xmlXPathNewFloat (lua_tonumber (lua, -1)));
	break;
      case LUA_TBOOLEAN:
	valuePush (ctxt, xmlXPathNewBoolean (lua_toboolean (lua, -1)));
	break;
      case LUA_TSTRING:
	valuePush (ctxt, xmlXPathNewCString (lua_tostring (lua, -1)));
	break;
      case LUA_TUSERDATA:
        if ((nodeset = luaX_tonodeset (lua, -1)) != NULL)
	  {
	    valuePush (ctxt, xmlXPathWrapNodeSet (
					  xmlXPathNodeSetMerge (NULL, nodeset)
						 ));
	    break;
	  }
	if ((node = luaX_tonode (lua, -1)) != NULL)
	  {
	    valuePush (ctxt, xmlXPathNewNodeSet (node));
	    break;
	  }
        /* ELSE FALL THROUGH */
      default:
	xsltGenericError (xsltGenericErrorContext, "invalid return value %s\n",
			  lua_typename (lua, (lua_type (lua, -1))));
	break;
      }

  lua_pushnil (lua);
  lua_setfield (lua, LUA_REGISTRYINDEX, "XPath.ctxt");

  lua_settop (lua, top);
  return 0;
}

static int
nodeset_current (lua_State *lua)
{
  xmlXPathContext *ctxt;

  lua_getfield (lua, LUA_REGISTRYINDEX, "XPath.ctxt");
  ctxt = lua_touserdata (lua, -1);
  lua_pop (lua, 1);
  return luaX_pushnodeset (lua, xmlXPathNodeSetCreate (ctxt->node));
}

static int
nodeset_position (lua_State *lua)
{
  xmlXPathContext *ctxt;

  lua_getfield (lua, LUA_REGISTRYINDEX, "XPath.ctxt");
  ctxt = lua_touserdata (lua, -1);
  lua_pop (lua, 1);
  lua_pushinteger (lua, ctxt->proximityPosition);
  return 1;
}

static int
nodeset_last (lua_State *lua)
{
  xmlXPathContext *ctxt;

  lua_getfield (lua, LUA_REGISTRYINDEX, "XPath.ctxt");
  ctxt = lua_touserdata (lua, -1);
  lua_pop (lua, 1);
  lua_pushinteger (lua, ctxt->contextSize);
  return 1;
}

static const struct luaL_Reg nodeset_extra_f[] =
  {
    { "current", nodeset_current }, /* ns = nodeset.current() */
    { "position", nodeset_position }, /* pos = nodeset.position() */
    { "last", nodeset_last }, /* pos = nodeset.last() */
    { NULL, NULL }
  };

static void
register_extra (lua_State *lua)
{
  luaL_setfuncs (lua, nodeset_extra_f, 0);
}
