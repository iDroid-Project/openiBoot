Import('*')

#
# iPhone 4G
#

iphone_4_src = [plat_a4_src,
				env.Localize([
				'#audiohw-null.c',
				])]
Export('iphone_4_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\"for iPhone 4\\"'])

env.OpenIBootTarget('iPhone4', 'iphone_4_openiboot', 'CONFIG_IPHONE_4', iphone_4_src, 'template-4g')
