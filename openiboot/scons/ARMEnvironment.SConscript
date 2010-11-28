#
# Setup our cross-compilation environment.
#

def ARMEnvironment(*a, **kw):
	env = Environment(tools=['gcc', 'gnulink', 'gas', 'ar'], *a, **kw)
	plat_flags = ['-mlittle-endian', '-mfpu=vfp', '-mthumb', '-mthumb-interwork']
	env.Append(CPPPATH = ['#includes'])
	env.Append(ASFLAGS = plat_flags+['-Lincludes'])
	env.Append(CPPFLAGS = plat_flags+['-nostdlib'])
	env.Append(LINKFLAGS = plat_flags+['-nostdlib', '--format=binary'])
	env.Append(LIBS = ['gcc'])

	if not env.has_key("CROSS"):
		env["CROSS"] = 'arm-elf-'

	env["CC"] = env["CROSS"] + 'gcc'
	env["OBJCOPY"] = env["CROSS"] + 'objcopy'
	env["AR"] = env["CROSS"] + 'ar'

	return env
Export('ARMEnvironment')
