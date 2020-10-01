#include <ctype.h>
#include <string.h>

#include <libxml/globals.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/encoding.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "xslt-lang.h"
#include "rfc4647.h"

#if !defined (__GNUC__) || __GNUC__ < 2
# define __attribute__(x)
#endif
#define _unused	__attribute__((unused))

static int scan_range (const char *, const char **, char *, size_t);
static int scan_name (const char *, const char **, char *, size_t);

/****************************************************************************
 * boolean lang:lang(string)
 *
 * Compare string or node's xml:lang against the range using the
 * RFC 4647 algorithm.  If the range is omitted an application defined
 * default range is used.
 ****************************************************************************/
static void
lang_lang (xmlXPathParserContextPtr ctxt, int nargs)
{
  char *lang, *range;

  if (nargs != 1)
    {
      ctxt->error = XPATH_INVALID_ARITY;
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "lang() takes 1 argument\n");
      return;
    }

  range = (char *) xmlXPathPopString (ctxt);
  if (range == NULL)
    {
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "lang() bad argument\n");
      return;
    }

  lang = (char *) xmlNodeGetLang (ctxt->context->node);
  if (lang == NULL)
    valuePush (ctxt, xmlXPathNewBoolean (0));
  else
    valuePush (ctxt, xmlXPathNewBoolean (rfc4647_extended_match (lang, range)));
  if (range != NULL)
    xmlFree (range);
  if (lang != NULL)
    xmlFree (lang);
}

/****************************************************************************
 * node-set lang:accept-lang(node-set,string)
 *
 * Return a node-set of elements whose xml:lang is the most specific match for
 * the RFC 4647 accept-language range specified in the second argument, if
 * present, or an application specific default.  The range is a space separated
 * language tag list in order of decreasing preference.
 ****************************************************************************/

/* Return the length of the common initial span for strings a and b
   ignoring case for alpha characters */
static int
strncaseinitspan (const char *a, const char *b, int len)
{
  const char *l = a;

  while (len > 0
         && ((*l == *b && *l != '\0')
	     || (isalpha (*l)
	         && isalpha (*b)
		 && (isupper (*l) ? (*l == toupper (*b)) : (toupper (*l) == *b))
		)
	    )
	)
    l++, b++, len--;
  return l - a;
}

static int
compare_range (const char *lang, const char *range, int rlen)
{
  int len;

  len = strncaseinitspan (lang, range, rlen);
  if (len > 0
      && lang[len] == '\0'
      && (range[len] == '\0' || range[len] == ' ' || range[len] == '-')
      && !(len > 2 && range[len-2] == '-'))
    return len;
  return 0;
}

struct params
  {
    const char *range, *erange;
    const xmlChar *lang;
    int last_len;
  };

static void
lang_comp (void *payload _unused, void *data, const xmlChar *lang)
{
  struct params *par = data;
  int len, rlen;

  rlen = par->erange - par->range;
  if (rlen == 1 && *par->range == '*')
    len = strlen ((char *) lang);
  else
    len = compare_range ((char *) lang, par->range, rlen);
  if (len > par->last_len)
    {
      par->lang = lang;
      par->last_len = len;
    }
}

static xmlXPathObject *
rfc4647_lookup (xmlHashTable *table, const char *lang)
{
  struct params par;
  xmlXPathObject *set;
  char range[64+1], canonic[sizeof range];
  const char *p;
  int len;

  memset (&par, 0, sizeof par);
  while ((len = scan_range (lang, &lang, range, sizeof range)) > 0
         && (par.range = canonic_tag (canonic, sizeof canonic, range, 1)) != NULL)
    {
      par.erange = strchr (par.range, '\0');

      /* probe the table for a preferred exact match (the keys in the
         table are canonic and the range has just been converted) */
      if ((set = xmlHashLookup (table, cX canonic)) != NULL)
        {
	  xmlHashRemoveEntry (table, cX canonic, NULL);
	  return set;
	}

      /* no exact match, scan the table for the best (longest) match */
      xmlHashScan (table, lang_comp, &par);
      if (par.lang != NULL)
        {
	  set = xmlHashLookup (table, par.lang);
	  xmlHashRemoveEntry (table, par.lang, NULL);
	  return set;
	}

      if ((p = strchr (lang, ',')) != NULL)
        lang = &p[1];
      else
	while (isspace (*lang))
	  lang++;
    }
  return NULL;
}

static void
free_table (void *payload, const xmlChar *name _unused)
{
  xmlXPathObject *obj = payload;

  if (obj != NULL)
    xmlXPathFreeObject (obj);
}

static void
lang_accept_language (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlNode *node;
  xmlXPathObject *obj, *set, *default_set;
  xmlHashTable *table;
  char *lang;
  char *range;
  char ctag[256];
  int i;

  if (nargs != 2)
    {
      ctxt->error = XPATH_INVALID_ARITY;
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "accept-lang() takes 2 arguments\n");
      return;
    }

  range = (char *) xmlXPathPopString (ctxt);
  if (range == NULL)
    {
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "accept-lang() expecting a string\n");
      return;
    }

  if (ctxt->value == NULL || ctxt->value->type != XPATH_NODESET)
    {
      ctxt->error = XPATH_INVALID_TYPE;
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "accept-lang() expecting a node-set\n");
      xmlFree (range);
      return;
    }
  obj = valuePop (ctxt);
  if (obj->nodesetval == NULL)
    {
      valuePush (ctxt, xmlXPathNewNodeSet (NULL));
      xmlFree (range);
      return;
    }

  default_set = xmlXPathNewNodeSet (NULL);

  /* Loop over the nodeset and build subsets for each language tag
     and store in a hash table. */
  table = xmlHashCreate (obj->nodesetval->nodeNr);
  for (i = 0; i < obj->nodesetval->nodeNr; i++)
    {
      node = obj->nodesetval->nodeTab[i];
      if ((lang = (char *) xmlNodeGetLang (node)) == NULL)
	set = default_set;
      else
	{
	  if (canonic_tag (ctag, sizeof ctag, lang, 1) == NULL)
	    strcpy (ctag, lang);
	  if ((set = xmlHashLookup (table, cX ctag)) == NULL)
	    {
	      set = xmlXPathNewNodeSet (NULL);
	      xmlHashAddEntry (table, cX ctag, set);
	    }
	  xmlFree (lang);
	}
      xmlXPathNodeSetAddUnique (set->nodesetval, node);
    }
  xmlXPathFreeObject (obj);

  /* Loop over the available language tags and pick the best match.
     Remove the entry from the table. Destroy the table. */
  if (range != NULL && (set = rfc4647_lookup (table, range)) != NULL)
    xmlXPathFreeObject (default_set);
  else
    set = default_set;
  xmlHashFree (table, free_table);

  /* Return the selected node-set */
  valuePush (ctxt, set);
  xmlFree (range);
}

/****************************************************************************
 * string lang:canonic-lang(string)
 *
 * Return the canonic form of the language tag in the string or "" if it is
 * not a valid RFC 4646 tag.
 ****************************************************************************/

static void
lang_canonic_tag (xmlXPathParserContextPtr ctxt, int nargs)
{
  char *tag, ntag[256];

  if (nargs != 1)
    {
      ctxt->error = XPATH_INVALID_ARITY;
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "canonic-lang() takes 1 argument\n");
      return;
    }

  tag = (char *) xmlXPathPopString (ctxt);
  if (tag == NULL)
    {
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "canonic-lang() bad argument\n");
      return;
    }

  if (canonic_tag (ntag, sizeof ntag, tag, 1) == NULL)
    ntag[0] = '\0';
  valuePush (ctxt, xmlXPathNewCString (ntag));
  xmlFree (tag);
}

/****************************************************************************
 * string lang:extract-lang(string)
 *
 * Extract the language tag from the string or "" if it is not a valid
 * RFC 4646 tag.
 ****************************************************************************/

static void
lang_tag (xmlXPathParserContextPtr ctxt, int nargs)
{
  char *tag, ntag[256];

  if (nargs != 1)
    {
      ctxt->error = XPATH_INVALID_ARITY;
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "extract-lang() takes 1 argument\n");
      return;
    }

  tag = (char *) xmlXPathPopString (ctxt);
  if (tag == NULL)
    {
      xsltTransformError (xsltXPathGetTransformContext (ctxt), NULL, NULL,
			  "extract-lang() bad argument\n");
      return;
    }

  if (canonic_tag (ntag, sizeof ntag, tag, 0) == NULL)
    ntag[0] = '\0';
  valuePush (ctxt, xmlXPathNewCString (ntag));
  xmlFree (tag);
}

/****************************************************************************/

static int
scan_name (const char *s, const char **es, char *buf, size_t buflen)
{
  char *t;

  buflen -= 1;			/* Leave space for \0 */
  while (isspace (*s))
    s++;
  if (!isalpha (*s))
    return 0;
  t = buf;
  *t++ = *s++;
  if (!(isalnum (*s) || *s == '-'))
    return 0;
  do
    {
      if (s[0] == '-' && !isalnum (s[1]))
	return 0;
      if (t < &buf[buflen])
	*t++ = *s;
      s++;
    }
  while (isalnum (*s) || *s == '-');
  *t = '\0';
  if (es != NULL)
    *es = s;
  return t - buf;
}

static int
scan_range (const char *s, const char **es, char *buf, size_t buflen)
{
  while (isspace (*s))
    s++;
  if (*s == '*')
    {
      strcpy (buf, "*");
      if (es != NULL)
	*es = s + 1;
      return 1;
    }
  return scan_name (s, es, buf, buflen);
}

/****************************************************************************/

void
xsltLangRegister (void)
{
  /* Tag matching functions */
  xsltRegisterExtModuleFunction (cX"lang", XSLT_LANG_NAMESPACE, lang_lang);
  xsltRegisterExtModuleFunction (cX"accept-lang", XSLT_LANG_NAMESPACE,
  				 lang_accept_language);
  /* Return the canonic version of the tag */
  xsltRegisterExtModuleFunction (cX"canonic-lang", XSLT_LANG_NAMESPACE,
  				 lang_canonic_tag);
  /* Extract the language portion of the tag */
  xsltRegisterExtModuleFunction (cX"extract-lang", XSLT_LANG_NAMESPACE,
  				 lang_tag);
}
