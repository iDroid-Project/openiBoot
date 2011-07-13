Import('*')

#
# iPhone 2G
#
	
iphone_2g_src = [
	plat_s5l8900_src,
	env.Localize([
	'camera.c',
	'als-TSL2561.c',
	'multitouch-z1.c',
	'wm8958.c',
	'vibrator-2G.c',
	])]
Export('iphone_2g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPhone 2G\\"', 'MACH_ID=3556'])

env.AddModules([
	"radio-pmb8876",
	"nor-cfi",
	])

elf, bin, img3 = env.MenuEnv().OpenIBootTarget('iPhone2G', 'iphone_2g_openiboot', 'CONFIG_IPHONE_2G', iphone_2g_src, 'template')
env.InstallerEnv().OpenIBootTarget('iPhone2G-Installer', 'iphone_2g_installer', 'CONFIG_IPHONE_2G', iphone_2g_src, 'template')
Default(img3)
