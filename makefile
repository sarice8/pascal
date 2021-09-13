

ssl:   ssl.o
	cc -o ssl ssl.o
#	blink from lib:c.o ssl.o to ssl lib lib:lc.lib lib:amiga.lib addsym

# ssl.o depends on ssl.tbl when using the "ssl -c" option
ssl.o:  ssl.c ssl.h ssl.tbl
	cc -g -c ssl.c
#	lc -b0 -d3 -cuwd ssl.c

ssl.h: ssl.ssl
	ssl -l -d -c ssl
	- rm -f ssl.bak
	- mv ssl ssl.bak
	- rm -f ssl.tbl.bak
	- mv ssl.tbl ssl.tbl.bak
	- rm -f ssl.h
	- rm -f ssl.doc
	- rm -f ssl.lst
	- rm -f ssl.dbg
	mv ram_ssl.h ssl.h
	mv ram_ssl.tbl ssl.tbl
	mv ram_ssl.doc ssl.doc
	mv ram_ssl.lst ssl.lst
	mv ram_ssl.dbg ssl.dbg

