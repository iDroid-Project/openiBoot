Import('*')

#
# iPhone 4G
#

iphone_4_src = [plat_a4_src,
				env.Localize([
				'#audiohw-null.c',
				'accel.c',
				])]
Export('iphone_4_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPhone 4\\"', 'MACH_ID=3563'])

env.AddModules([
	"radio-xgold618",
	])

env.OpenIBootTarget('iPhone4', 'iphone_4_openiboot', 'CONFIG_IPHONE_4', iphone_4_src, None)
