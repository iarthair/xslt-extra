#ifndef __EXSLT_SCRIPT_H__
#define __EXSLT_SCRIPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBEXSLT_PUBLIC
#define LIBEXSLT_PUBLIC
#endif

#define cX              (const xmlChar *)

/**
 * EXSLT_SCRIPT_NAMESPACE:
 *
 * Namespace for EXSLT script functions
 */
#define EXSLT_SCRIPT_NAMESPACE (cX"http://exslt.org/functions")

#ifdef MODULE
#define exslt_script_register exslt_org_functions_init
#endif

LIBEXSLT_PUBLIC void exslt_script_register (void);

#ifdef __cplusplus
}
#endif
#endif /* __EXSLT_SCRIPT_H__ */

