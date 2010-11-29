Import('*')

#
# iPad 1G
#

ipad_1g_src = [base_src,
			plat_a4_src,
			env.Localize([
			])]
Export('ipad_1g_src')

env.OpenIBootTarget('iPad1G', 'ipad_1g', 'CONFIG_IPAD', ipad_1g_src, 'template-4g')
