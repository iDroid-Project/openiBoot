Import('*')

#
# iPhone 3G
#

iphone_3g_src = [base_src,
	plat_s5l8900_src,
	radio_pmb8878_src,
	env.Localize([
	'camera.c',
	'multitouch-z2.c',
	'wm8991.c',
	'alsISL29003.c',
	'vibrator-3G.c',
	])]
Export('iphone_3g_src')

elf, bin, img3 = env.OpenIBootTarget('iPhone3G', 'iphone_3g', 'CONFIG_IPHONE_3G', iphone_3g_src, 'template-3g')
Default(img3)
