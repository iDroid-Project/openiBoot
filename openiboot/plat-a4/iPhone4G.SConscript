Import('*')

#
# iPhone 4G
#

iphone_4g_src = env.Localize([
	]) + plat_a4_src + base_src
Export('iphone_4g_src')

tenv = env.Clone()
tenv['OBJPREFIX'] = 'iPhone4G_'
tenv.Append(CPPDEFINES=['CONFIG_IPHONE_4'])
iphone_4g_openiboot = tenv.Program('#iphone_4g_openiboot.bin', iphone_4g_src)
iphone_4g_openiboot_img3 = tenv.Make8900Image('#iphone_4g_openiboot.img3', iphone_4g_openiboot+['#mk8900image/template-4g.img3'])
Export(['iphone_4g_openiboot', 'iphone_4g_openiboot_img3'])
Alias('4G', iphone_4g_openiboot_img3)
