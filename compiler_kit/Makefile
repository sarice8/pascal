


bootstrap:
	cd ssl_rt ; make release_headers
	cd debug ; make release_headers
	cd schema ; make release_headers
	cd ssl ; make release_headers

	cd ssl_rt ; make bootstrap
	cd debug ; make bootstrap
	cd schema ; make bootstrap
	cd ssl ; make bootstrap


build:
	cd ssl_rt ; make
	cd debug ; make
	cd schema ; make
	cd ssl ; make

	cd ssl_rt ; make release
	cd debug ; make release
	cd schema ; make release
	cd ssl ; make release


clean:
	cd ssl_rt ; make clean
	cd debug ; make clean
	cd schema ; make clean
	cd ssl ; make clean
