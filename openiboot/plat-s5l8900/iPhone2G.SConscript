Import('*')

#
# iPhone 2G
#
	
iphone_2g_src = [
	plat_s5l8900_src,
	radio_pmb8876_src,
	env.Localize([
	'camera.c',
	'als.c',
	'multitouch.c',
	'wm8958.c',
	'vibrator.c',
	])]
Export('iphone_2g_src')

elf, bin, img3 = env.OpenIBootTarget('iPhone2G', 'iphone_2g_openiboot', 'CONFIG_IPHONE_2G', iphone_2g_src+menu_src, 'template')
env.OpenIBootTarget('iPhone2G-Installer', 'iphone_2g_installer', 'CONFIG_IPHONE_2G', iphone_2g_src+installer_src, 'template')
Default(img3)
