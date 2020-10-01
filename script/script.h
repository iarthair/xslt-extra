#ifndef _script_h
#define _script_h

typedef struct script script_t;
typedef struct implementation implementation_t;
typedef const char *(*readcb_t) (void *, void *, size_t *);

struct script
  {
    struct implementation *implementation;
    void *state;
  };

struct implementation
  {
    struct script *(*init) (const struct implementation *);
    void (*compile) (const struct script *, const char *uri,
		     readcb_t reader, void *arg);
    int (*call) (const struct script *, const char *uri, const char *function,
		 xmlXPathParserContext *ctxt, int nargs);
    void (*destroy) (const struct script *);
  };

const implementation_t *script_lookup_language (const char *language);
void script_register_hook (const char *name, const char *uri);

#endif
