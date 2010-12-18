scripting.o: scripting.c includes/openiboot.h includes/commands.h \
  includes/openiboot.h includes/nvram.h includes/hfs/common.h \
  includes/openiboot.h includes/util.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h \
  includes/malloc-2.8.3.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h includes/hfs/bdev.h \
  includes/hfs/common.h includes/hfs/fs.h includes/hfs/hfsplus.h \
  includes/hfs/common.h includes/hfs/hfsplus.h includes/printf.h \
  includes/util.h
scripting.c includes/openiboot.h includes/commands.h :
  includes/openiboot.h includes/nvram.h includes/hfs/common.h :
  includes/openiboot.h includes/util.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h :
  includes/malloc-2.8.3.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h includes/hfs/bdev.h :
  includes/hfs/common.h includes/hfs/fs.h includes/hfs/hfsplus.h :
  includes/hfs/common.h includes/hfs/hfsplus.h includes/printf.h :
  includes/util.h :
