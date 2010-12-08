Import('*')

#
# iPhone 3GS
#

iphone_3gs_src = [plat_s5l8920_src,
				radio_pmb8878_src,
				env.Localize([
				'#audiohw-null.c',
				])]
Export('iphone_3gs_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\"for iPhone 3GS\\"'])

env.OpenIBootTarget('iPhone3GS', 'iphone_3gs_openiboot', 'CONFIG_IPHONE_3GS', iphone_3gs_src, None)
