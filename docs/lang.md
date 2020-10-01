# RFC 4647 Language Tag Matching

The following functions implement [RFC 4647][1] language tag matching.

In each function the range argument is a string of space separated language
tags listed in order of decreasing preference.

[1]: https://tools.ietf.org/html/rfc4647

## lang()
```xquery
xmlns:lang="https://iarthair.github.io/lang"

boolean lang:lang(string)
```

Compare the context node's language against the range specified in the first
argument. The language of a node is the value of `xml:lang` attribute or, if
absent, the one carried by the nearest ancestor.

This is similar to the XPath `lang()` function except that the comparison is
guaranteed to be the algorithm described in RFC 4647 and consistent with the
other functions in this namespace URI.

### Arguments

* `string`: the language range to accept.

### Returns

* `boolean`: true if the node is matched by the range.

---

## accept-lang()
```xquery
xmlns:lang="https://iarthair.github.io/lang"

node-set lang:accept-lang(node-set,string)
```

Return a node-set of elements whose language is the most specific match for the
range specified in the second argument. The language of a node is the value of
`xml:lang` attribute or, if absent, the one carried by the nearest ancestor.

### Arguments

* `node-set`: the set of nodes to be matched.
* `string`: the language range to accept.

### Returns

* `node-set`: set of matching nodes.

---

## canonic-lang()
```xquery
xmlns:lang="https://iarthair.github.io/lang"

string lang:canonic-lang(string)
```

Return the canonic form of the language tag in the argument string or an empty
string if it is not a valid RFC 4646 tag.

### Arguments

* `string`: the language tag.

### Returns

* `string`: the canonic tag.

---

## extract-lang()
```xquery
xmlns:lang="https://iarthair.github.io/lang"

string lang:extract-lang(string)
```

Return the language tag extracted from the string or an empty string if it is
not a valid RFC 4646 tag.

### Arguments

* `string`: the language tag.

### Returns

* `string`: the canonic tag.

