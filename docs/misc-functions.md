# XPath Miscellaneous Functions

## if()
```xquery
xmlns:fn="https://iarthair.github.io/xpfunctions"

obj fn:if(obj, obj)
obj fn:if(obj, obj, obj)
```

Implement a conditional operator.  Convert the first argument to boolean as if
using the XPath `boolean()` function.  If called with two arguments return the
first argument if the condition is true otherwise return the second argument.
If called with three arguments return the second argument if the condition is
true otherwise return the final argument.  Arguments may be of any type
provided the first argument may be converted to boolean.

### Arguments

2 arguments:

* `obj`: evaluate as boolean and return if true
* `obj`: return if first argument is false

3 arguments:

* `obj`: evaluate as boolean
* `obj`: return if first argument is true
* `obj`: return if first argument is false

### Returns

* `obj`: matching argument

