Import('*')

#
# iPhone 3G
#

iphone_3g_src = env.Localize([
	'camera.c',
	'multitouch-z2.c',
	'wm8991.c',
	'alsISL29003.c',
	]) + radio_pmb8878_src + plat_s5l8900_src + base_src
Export('iphone_3g_src')

tenv = env.Clone()
tenv['OBJPREFIX'] = 'iPhone3G_'
tenv.Append(CPPDEFINES=['CONFIG_IPHONE_3G'])
iphone_3g_openiboot = tenv.Program('#iphone_3g_openiboot.bin', iphone_3g_src)
iphone_3g_openiboot_img3 = tenv.Make8900Image('#iphone_3g_openiboot.img3', iphone_3g_openiboot+['#mk8900image/template-3g.img3'])
Export(['iphone_3g_openiboot', 'iphone_3g_openiboot_img3'])
Alias('3G', iphone_3g_openiboot_img3)
