OIBC_OBJS = oibc.o
LOADIBEC_OBJS = loadibec.c
LIBRARIES = -L/opt/local/lib -lusb-1.0 -lpthread -lreadline
LOADIBEC_LIBS = -L/opt/local/lib -lusb-1.0
CFLAGS += -DHAVE_GETEUID -I/opt/local/include

ifeq ($(DEBUG),YES)
        CFLAGS += -ggdb
endif

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@


all:	oibc

oibc:	$(OIBC_OBJS)
	$(CC) $(CFLAGS) $(OIBC_OBJS) $(LIBRARIES) -o $@

loadibec:   $(LOADIBEC_OBJS)
	$(CC) $(CFLAGS) $(LOADIBEC_OBJS) $(LOADIBEC_LIBS) -o $@

clean:
	-rm *.o
	-rm oibc
	-rm loadibec

