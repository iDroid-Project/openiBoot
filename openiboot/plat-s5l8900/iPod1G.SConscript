Import('*')

#
# iPod Touch 1G
#

ipod_1g_src = [
	plat_s5l8900_src,
	env.Localize([
	'piezo.c',
	'wm8958.c',
	'multitouch-z2.c',
	])]
Export('ipod_1g_src')

elf, bin, img3 = env.OpenIBootTarget('iPod1G', 'ipod_1g_openiboot', 'CONFIG_IPOD', ipod_1g_src+menu_src, 'template-ipod')
env.OpenIBootTarget('iPod1G-Installer', 'ipod_1g_installer', 'CONFIG_IPOD', ipod_1g_src+installer_src, 'template-ipod')
Default(img3)
