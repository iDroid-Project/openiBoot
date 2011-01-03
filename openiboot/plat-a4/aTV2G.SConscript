Import('*')

#
# Apple TV 2G
#

iphone_4_src = [plat_a4_src,
				env.Localize([
				'#audiohw-null.c',
				])]
Export('atv_2g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\"for aTV 2G\\"'])

env.OpenIBootTarget('aTV2G', 'atv_2_openiboot', 'CONFIG_ATV_2G', atv_2g_src, 'template-4g')
