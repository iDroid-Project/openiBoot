Import('*')

#
# iPhone 4G
#

iphone_4_src = [plat_a4_src,
				env.Localize([
				])]
Export('iphone_4_src')

env.OpenIBootTarget('iPhone4', 'iphone_4', 'CONFIG_IPHONE_4', iphone_4_src, 'template-4g')
