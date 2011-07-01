Import('*')

#
# iPad 1G
#

ipad_1g_src = [plat_a4_src,
			env.Localize([
			'#audiohw-null.c',
			'accel.c',
			])]
Export('ipad_1g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPad 1G\\"', 'MACH_ID=3593'])

env.AddModules([
	"nor-spi",
	])

env.OpenIBootTarget('iPad1G', 'ipad_1g_openiboot', 'CONFIG_IPAD_1G', ipad_1g_src, None)
