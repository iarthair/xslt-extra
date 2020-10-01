#ifndef _luaxml_h
#define _luaxml_h

#ifdef NOLIBXML
#define xmlNode void
#define xmlNodeSet void
#endif

int luaopen_xml (lua_State *lua);
void doc_mark_gc (xmlDoc *doc);
int luaX_pushdoc (lua_State *lua, xmlDoc *doc);
xmlDoc *luaX_checkdoc (lua_State *lua, int index);
xmlDoc *luaX_todoc (lua_State *lua, int index);
int luaX_pushnode (lua_State *lua, xmlNode *node);
xmlNode *luaX_checknode (lua_State *lua, int index);
xmlNode *luaX_tonode (lua_State *lua, int index);
xmlNode *luaX_toattrs (lua_State *lua, int index);
int luaX_pushattrs (lua_State *lua, xmlNode *node);

int luaX_pushnodeset (lua_State *lua, struct _xmlNodeSet *nodeset);
struct _xmlNodeSet *luaX_tonodeset (lua_State *lua, int index);
struct _xmlNodeSet *luaX_checknodeset (lua_State *lua, int index);

#endif
