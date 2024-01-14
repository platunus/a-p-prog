DESTDIR=
PREFIX=/usr/local

CC=gcc
CFLAGS=-g

all: pp3 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

OBJS+=main.o
OBJS+=pp3.o
OBJS+=comm.o
OBJS+=pp_ops.o
OBJS+=pp_utils.o
OBJS+=cf_p18f_qxx.o
OBJS+=cf_p16f_a.o
OBJS+=cf_p16f_b.o
OBJS+=cf_p16f_c.o
OBJS+=cf_p16f_d.o

pp3: $(OBJS)
	$(CC) $^ -o pp3

install: pp3 
	mkdir -p $(DESTDIR)/etc/pp3
	install -m0644 pp3_devices.dat $(DESTDIR)/etc/pp3/
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m0755 pp3 $(DESTDIR)$(PREFIX)/bin

clean:
	rm -f pp3 $(OBJS)
