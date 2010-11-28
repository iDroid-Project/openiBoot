Import('*')

#
# iPad 1G
#

ipad_1g_src = env.Localize([
	]) + plat_a4_src + base_src
Export('ipad_1g_src')

tenv = env.Clone()
tenv['OBJPREFIX'] = 'iPad1G_'
tenv.Append(CPPDEFINES=['CONFIG_IPAD'])
ipad_1g_openiboot = tenv.Program('#ipad_1g_openiboot.bin', ipad_1g_src)
ipad_1g_openiboot_img3 = tenv.Make8900Image('#ipad_1g_openiboot.img3', ipad_1g_openiboot+['#mk8900image/template-4g.img3'])
Export(['ipad_1g_openiboot', 'ipad_1g_openiboot_img3'])
Alias('1G', ipad_1g_openiboot_img3)
