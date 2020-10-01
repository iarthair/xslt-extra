# EXSLT Script Extension

This module implements the `<script>` extension element decribed in
[exslt.org](http://exslt.org/func/elements/script/index.html) with a few
minor differences.

## <script\>

```xml
xmlns:func="http://exslt.org/functions"

<func:script implements-prefix=ncname
             language=qname
             src=uri-reference
             archive=uri-reference/>
```

The top-level `<func:script>` element provides an implementation of extension
functions in the specified namespace.

The function implementations may be provided as a separate resource or as
element content.

### implements-prefix

The `implements-prefix` attribute is an [ncname][1] expanded to a URI using the
namespace declarations in scope for the script element.  This URI is the
namespace URI identifying the [local name][3] of the XPath extension functions
being implemented.

### language

A [qname][4] identifying programming language used for the extension.  If the
language value is not supported, the script element is ignored.

### src

Optional URI for the resource containing the extension functions.  A relative
URI is resolved relative to the [base URI][2] of the script element.

If omitted the script is read from the element content instead.  Element
content should be protected by enclosing it in a CDATA section or by using XML
entities where these conflict with symbols used in the programming language.
An example is given below.

!!! important
    Currently the `src` property is not fully implemented, only resources
    that are pathnames identifying local files may be specified.

### archive

Not implemented, ignored.

[1]: https://www.w3.org/TR/REC-xml-names/#ns-decl (Declaring Namespaces)
[2]: https://www.w3.org/TR/1999/REC-xslt-19991116#base-uri (Base URI)
[3]: https://www.w3.org/TR/REC-xml-names/#dt-localname (Basic Concepts)
[4]: https://www.w3.org/TR/REC-xml-names/#ns-qualnames (Qualified Names)

# Lua

Currently the only supported script language is
[Lua 5.3](https://www.lua.org/manual/5.3/). The `language` property should be
specified as `Lua`.

Lua functions are exported when the script is compiled, by returning a table of
functions where the key is a string naming the XPath function.
 
For example the following will create the XPath functions `my:func1()`,
`my:func2()` and `my:func3()`. Func3() illustrates that it is possible to
create XPath functions with a closure.

```xml
<xsl:stylesheet ...
                xmlns:func="http://exslt.org/functions"
                xmlns:my="http://example.org/lua"
                extension-element-prefixes="func">

<func:script implements-prefix="my" language="Lua" src="functions.lua"/>

...

</xsl:stylesheet>
```

```lua
-- content of functions.lua

exports = {}

function exports.func1(...)
    return ""
end

function exports.func2(...)
    return ""
end

function func3_factory()
    local closure_var = ""

    return function(...)
        -- do some work involving the closure variables
        return closure_var
    end
end

exports.func3 = func3_factory()

return exports
```

If the above example were implemented as script element content it would read
as follows. In this case to avoid issues with CDATA quoting, multi-line
comments and strings should be written as `--[=[ ... ]=]` and `[=[ ... ]=]`
respectively.

```xml
<xsl:stylesheet ...
                xmlns:func="http://exslt.org/functions"
                xmlns:my="http://example.org/lua"
                extension-element-prefixes="func">

<func:script implements-prefix="my" language="Lua">
<![CDATA[
exports = {}

function exports.func1(...)
    return ""
end

function exports.func2(...)
    return ""
end

function func3_factory()
    local closure_var = ""

    return function(...)
        -- do some work involving the closure variables
        return closure_var
    end
end

exports.func3 = func3_factory()

return exports
]]>
</func:script>

...

</xsl:stylesheet>
```
