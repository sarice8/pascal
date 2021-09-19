

schema:   schema.o schema_scan.o
	blink from lib:c.o schema.o schema_scan.o to schema lib lib:lc.lib lib:amiga.lib addsym
#	cc -o schema schema.o schema_scan.o

# schema.o depends on schema.tbl when using "ssl -c"
schema.o:   schema.c schema.h schema_glob.h schema.tbl
	lc -b0 -d3 -cuwd schema.c
#	cc -c schema.c -g

schema_scan.o:   schema_scan.c schema.h schema_glob.h
	lc -b0 -d3 -cuwd schema_scan.c
#	cc -c schema.c -g

schema.h: schema.ssl
	ssl -l -d -c schema
	- delete schema.h
	- delete schema.tbl
	- delete schema.doc
	- delete schema.lst
	- delete schema.dbg
	rename ram_schema.h schema.h
	rename ram_schema.tbl schema.tbl
	rename ram_schema.doc schema.doc
	rename ram_schema.lst schema.lst
	rename ram_schema.dbg schema.dbg

ssl: schema.h

