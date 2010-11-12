OIBC_OBJS = oibc.o
LIBRARIES = -L/opt/local/lib -lusb -lpthread -lreadline
CFLAGS += -DHAVE_GETEUID -I/opt/local/include

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@


all:	oibc

oibc:	$(OIBC_OBJS)
	$(CC) $(CFLAGS) $(OIBC_OBJS) $(LIBRARIES) -o $@

clean:
	-rm *.o
	-rm oibc
	-rm loadibec

