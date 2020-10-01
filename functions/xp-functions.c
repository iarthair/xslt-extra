#include <ctype.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/hash.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/imports.h>
#include <libxslt/transform.h>
#include <libxslt/templates.h>
#include <libxslt/extensions.h>
#include <libxml/uri.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "xp-functions.h"

/****************************************************************************
 * string fn:string-join(node-set,string?)
 *	return the concatenation the string values of each node
 *      separated by the string specified in the 1st argument.
 ****************************************************************************/

static void
fn_string_join (xmlXPathParserContextPtr ctxt, int nargs)

{
  xmlChar *separator, *item, *result;
  xmlNodeSet *nodeset;
  int i;

  if (nargs == 1)
    separator = xmlStrdup (cX"");
  else if (nargs == 2)
    separator = xmlXPathPopString (ctxt);
  else
    {
      xmlGenericError (xmlGenericErrorContext,
		       "fn:string-join(node-set,string?)\n");
      ctxt->error = XPATH_INVALID_ARITY;
      return;
    }

  nodeset = xmlXPathPopNodeSet (ctxt);

  if (nodeset == NULL || nodeset->nodeNr < 1)
    result = xmlStrdup (cX"");
  else
    {
      result = xmlNodeGetContent (nodeset->nodeTab[0]);
      for (i = 1; i < nodeset->nodeNr; i++)
	{
	  item = xmlNodeGetContent (nodeset->nodeTab[i]);
	  result = xmlStrcat (result, separator);
	  result = xmlStrcat (result, item);
	  xmlFree (item);
	}
    }

  valuePush (ctxt, xmlXPathNewString (result));

  xmlXPathFreeNodeSet (nodeset);
  xmlFree (separator);
  xmlFree (result);
}

/****************************************************************************
 * boolean fn:ends-with(string, string)
 *   return true if the 2nd argument string is a suffix of the 1st argument.
 ****************************************************************************/

static void
fn_ends_with (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlChar *string, *suffix;
  int string_len, suffix_len;
  int result;

  if (nargs != 2)
    {
      xmlGenericError (xmlGenericErrorContext,
		       "fn:ends-with($string as string,$end as string)\n");
      ctxt->error = XPATH_INVALID_ARITY;
      return;
    }

  suffix = xmlXPathPopString (ctxt);
  string = xmlXPathPopString (ctxt);

  string_len = xmlStrlen (string);
  suffix_len = xmlStrlen (suffix);

  result = suffix_len <= string_len
	    && xmlStrEqual (&string[string_len - suffix_len], suffix);
  valuePush (ctxt, xmlXPathNewBoolean (result));

  xmlFree (suffix);
  xmlFree (string);
}

/****************************************************************************
 * string fn:base-uri(node-set?)
 * return the base URI of the 1st node in node-set or the context node
 * if the argument is omitted.
 ****************************************************************************/

static void
fn_base_uri (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlChar *base;
  xmlNodeSet *nodeset;
  xmlNode *node;

  if (nargs == 0)
    node = ctxt->context->node;
  else if (nargs == 1)
    {
      if ((nodeset = xmlXPathPopNodeSet (ctxt)) == NULL)
        {
	  xmlGenericError (xmlGenericErrorContext,
			   "fn:base-uri(): invalid node-set\n");
	  return;
	}
      if (nodeset->nodeNr < 1)
	{
	  xmlGenericError (xmlGenericErrorContext,
			   "fn:base-uri(): empty node-set\n");
	  xmlXPathFreeNodeSet (nodeset);
	  return;
	}

      node = nodeset->nodeTab[0];
      xmlXPathFreeNodeSet (nodeset);
    }
  else
    {
      xmlGenericError (xmlGenericErrorContext,
		       "fn:base-uri($node as node-set?)\n");
      ctxt->error = XPATH_INVALID_ARITY;
      return;
    }

  if ((base = xmlNodeGetBase (node->doc, node)) == NULL)
    base = xmlStrdup (node->doc->URL);

  valuePush (ctxt, xmlXPathNewString (base));
  xmlFree (base);
}

/****************************************************************************
 * string fn:resolve-uri(string, string?)
 * return the relative URI specified in the 1st argument using the 1st node in
 * node-set or the context node if the 2nd argument is omitted.
 ****************************************************************************/

static void
fn_resolve_uri (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlChar *relative, *base, *uri;
  xmlNode *node;

  if (nargs == 1)
    {
      node = ctxt->context->node;
      if ((base = xmlNodeGetBase (node->doc, node)) == NULL)
        base = xmlStrdup (node->doc->URL);
    }
  else if (nargs == 2)
    base = xmlXPathPopString (ctxt);
  else
    {
      xmlGenericError (xmlGenericErrorContext,
		       "fn:resolve-uri($relative as string,$base as string?)\n");
      ctxt->error = XPATH_INVALID_ARITY;
      return;
    }

  relative = xmlXPathPopString (ctxt);
  uri = xmlBuildURI (relative, base);
  valuePush (ctxt, xmlXPathNewString (uri));

  xmlFree (base);
  xmlFree (relative);
  xmlFree (uri);
}

/****************************************************************************/

static int
class_match (const char *class, const char *name)
{
  const char *p;
  int len;

  if (class != NULL && name != NULL && *name != '\0'
      && (p = strstr (class, name)) != NULL)
    {
      len = strlen (name);
      do
	if ((p == class || isspace (p[-1]))
	    && (p[len] == '\0' || isspace (p[len])))
	  return 1;
      while ((p = strstr (p + len, name)) != NULL);
    }
  return 0;
}

/****************************************************************************
 * boolean str:class-match(string,string)
 *	return true if the 2nd argument matches any of the space-separated
 *	tokens in the 1st argument.
 ****************************************************************************/

static void
fn_class_match (xmlXPathParserContextPtr ctxt, int nargs)
{
  char *name, *class;
  int ret;

  if (nargs != 2)
    {
      xmlGenericError (xmlGenericErrorContext,
		       "str:class-match requires 2 arguments\n");
      ctxt->error = XPATH_INVALID_ARITY;
      return;
    }

  name = (char *) xmlXPathPopString (ctxt);
  class = (char *) xmlXPathPopString (ctxt);
  ret = class_match (class, name);
  xmlFree (name);
  xmlFree (class);
  xmlXPathReturnBoolean (ctxt, ret);
}

/****************************************************************************
 * obj if(obj,obj?,obj)
 * Cast the first argument to type boolean.  If false return the final argument,
 * otherwise if called with 2 arguments return the 1st argument or if called
 * with 3 arguments return the second argument.
 ****************************************************************************/
static void
fn_cond_if (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlXPathObjectPtr cond, if_true, if_false;

  if (nargs < 2 || nargs > 3)
    {
      xmlGenericError (xmlGenericErrorContext,
		       "if() requires 2 or 3 arguments\n");
      ctxt->error = XPATH_INVALID_ARITY;
      return;
    }

  if_false = valuePop (ctxt);
  if_true = (nargs == 3) ? valuePop (ctxt) : NULL;
  cond = valuePop (ctxt);
  if (xmlXPathCastToBoolean (cond))
    {
      if (if_true != NULL)
        {
	  xmlXPathFreeObject (cond);
	  valuePush (ctxt, if_true);
	}
      else
	valuePush (ctxt, cond);
      xmlXPathFreeObject (if_false);
    }
  else
    {
      xmlXPathFreeObject (cond);
      if (if_true != NULL)
	xmlXPathFreeObject (if_true);
      valuePush (ctxt, if_false);
    }
}

/****************************************************************************/

void
xsltFunctionsRegister (void)
{
  xsltRegisterExtModuleFunction (cX"base-uri", XSLT_FUNCTIONS_NAMESPACE,
  				 fn_base_uri);
  xsltRegisterExtModuleFunction (cX"resolve-uri", XSLT_FUNCTIONS_NAMESPACE,
  				 fn_resolve_uri);
  xsltRegisterExtModuleFunction (cX"string-join", XSLT_FUNCTIONS_NAMESPACE,
  				 fn_string_join);
  xsltRegisterExtModuleFunction (cX"ends-with", XSLT_FUNCTIONS_NAMESPACE,
  				 fn_ends_with);
  xsltRegisterExtModuleFunction (cX"class-match", XSLT_FUNCTIONS_NAMESPACE,
  				 fn_class_match);
  xsltRegisterExtModuleFunction (cX"if", XSLT_FUNCTIONS_NAMESPACE, fn_cond_if);
}
