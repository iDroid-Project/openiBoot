Import('*')

#
# iPad 1G
#

ipad_1g_src = [plat_a4_src,
			env.Localize([
			'#audiohw-null.c',
			'#syscfg.c',
			'#nvram.c',
			'#images.c',
			'#sha1.c',

			'accel.c',
			])]
Export('ipad_1g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPad 1G\\"'])

env.AddModules([
	"nor-spi",
	])

env.OpenIBootTarget('iPad1G', 'ipad_1g_openiboot', 'CONFIG_IPAD_1G', ipad_1g_src, None)
