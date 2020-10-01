# XPath URI Functions

## base-uri()
```xquery
xmlns:fn="https://iarthair.github.io/xpfunctions"

string fn:base-uri(node-set?)
```

Return the base URI of the specified node or the context node
if the argument is omitted.

### Arguments

* `node-set`?: use first node in node-set or context node if omitted.

### Returns

* `string`: the base URI.

---

## resolve-uri()
```xquery
xmlns:fn="https://iarthair.github.io/xpfunctions"

string fn:resolve-uri(string, string?)
```

Resolve-uri() takes a base URI and a relative URI as arguments, and
constructs an absolute URI.  If either argument is
not a string it is converted as if with the XPath `string()` function.

### Arguments

* `string`: relative URI.
* `string`?: base URI or base URI of context node if omitted.

### Returns

* `string`: absolute URI

