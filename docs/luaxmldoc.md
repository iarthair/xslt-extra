# XML Document

XML documents are available via the following properties and methods:

## Properties

Document properties are read only variables.

### doc:name
```lua
string = doc:name
```

The document name.

### doc:url
```lua
string = doc:url
```

The document's URL.

### doc:root
```lua
node = doc:root
```

The document's root node.

## Methods

### doc:node()
```lua
local node = doc:node()
```

Create a new node belonging to the document.

### doc:setroot()
```lua
old_root = doc:setroot(node)
```

Set the document's root node to `node`. Returns the old root node.

### doc:serialize()
```lua
doc:serialize(func)
doc:serialize(function (text) io.write (text) end)
```

Serialise the XML document. The argument is a function repeatedly called to
write the resulting text.

