Import('*')

#
# iPod Touch 1G
#

ipt_1g_src = [
	plat_s5l8900_src,
	env.Localize([
	'piezo.c',
	'wm8958.c',
	'multitouch-z2.c',
	])]
Export('ipt_1g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPod Touch 1G\\"', 'MACH_ID=3558'])

env.AddModules([
	"nor-cfi",
	])

elf, bin, img3 = env.MenuEnv().OpenIBootTarget('iPodTouch1G', 'ipt_1g_openiboot', 'CONFIG_IPOD_TOUCH_1G', ipt_1g_src, 'template-ipt1g')
env.InstallerEnv().OpenIBootTarget('iPodTouch1G-Installer', 'ipt_1g_installer', 'CONFIG_IPOD_TOUCH_1G', ipt_1g_src, 'template-ipt1g')
Default(img3)
