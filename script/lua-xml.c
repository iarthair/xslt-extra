#include <assert.h>
#include <syslog.h>
#include <string.h>
#include <stddef.h>

#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "luatools.h"
#include "lua-xml.h"

#define cX              (const xmlChar *)
#define container_of(ptr, type, member) ( \
      (type *) \
      ((char *)(1 ? (ptr) : &((type *)0)->member) - offsetof(type, member)) \
)

/*****************************************************************************
 	Utility functions
 *****************************************************************************/
static int
userdata_index (lua_State *lua)
{
  const struct luaL_Reg *prop;

  lua_getmetatable (lua, 1);
  lua_pushvalue (lua, 2);
  lua_rawget (lua, -2);
  if (lua_islightuserdata (lua, -1))
    {
      prop = lua_touserdata (lua, -1);
      lua_pop (lua, 2);	/* pop metatable and lightuserdata */
      return (*prop->func) (lua);
    }
  lua_replace (lua, -2); /* remove metatable */
  return 1;
}

static void
luaX_registerprop (lua_State *lua, const struct luaL_Reg *props)
{
  lua_pushcfunction (lua, userdata_index);
  lua_setfield (lua, -2, "__index");
  while (props->name != NULL)
    {
      lua_pushlightuserdata (lua, (void *) props);
      lua_setfield (lua, -2, props->name);
      props++;
    }
}

/*****************************************************************************/

static int
serialize_cb (void *context, const char *buffer, int len)
{
  lua_State *lua = context;

  lua_pushvalue (lua, -1);
  lua_pushlstring (lua, buffer, len);
  lua_call (lua, 1, 0);
  return len;
}

/*****************************************************************************
	Documents
 *****************************************************************************/
xmlDoc *
luaX_checkdoc (lua_State *lua, int index)
{
  return luatool_checktype (lua, index, "luaX.doc");
}

xmlDoc *
luaX_todoc (lua_State *lua, int index)
{
  return luatool_totype (lua, index, "luaX.doc");
}

int
luaX_pushdoc (lua_State *lua, xmlDoc *doc)
{
  return luatool_wrap_unique (lua, doc, "luaX.doc");
}

static void *do_gc = &do_gc;

void
doc_mark_gc (xmlDoc *doc)
{
  doc->_private = &do_gc;
}

static int
doc_name (lua_State *lua)
{
  xmlDoc *doc = luaX_checkdoc (lua, 1);

  lua_pushstring (lua, (const char *) doc->name);
  return 1;
}

static int
doc_url (lua_State *lua)
{
  xmlDoc *doc = luaX_checkdoc (lua, 1);

  lua_pushstring (lua, (const char *) doc->URL);
  return 1;
}

/* usage: doc:serialize(function (text) io.write (text) end) */
static int
doc_serialize (lua_State *lua)
{
  xmlDoc *doc = luaX_todoc (lua, 1);
  xmlSaveCtxt *ctxt;

  if (!lua_isfunction (lua, 2))
    {
      syslog (LOG_ERR, "XML serialize: Invalid callback\n");
      return 0;
    }
  ctxt = xmlSaveToIO (serialize_cb, NULL, lua, "UTF-8", XML_SAVE_FORMAT);
  xmlSaveDoc (ctxt, doc);
  xmlSaveClose (ctxt);
  return 0;
}

static int
doc_setroot (lua_State *lua)
{
  xmlDoc *doc = luaX_checkdoc (lua, 1);
  xmlNode *node = luaX_checknode (lua, 2);

  return luaX_pushnode (lua, xmlDocSetRootElement (doc, node));
}

static int
doc_root (lua_State *lua)
{
  xmlDoc *doc = luaX_checkdoc (lua, 1);

  return luaX_pushnode (lua, xmlDocGetRootElement (doc));
}

static int
doc_node_new (lua_State *lua)
{
  xmlDoc *doc = luaX_checkdoc (lua, 1);
  const char *name;
  xmlNode *node;

  name = lua_tostring (lua, 2);
  node = xmlNewDocNode (doc, NULL, (xmlChar *) name, NULL);
  return luaX_pushnode (lua, node);
}

static int
doc_gc (lua_State *lua)
{
  xmlDoc *doc = luaX_checkdoc (lua, 1);

  if (doc->_private == &do_gc)
    xmlFreeDoc (doc);
  return 1;
}

static const struct luaL_Reg doc_m[] =
  {
    { "__gc", doc_gc }, /* f(t) */
    { "node", doc_node_new }, /* f(t,name) */
    { "serialize", doc_serialize }, /* f(t,func) */
    { "setroot", doc_setroot }, /* f(t,node) */
    { NULL, NULL }
  };

static const struct luaL_Reg doc_p[] =
  {
    { "name", doc_name }, /* string */
    { "root", doc_root }, /* doc */
    { "url", doc_url }, /* string */
    { NULL, NULL }
  };

/*****************************************************************************
 * XPath Node Sets
 *****************************************************************************/
xmlNodeSet *
luaX_checknodeset (lua_State *lua, int index)
{
  return luatool_checktype (lua, index, "luaX.nodeset");
}

xmlNodeSet *
luaX_tonodeset (lua_State *lua, int index)
{
  return luatool_totype (lua, index, "luaX.nodeset");
}

int
luaX_pushnodeset (lua_State *lua, xmlNodeSet *nodeset)
{
  return luatool_wrap (lua, nodeset, "luaX.nodeset");
}

/*****************************************************************************/

static int
nodeset_new (lua_State *lua)
{
  xmlNode *node = luaX_tonode (lua, 1);

  return luaX_pushnodeset (lua, xmlXPathNodeSetCreate (node));
}

static int
nodeset_new_empty (lua_State *lua)
{
  return luaX_pushnodeset (lua, xmlXPathNodeSetCreate (NULL));
}


static int
nodeset_free (lua_State *lua)
{
  xmlNodeSet *set = luaX_checknodeset (lua, 1);

  xmlXPathFreeNodeSet (set);
  return 1;
}

static int
nodeset_tostring (lua_State *lua)
{
  xmlNodeSet *set = luaX_checknodeset (lua, 1);
  char *str;

  str = (char *) xmlXPathCastNodeSetToString (set);
  lua_pushstring (lua, str);
  xmlFree (str);
  return 1;
}

static int
nodeset_merge (lua_State *lua)
{
  xmlNodeSet *lhs = luaX_checknodeset (lua, 1);
  xmlNodeSet *rhs = luaX_checknodeset (lua, 2);
  xmlNodeSet *nodeset;

  nodeset = xmlXPathNodeSetMerge (NULL, lhs);
  return luaX_pushnodeset (lua, xmlXPathNodeSetMerge (nodeset, rhs));
}

static int
nodeset_intersection (lua_State *lua)
{
  xmlNodeSet *lhs = luaX_checknodeset (lua, 1);
  xmlNodeSet *rhs = luaX_checknodeset (lua, 2);

  return luaX_pushnodeset (lua, xmlXPathIntersection (lhs, rhs));
}

static int
nodeset_difference (lua_State *lua)
{
  xmlNodeSet *lhs = luaX_checknodeset (lua, 1);
  xmlNodeSet *rhs = luaX_checknodeset (lua, 2);

  return luaX_pushnodeset (lua, xmlXPathDifference (lhs, rhs));
}

static int
nodeset_size (lua_State *lua)
{
  xmlNodeSet *set = luaX_checknodeset (lua, 1);

  lua_pushnumber (lua, xmlXPathNodeSetGetLength (set));
  return 1;
}

static int
nodeset_sort (lua_State *lua)
{
  xmlNodeSet *set = luaX_checknodeset (lua, 1);
  xmlNodeSet *nodeset;

  nodeset = xmlXPathNodeSetMerge (NULL, set);
  xmlXPathNodeSetSort (nodeset);
  return luaX_pushnodeset (lua, nodeset);
}

static int
nodeset_index (lua_State *lua)
{
  xmlNodeSet *set = luaX_checknodeset (lua, 1);
  int index;
  xmlNode *node;

  if (lua_isnumber (lua, 2))
    {
      index = lua_tointeger (lua, 2);
      node = xmlXPathNodeSetItem (set, index - 1);
      luaL_argcheck (lua, node != NULL, 2, "index out of range");
      return luaX_pushnode (lua, node);
    }

  /* try to find the method function in the metatable */
  lua_getmetatable (lua, 1);
  lua_pushvalue (lua, 2);
  lua_rawget (lua, -2);
  lua_replace (lua, -2);	/* remove unneeded metatable from the stack */
  return 1;
}

static int
nodeset_add (lua_State *lua)
{
  xmlNodeSet *set = luaX_checknodeset (lua, 1);
  xmlNode *node = luaX_checknode (lua, 2);

  xmlXPathNodeSetAddUnique (set, node);
  lua_settop (lua, 1);
  return 1;
}

struct nodeset_iterator
  {
    xmlNodeSet *set;
    int index;
  };

static int
nodeset_iterator (lua_State *lua)
{
  struct nodeset_iterator *nsi;

  nsi = lua_touserdata (lua, lua_upvalueindex (1));
  if (nsi->index >= nsi->set->nodeNr)
    return 0;
  luaX_pushnode (lua, nsi->set->nodeTab[nsi->index++]);
  return 1;
}

static int
nodeset_list (lua_State *lua)
{
  struct nodeset_iterator *nsi;

  nsi = lua_newuserdata (lua, sizeof (struct nodeset_iterator));
  nsi->index = 0;
  nsi->set = luaX_checknodeset (lua, 1);
  lua_pushcclosure (lua, nodeset_iterator, 1);
  return 1;
}


static const struct luaL_Reg nodesetlib_m[] =
  {
    { "__add", nodeset_merge }, /* f(lhs,rhs) */
    { "__gc", nodeset_free }, /* f(op) */
    { "__index", nodeset_index }, /* f(t,key) */
    { "__len", nodeset_size }, /* f(op) */
    { "__mul", nodeset_intersection }, /* f(lhs,rhs) */
    { "__sub", nodeset_difference }, /* f(lhs,rhs) */
    { "__tostring", nodeset_tostring }, /* f(op) */
    { "add", nodeset_add }, /* f(ns,node) */
    { "list", nodeset_list }, /* f(ns,node) */
    { "sort", nodeset_sort }, /* f(ns) */
    { NULL, NULL }
  };

/*****************************************************************************
	Attributes
  NB must offset node pointer otherwise luatool_wrap_unique() will fail.
 *****************************************************************************/

xmlNode *
luaX_toattrs (lua_State *lua, int index)
{
  xmlAttr **attrs = luatool_totype (lua, index, "luaX.attr");

  return container_of (attrs, xmlNode, properties);
}

int
luaX_pushattrs (lua_State *lua, xmlNode *node)
{
  return luatool_wrap_unique (lua, &node->properties, "luaX.attr");
}

/*****************************************************************************/

static int
attr_nth (lua_State *lua)
{
  xmlNode *node = luaX_toattrs (lua, 1);
  int index = lua_tointeger (lua, 2);
  xmlAttr *attr = NULL;

  if (index > 0)
    for (attr = node->properties;
	 attr != NULL && index > 1;
	 attr = attr->next, index--)
      ;
  if (attr != NULL)
    lua_pushstring (lua, (const char *) attr->name);
  else
    lua_pushnil (lua);
  return 1;
}

static int
attr_get (lua_State *lua)
{
  xmlNode *node = luaX_toattrs (lua, 1);
  const char *prop = lua_tostring (lua, 2);
  char *value;

  value = (char *) xmlGetProp (node, (xmlChar *) prop);
  lua_pushstring (lua, value);
  xmlFree (value);
  return 1;
}

static int
attr_set (lua_State *lua)
{
  xmlNode *node = luaX_toattrs (lua, 1);
  const char *prop = lua_tostring (lua, 2);
  const char *value = lua_tostring (lua, 3);

  xmlSetProp (node, (xmlChar *) prop, (xmlChar *) value);
  return 0;
}

static const struct luaL_Reg attr_m[] =
  {
    { "__newindex", attr_set }, /* f(t,attr,value) */
    { "__index", attr_get }, /* f(t,attr) */
    { "nth", attr_nth }, /* f(t,index) */
    { NULL, NULL }
  };

/*****************************************************************************
	Nodes
 *****************************************************************************/
xmlNode *
luaX_checknode (lua_State *lua, int index)
{
  return luatool_checktype (lua, index, "luaX.node");
}

xmlNode *
luaX_tonode (lua_State *lua, int index)
{
  return luatool_totype (lua, index, "luaX.node");
}

int
luaX_pushnode (lua_State *lua, xmlNode *node)
{
  return luatool_wrap_unique (lua, node, "luaX.node");
}

static int
node_tostring (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  char *str;

  str = (char *) xmlNodeGetContent (node);
  lua_pushstring (lua, str);
  xmlFree (str);
  return 1;
}

static int
node_children (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  return luaX_pushnode (lua, node->children);
}

static int
node_last (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  return luaX_pushnode (lua, node->last);
}

static int
node_next (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  return luaX_pushnode (lua, node->next);
}

static int
node_prev (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  return luaX_pushnode (lua, node->prev);
}

static int
node_parent (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  return luaX_pushnode (lua, node->parent);
}

struct node_iterator
  {
    xmlNode *node;
  };

static int
node_iterator (lua_State *lua)
{
  struct node_iterator *nsi;
  xmlNode *node;

  nsi = lua_touserdata (lua, lua_upvalueindex (1));
  node = nsi->node;
  if (nsi->node == NULL)
    return 0;
  nsi->node = node->next;
  luaX_pushnode (lua, node);
  return 1;
}

static int
node_list (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  struct node_iterator *nsi;

  nsi = lua_newuserdata (lua, sizeof (struct node_iterator));
  nsi->node = node->children;
  lua_pushcclosure (lua, node_iterator, 1);
  return 1;
}

static int
node_getattr (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  const char *prop = lua_tostring (lua, 2);
  char *value;

  value = (char *) xmlGetProp (node, (xmlChar *) prop);
  lua_pushstring (lua, value);
  xmlFree (value);
  return 1;
}

static int
node_setattr (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  const char *prop = lua_tostring (lua, 2);
  const char *value = lua_tostring (lua, 3);

  xmlSetProp (node, (xmlChar *) prop, (xmlChar *) value);
  lua_settop (lua, 1);
  return 1;
}

static int
node_unsetattr (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  const char *prop = lua_tostring (lua, 2);

  xmlUnsetProp (node, (xmlChar *) prop);
  lua_settop (lua, 1);
  return 1;
}

static int
node_local_name (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  lua_pushstring (lua, (char *) node->name);
  return 1;
}

static int
node_name (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  xmlNs *ns;

  if ((ns = node->ns) == NULL)
    lua_pushstring (lua, (char *) node->name);
  else
    lua_pushfstring (lua, "%s:%s", (char *) ns->prefix, (char *) node->name);
  return 1;
}

static int
node_prefix (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  xmlNs *ns;

  if ((ns = node->ns) == NULL)
    lua_pushnil (lua);
  else
    lua_pushstring (lua, (char *) ns->prefix);
  return 1;
}

static int
node_namespace_uri (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  xmlNs *ns;

  if ((ns = node->ns) == NULL)
    lua_pushnil (lua);
  else
    lua_pushstring (lua, (char *) ns->href);
  return 1;
}

static int
node_eq (lua_State *lua)
{
  xmlNode *lhs = luaX_checknode (lua, 1);
  xmlNode *rhs = luaX_checknode (lua, 2);

  lua_pushboolean (lua, lhs == rhs);
  return 1;
}

static int
node_lt (lua_State *lua)
{
  xmlNode *lhs = luaX_checknode (lua, 1);
  xmlNode *rhs = luaX_checknode (lua, 2);

  lua_pushboolean (lua, xmlXPathCmpNodes (lhs, rhs) > 0);
  return 1;
}

static int
node_le (lua_State *lua)
{
  xmlNode *lhs = luaX_checknode (lua, 1);
  xmlNode *rhs = luaX_checknode (lua, 2);

  lua_pushboolean (lua, xmlXPathCmpNodes (lhs, rhs) >= 0);
  return 1;
}

static int
node_copy (lua_State *lua)
{
  xmlNode *dst = luaX_checknode (lua, 1);
  xmlNode *src = luaX_checknode (lua, 2);
  int deep = lua_toboolean (lua, 3);
  xmlNode *node;

  node = xmlDocCopyNode (src, dst->doc, !!deep);
  xmlAddChild (dst, node);
  return luaX_pushnode (lua, node);
}

static int
node_replace (lua_State *lua)
{
  xmlNode *dst = luaX_checknode (lua, 1);
  xmlNode *src = luaX_checknode (lua, 2);
  xmlNode *node;

  node = xmlReplaceNode (dst, src);
  return luaX_pushnode (lua, node);
}

static int
node_unlink (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  xmlUnlinkNode (node);
  return 0;
}

static int
node_new (lua_State *lua)
{
  xmlNode *lhs = luaX_checknode (lua, 1);
  const char *name;
  xmlNode *node;

  name = lua_tostring (lua, 2);
  node = xmlNewDocNode (lhs->doc, NULL, (xmlChar *) name, NULL);
  return luaX_pushnode (lua, node);
}

static int
node_text (lua_State *lua)
{
  xmlNode *lhs = luaX_checknode (lua, 1);
  const char *text;
  size_t len;
  xmlNode *node;

  text = lua_tolstring (lua, 2, &len);
  node = xmlNewDocTextLen (lhs->doc, cX text, len);
  xmlAddChild (lhs, node);
  return luaX_pushnode (lua, node);
}

static int
node_append_child (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  xmlNode *child = luaX_checknode (lua, 2);

  xmlAddChild (node, child);
  lua_settop (lua, 1);
  return 1;
}

static int
node_root (lua_State *lua)
{
  xmlNode *src = luaX_checknode (lua, 1);

  return luaX_pushnode (lua, xmlDocGetRootElement (src->doc));
}

static int
node_attr (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  return luaX_pushattrs (lua, node);
}

static int
node_document (lua_State *lua)
{
  xmlNode *src = luaX_checknode (lua, 1);

  //XXX this could leak documents not to be garbage collected
  return luaX_pushdoc (lua, src->doc);
}

/* usage: node:parse("<tag> ...well balanced content... </tag>") */
static int
node_parse_chunk (lua_State *lua)
{
  xmlNode *ref = luaX_tonode (lua, 1);
  const char *value = lua_tostring (lua, 2);
  xmlNode *list = NULL;

  /* value is a well balanced XML fragment */
  if (xmlParseBalancedChunkMemory (ref->doc, NULL, NULL, 0,
  				   cX value, &list) == 0)
    xmlAddChildList (ref, list);
  return 0;
}

/* usage: node:serialize(function (text) io.write (text) end) */
static int
node_serialize (lua_State *lua)
{
  xmlNode *node = luaX_tonode (lua, 1);
  xmlSaveCtxt *ctxt;

  if (!lua_isfunction (lua, 2))
    {
      syslog (LOG_ERR, "XML serialize: Invalid callback\n");
      return 0;
    }
  ctxt = xmlSaveToIO (serialize_cb, NULL, lua, "UTF-8", XML_SAVE_FORMAT);
  xmlSaveTree (ctxt, node);
  xmlSaveClose (ctxt);
  return 0;
}

static int
node_new_attr (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);
  const char *name, *value;

  name = lua_tostring (lua, 2);
  value = lua_tostring (lua, 3);
  xmlNewProp (node, cX name, cX value);
  return 0;
}

static int
node_gc (lua_State *lua)
{
  xmlNode *node = luaX_checknode (lua, 1);

  /* assume only unlinked nodes are to be collected */
  if (node->parent == NULL && node->next == NULL && node->prev == NULL)
    xmlFreeNode (node);
  return 0;
}

/* methods - node:method(...) */
static const struct luaL_Reg node_m[] =
  {
    { "__eq", node_eq }, /* f(lhs,rhs) */
    { "__gc", node_gc }, /* f(t) */
    { "__le", node_le }, /* f(lhs,rhs) */
    { "__lt", node_lt }, /* f(lhs,rhs) */
    { "__tostring", node_tostring }, /* f(op) */
    { "append_child", node_append_child }, /* f(t,child,ref) */
    { "children", node_list }, /* f(t) */
    { "copy", node_copy }, /* f(t,node,[deep]) */
    { "getattr", node_getattr }, /* f(t,attr) */
    { "newattr", node_new_attr }, /* f(t,name,value) */
    { "new", node_new }, /* f(t,name) */
    { "nodeset", nodeset_new }, /* f(t) */
    { "parse", node_parse_chunk }, /* f(t,string) */
    { "replace", node_replace }, /* f(t,node) */
    { "serialize", node_serialize }, /* f(t,func) */
    { "setattr", node_setattr }, /* f(t,attr,value) */
    { "text", node_text }, /* f(t,text) */
    { "unlink", node_unlink }, /* f(t) */
    { "unsetattr", node_unsetattr }, /* f(t,attr) */
    { NULL, NULL }
  };

/* methods - node.prop */
static const struct luaL_Reg node_p[] =
  {
    { "attr", node_attr }, /* attr */
    { "document", node_document }, /* node */
    { "first_child", node_children }, /* node */
    { "last_child", node_last }, /* node */
    { "local_name", node_local_name }, /* string */
    { "name", node_name }, /* string */
    { "namespace_uri", node_namespace_uri }, /* string */
    { "next_sibling", node_next }, /* node */
    { "parent", node_parent }, /* node */
    { "prefix", node_prefix }, /* string */
    { "previous_sibling", node_prev }, /* node */
    { "root", node_root }, /* node */
    { NULL, NULL }
  };

/*****************************************************************************/

static const struct luaL_Reg xmllib_f[] =
  {
    { "nodeset", nodeset_new_empty }, /* nodeset = xmllib.nodelib() */
    { NULL, NULL }
  };

int
luaopen_xml (lua_State *lua)
{
  luaL_newmetatable (lua, "luaX.doc");
  luaL_setfuncs (lua, doc_m, 0);
  luaX_registerprop (lua, doc_p);
  lua_pop (lua, 1);

  luaL_newmetatable (lua, "luaX.node");
  luaL_setfuncs (lua, node_m, 0);
  luaX_registerprop (lua, node_p);
  lua_pop (lua, 1);

  luaL_newmetatable (lua, "luaX.attr");
  luaL_setfuncs (lua, attr_m, 0);
  lua_pop (lua, 1);

  luaL_newmetatable (lua, "luaX.nodeset");
  luaL_setfuncs (lua, nodesetlib_m, 0);
  lua_pop (lua, 1);

  lua_newtable (lua);
  luaL_setfuncs (lua, xmllib_f, 0);
  return 1;
}
