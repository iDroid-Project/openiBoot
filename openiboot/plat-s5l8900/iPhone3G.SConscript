Import('*')

#
# iPhone 3G
#

iphone_3g_src = [
	plat_s5l8900_src,
	env.Localize([
	'camera.c',
	'multitouch-z2.c',
	'wm8991.c',
	'alsISL29003.c',
	'vibrator-3G.c',
	])]
Export('iphone_3g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPhone 3G\\"', 'MACH_ID=3557'])

env.AddModules([
	"radio-pmb8878",
	"nor-spi",
	])

elf, bin, img3 = env.MenuEnv().OpenIBootTarget('iPhone3G', 'iphone_3g_openiboot', 'CONFIG_IPHONE_3G', iphone_3g_src, 'template-3g')
env.InstallerEnv().OpenIBootTarget('iPhone3G-Installer', 'iphone_3g_installer', 'CONFIG_IPHONE_3G', iphone_3g_src, 'template-3g')
Default(img3)
