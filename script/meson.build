# build the script extension from exslt.org/functions

script_source = [
    'exslt-script.c',
    'exslt-script.h',
    'exslt-script-lua.c',
    'luatools.c',
    'luatools.h',
    'lua-xml.c',
    'lua-xml.h',
    'script.h',
]

luadep = dependency('lua5.3', required : false)
if luadep.found()
    shared_module('functions', script_source,
		  name_prefix : prefix_exslt,
		  dependencies : [xsldep, luadep],
		  install_dir: plugin_dir,
		  install : true)
endif
