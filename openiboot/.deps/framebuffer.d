framebuffer.o: framebuffer.c includes/openiboot.h includes/framebuffer.h \
  includes/openiboot.h includes/lcd.h includes/clock.h \
  includes/hardware/clock1.h includes/hardware/s5l8900.h includes/util.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h \
  includes/malloc-2.8.3.h \
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h pcf/9x15.h \
  includes/openiboot-asmhelpers.h includes/stb_image.h includes/util.h
framebuffer.c includes/openiboot.h includes/framebuffer.h :
  includes/openiboot.h includes/lcd.h includes/clock.h :
  includes/hardware/clock1.h includes/hardware/s5l8900.h includes/util.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stdarg.h includes/printf.h :
  includes/malloc-2.8.3.h :
  /usr/local/lib/gcc/arm-elf/4.1.1/include/stddef.h pcf/9x15.h :
  includes/openiboot-asmhelpers.h includes/stb_image.h includes/util.h :
