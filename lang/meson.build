# build the lang extension

lang_source = [
    'xslt-lang.c',
    'xslt-lang.h',
    'lang.c',
    'rfc4647.c',
    'rfc4647.h'
]

shared_module('lang', lang_source,
	      name_prefix : prefix_iarthair,
	      dependencies : [xsldep],
	      install_dir: plugin_dir,
	      install : true)
