Import('*')

#
# iPod Touch 2G
#

ipt_2g_src = [plat_s5l8720_src,
				env.Localize([
				'#audiohw-null.c',
				])]
Export('ipt_2g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for iPod Touch 2G\\"'])

elf, bin, img3 = env.OpenIBootTarget('iPodTouch2G', 'ipt_2g_openiboot', 'CONFIG_IPOD_TOUCH_2G', ipt_2g_src, 'template-ipt2g')
Default(img3)
