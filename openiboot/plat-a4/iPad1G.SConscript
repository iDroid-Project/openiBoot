Import('*')

#
# iPad 1G
#

ipad_1g_src = [plat_a4_src,
			env.Localize([
			'#audiohw-null.c',
			])]
Export('ipad_1g_src')

env.OpenIBootTarget('iPad1G', 'ipad_1g_openiboot', 'CONFIG_IPAD', ipad_1g_src, 'template-4g')
