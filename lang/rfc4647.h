#ifndef _rfc4647_h
#define _rfc4647_h

int rfc4647_extended_match (const char *lang, const char *range);

char *canonic_tag (char *buf, size_t bufsize, const char *tag, int full);

#endif
