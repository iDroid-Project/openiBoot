OIBC_OBJS = oibc.o
LOADIBEC_OBJS = loadibec.c
LIBRARIES = -lusb-1.0 -lpthread -lreadline
LOADIBEC_LIBS = -lusb-1.0
CFLAGS += -DHAVE_GETEUID

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

install:
	install -m +x oibc /usr/bin/
