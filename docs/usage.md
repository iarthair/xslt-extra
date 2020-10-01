# Using the Libxslt Extension Modules

The extension modules are installed in libxslt's plugin directory, for example
`/usr/lib/libxslt-plugins`. Extension modules placed there will be located
automatically by libxslt or any program that uses it such as `xsltproc`.

## Usage

Usage is straightforward.  The documentation for each extension element or
function includes the namespace URI to be used. This is declared in the
`<xsl:stylesheet>` element in the usual way and the prefix is listed in the
`extension-element-prefixes` attribute.

!!! important
    Remember to add the prefix to `extension-element-prefixes` since this tells
    libxslt to load the plugin referenced by the associated URI.  Forgetting to
    do this can be a frustrating source of errors in your stylesheet.

For example if the prefix is `ext` and the URI is `https://iarthair.github.io/xpfunctions`
they are declared in the XSL-T stylesheet follows as:

```xsl
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:ext="https://iarthair.github.io/xpfunctions"
                extension-element-prefixes="ext">

...

</xsl:stylesheet>
```

XPath functions may then be referenced via the namespace prefix in the usual
way, for example to call the `string-join()` function:

```xsl
<xsl:template match="*">
  <xsl:value-of select="ext:string-join(*,' + ')"/>
</xsl:template>
```

Similar considerations apply for extension elements.

# Non-standard Installation

If installation is made to a non-standard plugin directory, set the environment
variable `LIBXSLT_PLUGINS_PATH` to point to the appropriate directory.  Note
that this is not a colon separated list of directories, only a single directory
may be specified. For example:

```sh
$ LIBXSLT_PLUGINS_PATH=/path/to/modules xsltproc ss.xsl file.xml
```



