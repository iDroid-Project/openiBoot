Import('*')

#
# Apple TV 2G
#

atv_2g_src = [plat_a4_src,
				env.Localize([
				'#audiohw-null.c',
				'mcu.c',
				])]
Export('atv_2g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\" for aTV 2G\\"', 'MACH_ID=3594'])

env.OpenIBootTarget('aTV2G', 'atv_2g_openiboot', 'CONFIG_ATV_2G', atv_2g_src, None)
