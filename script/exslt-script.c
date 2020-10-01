#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlIO.h>
#include <libxml/uri.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/templates.h>
#include <libxslt/extensions.h>

#include "exslt-script.h"
#include "script.h"

#define _unused      __attribute__((unused))

/*****************************************************************************
 *****************************************************************************/

static script_t *
init_script (const implementation_t *implementation)
{
  return (*implementation->init) (implementation);
}

static void
destroy_script (script_t *script)
{
  (*script->implementation->destroy) (script);
}

static void
compile_script (script_t *script, const char *uri, readcb_t reader, void *arg)
{
  (*script->implementation->compile) (script, uri, reader, arg);
}

static int
call_script_function (script_t *script, const char *uri, const char *function,
		      xmlXPathParserContext *ctxt, int nargs)
{
  return (*script->implementation->call) (script, uri, function, ctxt, nargs);
}

/*****************************************************************************
 *****************************************************************************/

static void *
script_style_init (xsltStylesheet *style _unused, const xmlChar *uri _unused)
{
  return xmlHashCreate (10);
}

static void
destroy_script_cb (void *payload, const xmlChar *name _unused)
{
  script_t *script = payload;

  destroy_script (script);
}

static void
script_style_shutdown (xsltStylesheet *style _unused,
		       const xmlChar *uri _unused, void *data)
{
  xmlHashFree (data, destroy_script_cb);
}

/*****************************************************************************
 *****************************************************************************/

struct buffer
  {
    const char *(*reader) (void *, void *, size_t *);
    void *arg;
    const char *uri;
    char data[4096];
  };

static const char *
script_readfile_cb (void *state _unused, void *stream, size_t *size)
{
  struct buffer *buf = stream;
  size_t len;

  len = fread (buf->data, sizeof (char), sizeof buf->data, buf->arg);
  *size = len;
  return len > 0 ? buf->data : NULL;
}

static const char *
script_readstring_cb (void *state _unused, void *stream, size_t *size)
{
  struct buffer *buf = stream;
  const char *data;

  if (buf->arg == NULL)
    return NULL;
  data = buf->arg;
  buf->arg = NULL;
  *size = strlen (data);
  return data;
}

/*****************************************************************************
 *****************************************************************************/

static void
script_comp (xsltStylesheet *style, xmlNode *inst)
{
  xmlHashTable *style_data;
  xmlNs *ns;
  const xmlChar *prefix, *language, *value;
  xmlChar *base;
  char *src, *source;
  script_t *script;
  const implementation_t *implementation;
  struct buffer buf;
  FILE *fp;

  if (style == NULL || inst == NULL)
    return;

  style_data = xsltStyleGetExtData (style, EXSLT_SCRIPT_NAMESPACE);
  if (style_data == NULL)
    {
      xsltGenericError (xsltGenericErrorContext,
			"script_comp: no stylesheet data\n");
      return;
    }

  /* check required attributes are present */
  prefix = xsltGetCNsProp (style, inst, cX"implements-prefix",
			   EXSLT_SCRIPT_NAMESPACE);
  language = xsltGetCNsProp (style, inst, cX"language", EXSLT_SCRIPT_NAMESPACE);
  if (prefix == NULL || language == NULL)
    {
      xsltGenericError (xsltGenericErrorContext, "script_comp: "
			"implements-prefix and language attributes "
			"must both be specified\n");
      return;
    }

  /* check prefix */
  if ((ns = xmlSearchNs (inst->doc, inst, prefix)) == NULL)
    {
      xsltGenericError (xsltGenericErrorContext, "script_comp: "
      			"unknown prefix '%s'\n", prefix);
      return;
    }

  /* check language is implemented */
  if ((implementation = script_lookup_language ((const char *) language)) == NULL)
    {
      xsltGenericError (xsltGenericErrorContext, "script_comp: "
      			"unknown language '%s'\n", language);
      return;
    }

  /* ignore element if this namespace is already implemented by a
     different language */
  if ((script = xmlHashLookup (style_data, ns->href)) != NULL
      && script->implementation != implementation)
    return;

  /* fetch the source either from the element content or the src attribute */
  source = NULL;
  src = NULL;
  fp = NULL;
  value = xsltGetCNsProp (style, inst, cX"src", EXSLT_SCRIPT_NAMESPACE);
  if (value != NULL)
    {
      if ((base = xmlNodeGetBase (inst->doc, inst)) != NULL)
	{
	  src = (char *) xmlBuildURI (value, base);
	  xmlFree (base);
	}
      else
	src = (char *) xmlBuildURI (value, inst->doc->URL);
      if ((fp = fopen (src, "r")) == NULL)
        {
	  xsltGenericError (xsltGenericErrorContext, "script_comp: "
			    "can't open %s\n", src);
	  xmlFree (src);
	  return;
	}
      buf.reader = script_readfile_cb;
      buf.arg = fp;
      buf.uri = src;
    }
  else if ((source = (char *) xmlNodeGetContent (inst)) != NULL
  	   && *source != '\0')
    {
      buf.reader = script_readstring_cb;
      buf.arg = source;
      value = xsltGetCNsProp (style, inst, cX"id", EXSLT_SCRIPT_NAMESPACE);
      buf.uri = (value != NULL) ? (const char *) value : "node-content";
    }
  else
    {
      xsltGenericError (xsltGenericErrorContext, "script_comp: "
      			"missing script source\n");
      return;
    }

  /* compile_script() creates a registry for Lua functions accessed by a
     combination of namespace URI and function name.
     xsltRegisterExtModuleFunction () is called to register
     script_function_hook() for each Lua function.  Implementation
     dependent variables are set up when compile_script() is first called
     for a particular scripting language. */
  if (script == NULL)
    {
      script = init_script (implementation);
      xmlHashAddEntry (style_data, ns->href, script);
    }
  compile_script (script, (const char *) ns->href, buf.reader, &buf);
  if (fp != NULL)
    fclose (fp);
  xmlFree (source);
  xmlFree (src);
}

static void
script_function_hook (xmlXPathParserContextPtr ctxt, int nargs)
{
  xsltTransformContext *tctxt;
  xmlHashTable *style_data;
  script_t *script;

  tctxt = xsltXPathGetTransformContext (ctxt);
  style_data = xsltStyleGetExtData (tctxt->style, EXSLT_SCRIPT_NAMESPACE);
  script = xmlHashLookup (style_data, ctxt->context->functionURI);
  call_script_function (script, (const char *) ctxt->context->functionURI,
		        (const char *) ctxt->context->function, ctxt, nargs);
}

void
script_register_hook (const char *name, const char *uri)
{
  xsltRegisterExtModuleFunction (cX name, cX uri, script_function_hook);
}



/*****************************************************************************
 * 
 *****************************************************************************/

void
exslt_script_register (void)
{
  xsltRegisterExtModuleFull (EXSLT_SCRIPT_NAMESPACE,
			     NULL, NULL,
			     script_style_init, script_style_shutdown);

  xsltRegisterExtModuleTopLevel ((const xmlChar *) "script",
				 EXSLT_SCRIPT_NAMESPACE, script_comp);
}
