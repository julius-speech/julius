project('julius_exe', 'c', version:'4.5')

libsent_proj = subproject('libsent')
libjulius_proj = subproject('libjulius')

sources=[ 
  'main.c',
  'recogloop.c',
  'module.c',
  'output_module.c',
  'output_stdout.c',
  'output_file.c',
  'record.c'
]

conf = configuration_data()
compiler = meson.get_compiler('c')

if get_option('visualize')
	conf.set('VISUALIZE',1)
	if host_machine.system() == 'linux'
		conf.set('PLAYCOMMAND','"sox -r %d -c 1 -s -w -t .raw -x %s -t ossdsp /dev/dsp"')
	elif host_machine.system() == 'solaris2'
		conf.set('PLAYCOMMAND','"sox -r %d -c 1 -s -w -t .raw %s -t sunau /dev/audio"')
	endif
endif

use_charconv=get_option('charconv')

if use_charconv != 'no'
	sources+='charconv.c'
endif

if use_charconv=='auto'
	if compiler.has_header('winnls.h')
		use_cfunc='win32'
	elif compiler.has_function('iconv_open', prefix: '#include <iconv.h>')
		use_cfunc='iconv'
	endif
elif use_charconv == 'win'
	if compiler.has_header('winnls.h')
		use_cfunc='win32'
	else
		use_cfunc='none'
	endif
elif use_charconv == 'iconv'
	if compiler.has_function('iconv_open', prefix: '#include <iconv.h>')
		use_cfunc='iconv'
	endif
elif use_charconv == 'no'
	use_cfunc='no'
endif

if use_cfunc == 'win32'
	conf.set('CHARACTER_CONVERSION',1)
	conf.set('USE_WIN32_MULTIBYTE',1)
	sources += [
		'charconv_win32.c',
		'charconv_libjcode.c',
		'libjcode/libjcode.c',
		'libjcode/detect.c'
	]
elif use_cfunc == 'iconv'
	conf.set('CHARACTER_CONVERSION',1)
	sources += [
		'charconv_iconv.c'
	]
elif use_cfunc == 'libjcode'
	conf.set('CHARACTER_CONVERSION',1)
	conf.set('USE_LIBJCODE',1)
	sources += [
		'charconv_libjcode.c',
		'libjcode/libjcode.c',
		'libjcode/detect.c',
	]
endif

configure_file(output :'config.h', configuration : conf)

libsent_dep = libsent_proj.get_variable('libsent_dep')
libjulius_dep = libjulius_proj.get_variable('libjulius_dep')


executable('julius', sources, dependencies:[libjulius_dep,libsent_dep])


