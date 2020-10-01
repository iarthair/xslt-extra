# Libxslt Extension Modules

Although [libxml2 and libxslt](https://www.xmlsoft.org/) are widely available,
unfortunately they implement only XPath 1.0 and XSL-T 1.0.  Therefore a lot of
useful features are missing, particularly XPath functions.

This collection implements various extension elements and functions for libxslt
that have proved useful in a few projects. The functions or elements actually
implemented follow no particular plan, rather they are somewhat randomly chosen
based on immediate need over time.

In principle the XPath extension functions are independent of libxslt and could
be implemented using only libxml2, however the extension modules rely on the
module loading mechanism in libxslt and so they are only available when used
through an XSL-T stylesheet.

They attempt to follow the specifications from [exslt.org](http://exslt.org/)
or XPath 2.0 functions where feasible, although some functions appear in
neither specification. Since XPath 2.0 has a number of features not present in
the libxml2 implementation, corresponding implementations are at most similar
rather than identical.

## Copyright & Licence

The Libxslt Extension Modules are copyright [© 2020 Brian Stafford](license.md).
They are released under the [MIT Licence][1] since this is the licence used
by libxml2 and libxslt.

[1]: https://opensource.org/licenses/mit-license.html
