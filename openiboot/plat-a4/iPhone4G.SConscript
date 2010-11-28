Import('*')

#
# iPhone 4G
#

iphone_4_src = env.Localize([
	]) + plat_a4_src + base_src
Export('iphone_4_src')

tenv = env.Clone()
tenv['OBJPREFIX'] = 'iPhone4_'
tenv.Append(CPPDEFINES=['CONFIG_IPHONE_4'])
iphone_4_openiboot = tenv.Program('#iphone_4_openiboot.bin', iphone_4_src)
iphone_4_openiboot_img3 = tenv.Make8900Image('#iphone_4_openiboot.img3', iphone_4_openiboot+['#mk8900image/template-4g.img3'])
Export(['iphone_4_openiboot', 'iphone_4_openiboot_img3'])
Alias('iPhone4', iphone_4_openiboot_img3)
