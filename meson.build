project('xslt-extra', 'c', version : '1.0.1', default_options: ['c_std=c11'])
cc = meson.get_compiler('c')

cflags = [
    '-D_GNU_SOURCE',
    '-DMODULE'
]
if get_option('warning_level') == '3'
    cflags += [
	'-Wstrict-prototypes',
	'-Wmissing-prototypes',
	'-Wnested-externs',
	'-Walloc-zero',
	'-Wduplicated-branches',
	'-Wpointer-arith',
	'-Wcast-align',
	'-Wwrite-strings',
	'-Wdeclaration-after-statement',
	'-Wshadow',
	'-Wredundant-decls',
	'-Wpacked',
	'-Wbad-function-cast',
    ]
endif
add_project_arguments(cc.get_supported_arguments(cflags), language: 'c')

# use for extensions described at www.exslt.org
ns_exslt = ['exslt','org']
prefix_exslt = '_'.join(ns_exslt) + '_'
domain_exslt = '.'.join(ns_exslt)

# use for other extensions
ns_iarthair = ['iarthair','github','io']
prefix_iarthair = '_'.join(ns_iarthair) + '_'
domain_iarthair = '.'.join(ns_iarthair)

xsldep = dependency('libxslt')
plugin_dir = xsldep.get_variable(pkgconfig : 'libdir') / 'libxslt-plugins'

subdir('script')
subdir('lang')
subdir('functions')
