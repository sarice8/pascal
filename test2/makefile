
# SSL compiler, built using generic SSL runtime module & scanner

COMPILERS_DIR  = /home/sarice/compilers
NSSL_DIR       = ${COMPILERS_DIR}/nssl

SSL_DIR        = ${NSSL_DIR}


test.h: test.ssl ${SSL_DIR}/ssl
	${SSL_DIR}/ssl -l -r -d -c test 


