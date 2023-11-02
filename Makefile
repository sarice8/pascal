
all:
	cd compiler_kit ; make
	cd src ; make

clean:
	cd compiler_kit ; make clean
	cd src ; make clean

bootstrap:
	cd compiler_kit ; make bootstrap

