#include <string.h>
#include <ctype.h>
#include "rfc4647.h"

#if !defined (__GNUC__) || __GNUC__ < 2
# define __attribute__(x)
#endif
#define _unused	__attribute__((unused))

const char *__lookup_canonic (const char *tag, int len);
const char *lookup_canonic (const char *tag, int len)
		__attribute__ ((weak, alias ("__lookup_canonic")));

const char *__lookup_un_region (const char *tag, int len);
const char *lookup_un_region (const char *tag, int len)
		__attribute__ ((weak, alias ("__lookup_un_region")));

const char *
__lookup_canonic (const char *tag _unused, int len _unused)
{
  return NULL;
}

const char *
__lookup_un_region (const char *tag _unused, int len _unused)
{
  return NULL;
}

/****************************************************************************/

static const char *step_irregular (const char *tag);

static int
alnum_span (const char *str)
{
  int len;

  for (len = 0; isalnum (str[len]); len++)
    ;
  return len;
}

static int
step_tag (const char *tag)
{
  size_t len;

  len = alnum_span (tag);
  if (len == 1 && tag[0] != '*' && tag[1] == '-') /* step over singleton tag */
    len = 2 + alnum_span (&tag[2]);
  return len;
}

static void
lcase (char *str, int len)
{
  for (; len-- > 0 && *str != '\0'; str++)
    *str = tolower (*str);
}

static void
ucase (char *str, int len)
{
  for (; len-- > 0 && *str != '\0'; str++)
    *str = toupper (*str);
}

char *
canonic_tag (char *buf, size_t bufsize, const char *tag, int full)
{
  const char *irregular, *canon, *subtag;
  char *dest;
  int len, tlen;

  if (strlen (tag) >= bufsize)
    return NULL;

  /* Initial tag is the language.  Look up canonic version, if available,
     otherwise len must be 2 or 3 or it is a grandfathered tag */
  if ((irregular = step_irregular (tag)) != NULL)
    len = tlen = strlen (irregular);
  else
    len = tlen = step_tag (tag);
  canon = lookup_canonic (irregular != NULL ? irregular : tag, len);
  if (canon != NULL)
    {
      len = strlen (canon);
      memcpy (buf, canon, len);
    }
  else if (irregular != NULL)
    memcpy (buf, irregular, len);
  else if (len == 2 || len == 3			/* ISO 639 */
           || (len == 1 && tag[1] == '*')	/* wildcard */
	   || (len >= 4 && tag[1] == '-'))	/* grandfathered */
    {
      memcpy (buf, tag, len);
      lcase (buf, len);
    }
  else
    return NULL;
  dest = buf + len;

  /* handle remainder of the subtags */
  if (full)
    for (subtag = tag + tlen; *subtag == '-'; subtag += len)
      {
	len = step_tag (++subtag);
	*dest++ = '-';
	if (len == 4 && isalpha (subtag[1]))	/* script - title case */
	  {
	    memcpy (dest, subtag, len);
	    ucase (dest, 1);
	    lcase (&dest[1], len - 1);
	    dest += len;
	  }
	else if (len == 2 && isalpha (*subtag))	/* region - upper case */
	  {
	    memcpy (dest, subtag, len);
	    ucase (dest, len);
	    dest += len;
	  }
	else if (len == 3 && isdigit (*subtag))	/* UN numeric region */
	  {
	    /* replace with ISO 639 region where available */
	    if ((canon = lookup_un_region (subtag, len)) != NULL)
	      {
		len = strlen (canon);
		memcpy (dest, canon, len);
	      }
	    else
	      memcpy (dest, subtag, len);
	    dest += len;
	  }
	else					/* other - lower case */
	  {
	    memcpy (dest, subtag, len);
	    lcase (dest, len);
	    dest += len;
	  }
      }
  *dest = '\0';
  return buf;
}

/****************************************************************************/

/* Table of irregular tags with the following exceptions, zh-cmn-Hans and
   zh-cmn-Hant are omitted so that zh-cmn is extracted and there are some
   additional entries (these use the ADD macro instead of IRR).
   The table is ordered in descending length of the tags */
struct irregular
  {
    const char *tag;
    int len;
  };
#define IRR(s)	{ s, sizeof s - 1, },
#define ADD(s)	{ s, sizeof s - 1, },
static const struct irregular irregular[] =
  {
  /*IRR("zh-cmn-Hans")*/
  /*IRR("zh-cmn-Hant")*/
    IRR("zh-min-nan")
    IRR("i-enochian")
    IRR("sgn-BE-fr")
    IRR("sgn-BE-nl")
    IRR("sgn-CH-de")
    IRR("en-GB-oed")
    IRR("i-default")
    IRR("i-klingon")
    IRR("i-navajo")
    ADD("zh-guoyu")
    ADD("zh-hakka")
    ADD("zh-xiang")
    IRR("i-mingo")
    IRR("no-bok")
    IRR("no-nyn")
    IRR("zh-cmn")
    IRR("zh-gan")
    IRR("zh-min")
    IRR("zh-wuu")
    IRR("zh-yue")
    IRR("i-ami")
    IRR("i-bnn")
    IRR("i-hak")
    IRR("i-lux")
    IRR("i-pwn")
    IRR("i-tao")
    IRR("i-tay")
    IRR("i-tsu")
  };

static const char *
step_irregular (const char *tag)
{
  int len;
  size_t i;

  len = strlen (tag);
  for (i = 0; i < sizeof irregular / sizeof irregular[0]; i++)
    if ((irregular[i].len == len 
	 || (irregular[i].len < len && tag[irregular[i].len] == '-'))
	&& strncasecmp (tag, irregular[i].tag, irregular[i].len) == 0)
      return irregular[i].tag;
  return NULL;
}

