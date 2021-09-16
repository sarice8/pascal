
#  SSL runtime modules

ROOT         = /home/sarice/compilers
DEBUG_DIR    = ${ROOT}/debug/1.2.8


ssl_rt:    ssl_rt.o ssl_scan.o

ssl_rt.o:  ssl_rt.c ssl_rt.h
	cc -g -c ssl_rt.c -I${DEBUG_DIR}

ssl_scan.o: ssl_scan.c ssl_rt.h
	cc -g -c ssl_scan.c

