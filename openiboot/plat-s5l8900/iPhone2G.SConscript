Import('*')

#
# iPhone 2G
#
	
iphone_2g_src = [
	plat_s5l8900_src,
	env.Localize([
	'camera.c',
	'als.c',
	'multitouch.c',
	'wm8958.c',
	'vibrator.c',
	])]
Export('iphone_2g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPhone 2G\\"'])

env.AddModules([
	"radio-pmb8876",
	"nor-cfi",
	])

elf, bin, img3 = env.MenuEnv().OpenIBootTarget('iPhone2G', 'iphone_2g_openiboot', 'CONFIG_IPHONE_2G', iphone_2g_src, 'template')
env.InstallerEnv().OpenIBootTarget('iPhone2G-Installer', 'iphone_2g_installer', 'CONFIG_IPHONE_2G', iphone_2g_src, 'template')
Default(img3)
