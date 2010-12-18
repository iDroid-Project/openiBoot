tasks.o: tasks.c includes/tasks.h includes/openiboot.h \
  includes/openiboot-asmhelpers.h includes/util.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h \
  includes/malloc-2.8.3.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h
tasks.c includes/tasks.h includes/openiboot.h :
  includes/openiboot-asmhelpers.h includes/util.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h :
  includes/malloc-2.8.3.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h :
