Import('*')

#
# iPhone 2G
#
	
iphone_2g_src = env.Localize([
	'camera.c',
	'als.c',
	'multitouch.c',
	'wm8958.c',
	]) + radio_pmb8876_src + plat_s5l8900_src + base_src
Export('iphone_2g_src')

tenv = env.Clone()
tenv['OBJPREFIX'] = 'iPhone2G_'
tenv.Append(CPPDEFINES=['CONFIG_IPHONE_2G'])
iphone_2g_openiboot = tenv.Program('#iphone_2g_openiboot.bin', iphone_2g_src)
iphone_2g_openiboot_img3 = tenv.Make8900Image('#iphone_2g_openiboot.img3', iphone_2g_openiboot+['#mk8900image/template.img3'])
Export(['iphone_2g_openiboot', 'iphone_2g_openiboot_img3'])
Alias('2G', iphone_2g_openiboot_img3)
