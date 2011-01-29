Import('*')

#
# iPod Touch 4G
#

ipt_4g_src = [plat_a4_src,
				env.Localize([
				'#audiohw-null.c',
				])]
Export('ipt_4g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\"for iPod Touch 4G\\"'])

env.OpenIBootTarget('iPodTouch4G', 'ipt_4g_openiboot', 'CONFIG_IPOD_4G', ipt_4g_src, None)
