# XML Nodes

!!! attention
    Lua XML documentation is still incomplete

XML element nodes are made available with the following API.

## Properties


### node:name
```lua
local string = node:name
```
### node:root
```lua
local root = node:root
```
### node:document
```lua
local doc = node:document
```
### node:local_name
```lua
local string = node:local_name
```
### node:prefix
```lua
local string = node:prefix
```
### node:namespace_uri
```lua
local string = node:namespace_uri
```

### node:attr
```lua
node:attr[name] = text
local attr = node:attr[name]
```

Set or retrieve an attribute value belonging to the node.  Equivalent to the
`setattr()` and `getattr()` methods respectively.

### node:parent
```lua
local node = node:parent
```
### node:next_sibling
```lua
local sibling = node:next_sibling
```
### node:previous_sibling
```lua
local sibling = node:previous_sibling
```
### node:first_child
```lua
local child = node:first_child
```
### node:last_child
```lua
local child = node:last_child
```

## Methods

### node:append_child()
```lua
node:append_child(child)
```
* child: child to append

Append `child` to the end of the node's child list.

### node:children()
```lua
local child

for child in node:children()
do
    -- process child
end
```

Return an iterator function for the node's children. This is a more convenient
alternative to explicitly looping through nodes from `node:first_child` to
`node:last_child` via the `child:next_sibling` properties.

### node:setattr()
```lua
node:setattr(name, value)
```
* name: string with name of attribute
* value: string with text value of attribute

Set the value of the node's attribute (property).

### node:getattr()
```lua
value = node:getattr(name)
```
* name: string with name of attribute

Return a string with the value of the node's property.

### node:unsetattr()
```lua
node:unsetattr(name)
```
* name: string with name of attribute to remove.

Remove attribute.

### node:copy()
```lua
node:copy()
```
### node:replace()
```lua
node:replace()
```
### node:unlink()
```lua
node:unlink()
```
### node:new()
```lua
node:new(name)
```

* name: string with new node local-name

Create a new node belonging to the same document as the reference node.

### node:text()
```lua
node:text(content)
```

* content: string with new text content

Create a new text node belonging to the same document as the reference node.

### node:parse()
```lua
node:parse(content)
```

* content: string with content to parse

Parse a string containing a well-balanced XML chunk and add the result to
the reference node's list of children.

### node:serialize()
```lua
node:serialize(func)
node:serialize(function (text) io.write (text) end)
```

* func: writer function.

Serialise the XML document. The argument is a function repeatedly called to
write the resulting text.

### node:nodeset()
```lua
local node_set = node:nodeset()
```

Create a new nodeset containing the node.

### node:newattr()
```lua
node:attr(name,text)
```

* name: string with attribute name.
* text: string with text content for the attribute.

Create a new attribute on the node with the specified name and content.

## tostring()

The Lua tostring() function returns the string value of the element content.

## Operators

The following operators are defined

* `=`:
* `<`:
* `<=`:
