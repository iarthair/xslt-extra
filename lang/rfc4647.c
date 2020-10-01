#include <stdio.h>
#include <string.h>
#include "rfc4647.h"

/* Implement the extended language matching algorithm described in RFC 4647 */

/* Return 1 if lang matches range according to the rules in
   RFC 4647 section 3.3.2 */
int
rfc4647_extended_match (const char *lang, const char *range)
{
  const char *tag_lang, *end_tag_lang, *tag_range, *end_tag_range;

  /* 1. Split both the extended language range and the language tag being
	compared into a list of subtags by dividing on the hyphen (%x2D)
	character.  Two subtags match if either they are the same when
	compared case-insensitively or the language range's subtag is the
	wildcard '*'.  */
  tag_range = range;
  if ((end_tag_range = strchr (tag_range, '-')) == NULL)
    end_tag_range = strchr (tag_range, '\0');
  tag_lang = lang;
  if ((end_tag_lang = strchr (tag_lang, '-')) == NULL)
    end_tag_lang = strchr (tag_lang, '\0');

  /* 2. Begin with the first subtag in each list.  If the first subtag in
	the range does not match the first subtag in the tag, the overall
	match fails.  Otherwise, move to the next subtag in both the
	range and the tag.  */
  if (end_tag_range - tag_range != 1 || *tag_range != '*')
    {
      if (end_tag_range - tag_range != end_tag_lang - tag_lang
	  || strncasecmp (tag_range, tag_lang, end_tag_lang - tag_lang) != 0)
	return 0;
    }
  if (*(tag_range = end_tag_range) == '-')
    tag_range++;
  if ((end_tag_range = strchr (tag_range, '-')) == NULL)
    end_tag_range = strchr (tag_range, '\0');
  if (*(tag_lang = end_tag_lang) == '-')
    tag_lang++;
  if ((end_tag_lang = strchr (tag_lang, '-')) == NULL)
    end_tag_lang = strchr (tag_lang, '\0');

  /* 3. While there are more subtags left in the language range's list:
   */
  while (*tag_range != '\0')
    {
      /* A. If the subtag currently being examined in the range is the
	    wildcard ('*'), move to the next subtag in the range and
	    continue with the loop.  */
      if (end_tag_range - tag_range == 1 && *tag_range == '*')
	{
	  if (*(tag_range = end_tag_range) == '-')
	    tag_range++;
	  if ((end_tag_range = strchr (tag_range, '-')) == NULL)
	    end_tag_range = strchr (tag_range, '\0');
	}

      /* B. Else, if there are no more subtags in the language tag's
	    list, the match fails.  */
      else if (*tag_lang == '\0')
	{
	  return 0;
	}

      /* C. Else, if the current subtag in the range's list matches the
	    current subtag in the language tag's list, move to the next
	    subtag in both lists and continue with the loop.  */
      else if (end_tag_range - tag_range == end_tag_lang - tag_lang
	  && strncasecmp (tag_range, tag_lang, end_tag_lang - tag_lang) == 0)
	{
	  if (*(tag_range = end_tag_range) == '-')
	    tag_range++;
	  if ((end_tag_range = strchr (tag_range, '-')) == NULL)
	    end_tag_range = strchr (tag_range, '\0');
	  if (*(tag_lang = end_tag_lang) == '-')
	    tag_lang++;
	  if ((end_tag_lang = strchr (tag_lang, '-')) == NULL)
	    end_tag_lang = strchr (tag_lang, '\0');
	}

      /* D. Else, if the language tag's subtag is a "singleton" (a single
	    letter or digit, which includes the private-use subtag 'x')
	    the match fails.  */
      else if (end_tag_lang - tag_lang == 1)
	{
	  return 0;
	}

      /* E. Else, move to the next subtag in the language tag's list and
	    continue with the loop.  */
      else
	{
	  if (*(tag_lang = end_tag_lang) == '-')
	    tag_lang++;
	  if ((end_tag_lang = strchr (tag_lang, '-')) == NULL)
	    end_tag_lang = strchr (tag_lang, '\0');
	}
    }
  /* 4. When the language range's list has no more subtags, the match
	succeeds.  */
  return 1;
}
