Import('*')

#
# iPod Touch 1G
#

ipod_1g_src = env.Localize([
	'piezo.c',
	'wm8958.c',
	'multitouch-z2.c',
	]) + plat_s5l8900_src + base_src
Export('ipod_1g_src')

tenv = env.Clone()
tenv.Append(CPPDEFINES = ['CONFIG_IPOD'])
tenv["OBJPREFIX"] = 'iPod1G_'
ipod_1g_openiboot = tenv.Program('#ipod_1g_openiboot.bin', ipod_1g_src)
ipod_1g_openiboot_img3 = tenv.Make8900Image('#ipod_1g_openiboot.img3', ipod_1g_openiboot+['#mk8900image/template-ipod.img3'])
Export(['ipod_1g_openiboot', 'ipod_1g_openiboot_img3'])
Alias('iPod1G', ipod_1g_openiboot_img3)
