Import('*')

#
# iPhone 2G
#
	
iphone_2g_src = [base_src,
	plat_s5l8900_src,
	radio_pmb8876_src,
	env.Localize([
	'camera.c',
	'als.c',
	'multitouch.c',
	'wm8958.c',
	])]
Export('iphone_2g_src')

elf, bin, img3 = env.OpenIBootTarget('iPhone2G', 'iphone_2g', 'CONFIG_IPHONE_2G', iphone_2g_src, 'template')
Default(img3)
