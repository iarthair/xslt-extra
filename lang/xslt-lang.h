#ifndef __XSLT_LANG_H__
#define __XSLT_LANG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBEXSLT_PUBLIC
#define LIBEXSLT_PUBLIC
#endif

#define cX              (const xmlChar *)

/**
 * XSLT_LANG_NAMESPACE:
 *
 * Namespace for EXSLT lang functions
 */
#define XSLT_LANG_NAMESPACE (cX"http://iarthair.github.io/lang")

#ifdef MODULE
#define xsltLangRegister iarthair_github_io_lang_init
#endif

LIBEXSLT_PUBLIC void xsltLangRegister (void);

#ifdef __cplusplus
}
#endif
#endif /* __XSLT_LANG_H__ */

