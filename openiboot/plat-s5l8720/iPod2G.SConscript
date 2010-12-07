Import('*')

#
# iPod Touch 2G
#

ipod_2g_src = [plat_s5l8720_src,
				env.Localize([
				'#audiohw-null.c',
				])]
Export('ipod_2g_src')

env = env.Clone()
env.Append(CPPDEFINES=['OPENIBOOT_VERSION_CONFIG=\\"for iPod Touch 2G\\"'])

elf, bin, img3 = env.OpenIBootTarget('iPod2G', 'ipod_2g_openiboot', 'CONFIG_IPOD_2G', ipod_2g_src, 'template-ipod2g')
#TODO: figure these out
#elf, bin, img3 = env.OpenIBootTarget('iPod2G', 'ipod_2g_openiboot', 'CONFIG_IPOD_2G', ipod_2g_src+menu_src, 'template-ipod2g')
#env.OpenIBootTarget('iPod2G-Installer', 'ipod_2g_installer', 'CONFIG_IPOD_2G', ipod_2g_src+installer_src, 'template-ipod2g')
Default(img3)

