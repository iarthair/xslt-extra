# build the functions extension

xpath_functions_source = [
    'xp-functions.c',
    'xp-functions.h',
]

shared_module('xpfunctions', xpath_functions_source,
	      name_prefix : prefix_iarthair,
	      dependencies : xsldep,
	      install_dir: plugin_dir,
	      install : true)

# build the regexp extension

regexp_source = [
    'xp-regexp.c',
    'xp-regexp.h',
]

shared_module('posix_regex', regexp_source,
	      name_prefix : prefix_iarthair,
	      dependencies : [xsldep],
	      install_dir: plugin_dir,
	      install : true)
