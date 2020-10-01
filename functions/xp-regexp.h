#ifndef __XSLT_REGEXP_H__
#define __XSLT_REGEXP_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBEXSLT_PUBLIC
#define LIBEXSLT_PUBLIC
#endif

#define cX              (const xmlChar *)

/**
 * XSLT_REGEXP_NAMESPACE:
 *
 * Namespace for XSLT regexp functions
 */
#define XSLT_REGEXP_NAMESPACE (cX"https://iarthair.github.io/posix-regex")

#ifdef MODULE
#define xsltRegexpRegister iarthair_github_io_posix_regex_init
#endif

LIBEXSLT_PUBLIC void xsltRegexpRegister (void);

#ifdef __cplusplus
}
#endif
#endif /* __XSLT_REGEXP_H__ */

