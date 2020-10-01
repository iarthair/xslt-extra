# XPath String Functions

## string-join()
```xquery
xmlns:fn="https://iarthair.github.io/xpfunctions"

string fn:string-join(node-set,string?)
```

Concatenate a list of strings with an intervening separator.  The list of
strings is specified by a node-set passed in the first argument.  Each
element in the node-set is converted to a string as if by using the XPath
`string()` function. If the separator is not specified an empty string is used.

### Arguments

* `node-set`: A node-set converted to a list of strings.
* `string`?: An optional separator string. If omitted an empty string is used.

### Returns

* `string`: the concatenated string

---

## ends-with()
```xquery
xmlns:fn="https://iarthair.github.io/xpfunctions"

boolean fn:ends-with(string,string)
```

Test whether a string ends with the specified suffix.  If either argument is
not a string it is converted as if with the XPath `string()` function.

### Arguments

* `string`: String to test.
* `string`: Suffix to search for.

### Returns

* `boolean`: true if the test string has the specified suffix.

---

## class-match()
```xquery
xmlns:fn="https://iarthair.github.io/xpfunctions"

string fn:class-match(string, string?)
```

Test whether the token specified in the second argument matches any of the
space-separated tokens in the 1st argument.  If either argument is
not a string it is converted as if with the XPath `string()` function.

### Arguments

* `string`: list of space separated tokens.
* `string`: token to test.

### Returns

* `boolean`: true if the token matches.

