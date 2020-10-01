# XML and XSL-T access from Lua

When using Lua from the `<script>` element, XML and XSLT types are
automatically available when accessed via arguments passed to Lua functions.

Additional functions described here are accessed using `require "libxslt"`.

```lua
xslt = require "libxslt"

nodeset = xslt.current()
pos = xslt.position()
pos = xslt.last()
nodeset = xslt.nodeset()
```

## Functions

### current()

Return a node-set containing the current XPath context node. Equivalent to the
XPath `current()` function.

### position()

Return a number equal to the context position from the current XPath expression
evaluation context.  Equivalent to the XPath `position()` function.

### last()

Return a number equal to the context size from the current XPath expression
evaluation context.  Equivalent to the XPath `last()` function.

### nodeset()

Return a new empty nodeset.

