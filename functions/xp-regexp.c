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

#include "xp-regexp.h"

#include <sys/types.h>
#include <regex.h>
#include <string.h>
#include <ctype.h>

#define NMATCH		100

/* Not clear if xmlFree() is safe for NULL pointers */
static inline void
xmlSafeFree (void *ptr)
{
  if (ptr != NULL)
    xmlFree (ptr);
}

/****************************************************************************
 * Within the replacement strings, the following are recognised:
 * \\ - stands for a \ sign
 * &  - the matched substring (N.B. write as &amp; in XML documents)
 * \0 - the matched substring, synonym for &
 * \p - the portion of the string that precedes the matched substring (prefix)
 * \s - the portion of the string that follows the matched substring (suffix)
 * \nnn - the nth matched substring
 ****************************************************************************/
static xmlChar *
copy_match (xmlChar *result, const xmlChar *string,
	    const xmlChar *replacement, const xmlChar *tail,
	    regmatch_t *match, size_t nsub)
{
  size_t nth;
  int sub_len;
  const xmlChar *rep, *sub;
  char *es;

  while ((rep = cX strpbrk ((const char *) replacement, "&\\")) != NULL)
    {
      /* Copy the leading portion of the substitution.  */
      if (rep > replacement)
	result = xmlStrncat (result, replacement, rep - replacement);
      /* Copy the replacement string */
      sub = NULL;
      sub_len = 0;
      if (*rep == '&')
	{
	  if (match[0].rm_so != -1)
	    {
	      sub = tail + match[0].rm_so;
	      sub_len = match[0].rm_eo - match[0].rm_so;
	    }
	  replacement = rep + 1;
	}
      else
	switch (*++rep)
	  {
	  case '\\':
	    sub = rep;
	    sub_len = 1;
	    replacement = rep + 1;
	    break;
	  case '0':
	    if (match[0].rm_so != -1)
	      {
		sub = tail + match[0].rm_so;
		sub_len = match[0].rm_eo - match[0].rm_so;
	      }
	    replacement = rep + 1;
	    break;
	  case 'p':
	    if (match[0].rm_so != -1)
	      {
		sub = string;
		sub_len = tail - string + match[0].rm_so;
	      }
	    replacement = rep + 1;
	    break;
	  case 's':
	    if (match[0].rm_so != -1)
	      {
		sub = tail + match[0].rm_eo;
		sub_len = xmlStrlen (sub);
	      }
	    replacement = rep + 1;
	    break;
	  default:
	    if (isdigit (*rep)
		&& (nth = strtoul ((const char *) rep, &es, 10)) >= 1
		&& nth <= nsub)
	      {
		if (match[nth].rm_so != -1)
		  {
		    sub = tail + match[nth].rm_so;
		    sub_len = match[nth].rm_eo - match[nth].rm_so;
		  }
		replacement = (xmlChar *) es;
	      }
	    else
	      {
		sub = cX "\\";
		sub_len = 1;
		replacement = rep;
	      }
	    break;
	  }
      if (sub_len > 0)
	result = xmlStrncat (result, sub, sub_len);
    }
  return xmlStrcat (result, replacement);
}


/****************************************************************************
 * pre_replace:
 *
 * Scans a string for RE matches and replaces the matched substrings.
 * Return value is the new string so formed.
 ****************************************************************************/
static void
pre_replace (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlChar *string, *tail, *pattern, *flags, *replacement, *ret = NULL;
  xmlChar *p;
  int global, re_flags;
  regex_t re;
  regmatch_t match[NMATCH];

  if (nargs != 4)
    {
      xmlXPathSetArityError (ctxt);
      return;
    }

  replacement = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || replacement == NULL)
    {
      xmlSafeFree (replacement);
      return;
    }

  flags = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || flags == NULL)
    {
      xmlSafeFree (flags);
      xmlSafeFree (replacement);
      return;
    }

  pattern = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || pattern == NULL)
    {
      xmlSafeFree (pattern);
      xmlSafeFree (flags);
      xmlSafeFree (replacement);
      return;
    }

  string = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || string == NULL)
    {
      xmlSafeFree (string);
      xmlSafeFree (pattern);
      xmlSafeFree (flags);
      xmlSafeFree (replacement);
      return;
    }

  /* parse substitution flags */
  global = 0;
  re_flags = REG_EXTENDED;
  for (p = flags; *p != '\0'; p++)
    switch (*p)
      {
      case 'g':
        global = 1;
	break;
      case 'i':
        re_flags |= REG_ICASE;
	break;
      case 'm':
        re_flags |= REG_NEWLINE;
	break;
      }

  if (regcomp (&re, (char *) pattern, re_flags) == 0)
    {
      tail = string;
      if (regexec (&re, (char *) tail, NMATCH, match, 0) == 0)
	do
	  {
	    if (match[0].rm_so > 0)
	      ret = xmlStrncat (ret, tail, match[0].rm_so);
	    if (replacement != NULL)
	      ret = copy_match (ret, string, replacement, tail,
				match, re.re_nsub);
	    tail += match[0].rm_eo;
	  }
	while (global && regexec (&re, (char *) tail, NMATCH, match, REG_NOTBOL) == 0);
      ret = xmlStrcat (ret, tail);
      regfree (&re);
    }

  xmlSafeFree (string);
  xmlSafeFree (pattern);
  xmlSafeFree (flags);
  xmlSafeFree (replacement);

  xmlXPathReturnString (ctxt, ret);
}

/****************************************************************************
 * pre_match:
 *
 * Scans a string for RE matches and returns a node set of match elements,
 * each containing the portion of the string matched by the RE. 
 ****************************************************************************/
static void
pre_match (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlNodePtr node, text;
  xmlXPathObjectPtr ret;
  xmlChar *string, *tail, *pattern, *flags;
  xmlChar *p;
  xmlDoc *container;
  xsltTransformContext *tctxt;
  int global, re_flags, matches, i;
  regex_t re;
  regmatch_t match[NMATCH];

  if (nargs < 2 || nargs > 3)
    {
      xmlXPathSetArityError (ctxt);
      return;
    }

  global = 0;
  re_flags = REG_EXTENDED;
  if (nargs == 3 && (flags = xmlXPathPopString (ctxt)) != NULL)
    {
      for (p = flags; *p != '\0'; p++)
	switch (*p)
	  {
	  case 'g':
	    global = 1;
	    break;
	  case 'i':
	    re_flags |= REG_ICASE;
	    break;
	  case 'm':
	    re_flags |= REG_NEWLINE;
	    break;
	  }
      xmlSafeFree (flags);
    }

  pattern = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || pattern == NULL)
    {
      xmlSafeFree (pattern);
      return;
    }

  string = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || string == NULL)
    {
      xmlSafeFree (string);
      xmlSafeFree (pattern);
      return;
    }

  tctxt = xsltXPathGetTransformContext (ctxt);
  container = xsltCreateRVT (tctxt);
  if (container == NULL)
    {
      xmlSafeFree (string);
      xmlSafeFree (pattern);
      return;
    }
  xsltRegisterTmpRVT (tctxt, container);

  ret = xmlXPathNewNodeSet (NULL);
  if (ret == NULL)
    {
      xmlSafeFree (string);
      xmlSafeFree (pattern);
      return;
    }

  ret->boolval = 0;
  if (regcomp (&re, (char *) pattern, re_flags) == 0)
    {
      tail = string;
      if (regexec (&re, (char *) tail, NMATCH, match, 0) == 0)
	{
	  if (global)
	    do
	      {
		/* create a match element for each submatch */
		node = xmlNewDocRawNode (container, NULL, cX "match", NULL);
		xmlAddChild ((xmlNodePtr) container, node);
		if (match[0].rm_so != -1)
		  {
		    text = xmlNewDocTextLen (container,
					     tail + match[0].rm_so,
					     match[0].rm_eo - match[0].rm_so);
		    xmlAddChild (node, text);
		  }
		xmlXPathNodeSetAdd (ret->nodesetval, node);
		tail += match[0].rm_eo;
	      }
	    while (regexec (&re, (char *) tail, NMATCH, match, REG_NOTBOL) == 0);
	  else
	    {
	      /* count the matches */
	      for (matches = NMATCH; matches > 0; matches--)
		if (match[matches - 1].rm_so != -1)
		  break;
	      /* create a match element for each submatch */
	      for (i = 0; i < matches; i++)
		{
		  node = xmlNewDocRawNode (container, NULL, cX "match", NULL);
		  xmlAddChild ((xmlNodePtr) container, node);
		  if (match[i].rm_so != -1)
		    {
		      text = xmlNewDocTextLen (container,
					       tail + match[i].rm_so,
					       match[i].rm_eo - match[i].rm_so);
		      xmlAddChild (node, text);
		    }
		  xmlXPathNodeSetAdd (ret->nodesetval, node);
		}
	    }
	}
      regfree (&re);
    }

  valuePush (ctxt, ret);

  xmlSafeFree (string);
  xmlSafeFree (pattern);
}


/****************************************************************************
 * pre_test:
 *
 * Returns TRUE if the  RE matches the string.
 ****************************************************************************/
static void
pre_test (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlChar *string, *pattern, *flags;
  xmlChar *p;
  int match, re_flags;
  regex_t re;

  if (nargs < 2 || nargs > 3)
    {
      xmlXPathSetArityError (ctxt);
      return;
    }

  re_flags = REG_EXTENDED;
  if (nargs == 3 && (flags = xmlXPathPopString (ctxt)) != NULL)
    {
      for (p = flags; *p != '\0'; p++)
	switch (*p)
	  {
	  case 'i':
	    re_flags |= REG_ICASE;
	    break;
	  case 'm':
	    re_flags |= REG_NEWLINE;
	    break;
	  }
      xmlSafeFree (flags);
    }

  pattern = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || pattern == NULL)
    {
      xmlSafeFree (pattern);
      return;
    }

  string = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || string == NULL)
    {
      xmlSafeFree (string);
      xmlSafeFree (pattern);
      return;
    }

  match = 0;
  if (regcomp (&re, (char *) pattern, re_flags) == 0)
    {
      match = regexec (&re, (char *) string, 0, NULL, 0) == 0;
      regfree (&re);
    }
  xmlXPathReturnBoolean (ctxt, match);

  xmlSafeFree (string);
  xmlSafeFree (pattern);
}

/****************************************************************************
 * pre_filter:
 *
 * Filter the node-set specified by the first argument.  For each
 * node cast to a string and if it matches the RE add it to the result
 * node-set.  Remaining arguments are as per test().
 ****************************************************************************/
static void
pre_filter (xmlXPathParserContextPtr ctxt, int nargs)
{
  xmlChar *string, *pattern, *flags;
  xmlChar *p;
  xmlXPathObject *set, *obj;
  xmlNode *node;
  int re_flags, i;
  regex_t re;

  if (nargs < 2 || nargs > 3)
    {
      xmlXPathSetArityError (ctxt);
      return;
    }

  re_flags = REG_EXTENDED;
  if (nargs == 3 && (flags = xmlXPathPopString (ctxt)) != NULL)
    {
      for (p = flags; *p != '\0'; p++)
	switch (*p)
	  {
	  case 'g':
	    break;
	  case 'i':
	    re_flags |= REG_ICASE;
	    break;
	  case 'm':
	    re_flags |= REG_NEWLINE;
	    break;
	  }
      xmlSafeFree (flags);
    }

  pattern = xmlXPathPopString (ctxt);
  if (xmlXPathCheckError (ctxt) || pattern == NULL)
    {
      xmlSafeFree (pattern);
      return;
    }

  obj = valuePop (ctxt);
  if (xmlXPathCheckError (ctxt) || obj == NULL)
    {
      xmlSafeFree (pattern);
      return;
    }

  set = xmlXPathNewNodeSet (NULL);
  if (obj->type == XPATH_NODESET && obj->nodesetval != NULL)
    {
      if (regcomp (&re, (char *) pattern, re_flags) == 0)
	{
	  for (i = 0; i < obj->nodesetval->nodeNr; i++)
	    {
	      node = obj->nodesetval->nodeTab[i];
	      string = xmlNodeGetContent (node);
	      if (regexec (&re, (char *) string, 0, NULL, 0) == 0)
		xmlXPathNodeSetAdd (set->nodesetval, node);
	      xmlSafeFree (string);
	    }
	  regfree (&re);
	}
    }
  valuePush (ctxt, set);

  xmlXPathFreeObject (obj);
  xmlSafeFree (pattern);
}


/**
 * xsltRegexpRegister:
 *
 * Registers the EXSLT - Regexp module
 */

void
xsltRegexpRegister (void)
{
  xsltRegisterExtModuleFunction (cX "replace", XSLT_REGEXP_NAMESPACE, pre_replace);
  xsltRegisterExtModuleFunction (cX "match", XSLT_REGEXP_NAMESPACE, pre_match);
  xsltRegisterExtModuleFunction (cX "test", XSLT_REGEXP_NAMESPACE, pre_test);
  xsltRegisterExtModuleFunction (cX "filter", XSLT_REGEXP_NAMESPACE, pre_filter);
}
