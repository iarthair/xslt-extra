#ifndef __XSLT_FUNCTIONS_H__
#define __XSLT_FUNCTIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBEXSLT_PUBLIC
#define LIBEXSLT_PUBLIC
#endif

#define cX              (const xmlChar *)

/**
 * XSLT_FUNCTIONS_NAMESPACE:
 *
 * Namespace for XPath functions
 */
#define XSLT_FUNCTIONS_NAMESPACE (cX"https://iarthair.github.io/xpfunctions")

#ifdef MODULE
#define xsltFunctionsRegister  iarthair_github_io_xpfunctions_init
#endif

LIBEXSLT_PUBLIC void xsltFunctionsRegister (void);

#ifdef __cplusplus
}
#endif
#endif /* __XSLT_FUNCTIONS_H__ */

