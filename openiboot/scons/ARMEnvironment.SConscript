#
# Setup our cross-compilation environment.
#

def ARMEnvironment(*a, **kw):
	env = Environment(tools=['gas', 'gcc', 'gnulink', 'ar'], *a, **kw)
	plat_flags = ['-mlittle-endian', '-mfpu=vfp', '-mthumb', '-mthumb-interwork', '-fPIC']
	env.Append(CPPPATH = ['#includes'])
	env.Append(CPPFLAGS = plat_flags+['-nostdlib'])
	env.Append(LINKFLAGS = plat_flags+['-nostdlib', '--nostdlib', '-Ttext=0x0'])
	env.Append(LIBS = ['gcc'])

	if not env.has_key("CROSS"):
		env["CROSS"] = 'arm-elf-'

	env["CC"] = env["CROSS"] + 'gcc'
	env["OBJCOPY"] = env["CROSS"] + 'objcopy'
	env["AR"] = env["CROSS"] + 'ar'

	return env
Export('ARMEnvironment')
