nvram.o: nvram.c includes/openiboot.h includes/nvram.h \
  includes/openiboot.h includes/util.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h \
  includes/malloc-2.8.3.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h includes/nor.h
nvram.c includes/openiboot.h includes/nvram.h :
  includes/openiboot.h includes/util.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h :
  includes/malloc-2.8.3.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h includes/nor.h :
