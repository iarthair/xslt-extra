# Posix Regular Expressions

The regular expression functions are modelled on those described at
[exslt.org](http://exslt.org/regexp/index.html).  The exslt specification
specifies Javascript style regular expressions, however these functions are
implemented using Posix regular expression syntax since these functions
are readily available on Unix like platforms.

In all cases if a non-string argument is specified where a string is expected
the argument is converted as if by using the XPath `string()` function.

## replace()
```xquery
xmlns:re="https://iarthair.github.io/posix-regex"

string re:replace(string, string, string, string)
```
The re:replace() function replaces the parts of a string that match a
Posix extended regular expression with the replacement string.

Within the replacement strings, the following sequences are recognised:

- `\\` - stands for a `\` sign.
- `&`  - the matched substring (write as `&amp;` in XML documents).
- `\0` - the matched substring, synonym for `&`.
- `\p` - the portion of the string that precedes the matched substring (prefix).
- `\s` - the portion of the string that follows the matched substring (suffix).
- `\nnn` - the nth matched substring.

The following flag characters are recognised:

- `i` - perform a case insensitive search.
- `g` - global match; replace all occurrences of the pattern, otherwise replace
  only the first match.
- `m` - match-any-character operators don't match a newline.

### Arguments

* `string`: the string to be matched
* `string`: a Posix extended regular expression
* `string`: flags
* `string`: replacement string

Note that the order of arguments follows exslt.org and differs from the
corresponding XPath 2 function.

### Returns

* `string`: resulting string

---

## match()
```xquery
xmlns:re="https://iarthair.github.io/posix-regex"

object re:match(string, string, string?)
```
The re:match() function lets you get hold of the substrings of the string
passed as the first argument that match the captured parts of the regular
expression passed as the second argument.

The following flag characters specified in the optional final argument are
recognised:

- `i` - perform a case insensitive search.
- `g` - global match
- `m` - match-any-character operators don't match a newline.

The return value is a node set of `<match>` elements, each of whose string
value is equal to a portion of the first argument string captured by the
regular expression.

Behaviour differs depending on whether the match is global.  If the match is
not global, the first match element has a value equal to the portion of the
string matched by the entire regular expression. Subsequent elements have
values equal to the corresponding submatches from the regular expression.

If the match is global, each match element contains a portion of the string
matched by the entire regular expression.

### Arguments

* `string`: the string to be matched
* `string`: a Posix extended regular expression
* `string`?: optional flags argument. Equivalent to empty string if omitted.

### Returns

* `node-set`: a node set of `<match>` elements.

---

## test()
```xquery
xmlns:re="https://iarthair.github.io/posix-regex"

boolean re:test(string, string, string?)
```
The re:test() function returns true if the string given as the first argument
matches the regular expression given as the second argument. 

The following flag characters are recognised:
- `i` - perform a case insensitive search.
- `m` - match-any-character operators don't match a newline.

### Arguments

* `string`: the string to be matched
* `string`: a Posix extended regular expression
* `string`?: optional flags argument. Equivalent to empty string if omitted.

### Returns

* `boolean`: whether the string matches.

---

## filter()
```xquery
xmlns:re="https://iarthair.github.io/posix-regex"

re:filter(node-set, string, string?)

```
Filter the node-set specified by the first argument. Each node is converted
to a string as if using the XPath string() function and if it matches the RE
add it to the result node-set.

The following flag characters are recognised:
- `i` - perform a case insensitive search.
- `m` - match-any-character operators don't match a newline.

### Arguments

* `node-set`: the nodes to be matched
* `string`: a Posix extended regular expression
* `string`?: optional flags argument. Equivalent to empty string if omitted.

### Returns

* `node-set`: the filtered, possibly empty, node set.

