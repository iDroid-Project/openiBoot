spi.o: spi.c includes/openiboot.h includes/openiboot-asmhelpers.h \
  includes/spi.h includes/openiboot.h includes/hardware/spi.h \
  includes/hardware/s5l8900.h includes/util.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h \
  includes/malloc-2.8.3.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h includes/clock.h \
  includes/hardware/clock1.h includes/chipid.h includes/timer.h \
  includes/interrupt.h includes/hardware/interrupt.h
spi.c includes/openiboot.h includes/openiboot-asmhelpers.h :
  includes/spi.h includes/openiboot.h includes/hardware/spi.h :
  includes/hardware/s5l8900.h includes/util.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h :
  includes/malloc-2.8.3.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h includes/clock.h :
  includes/hardware/clock1.h includes/chipid.h includes/timer.h :
  includes/interrupt.h includes/hardware/interrupt.h :
