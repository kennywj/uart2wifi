#
# 'make depend' to automatically generate dependences
# 'make' to rebuild object codes
# 'make clean' remove object files
#
ODIR = ./
LIB = ../../build/libmbedtls.a
INCS =  -I./ -I../include -I../include/mbedtls
VPATH = ../library:../programs/test:../programs/ssl

CFLAGS += -fPIC -DU2W_SIM -DMBEDTLS_CONFIG_FILE='<u2w_config.h>'

SRCS = aes.c aesni.c arc4.c asn1parse.c asn1write.c base64.c bignum.c blowfish.c	\
	camellia.c ccm.c certs.c cipher.c cipher_wrap.c	cmac.c	ctr_drbg.c debug.c	\
	des.c dhm.c ecdh.c ecdsa.c ecjpake.c ecp_curves.c ecp.c entropy.c entropy_poll.c	\
	error.c gcm.c havege.c hmac_drbg.c md2.c md4.c md5.c md.c md_wrap.c memory_buffer_alloc.c \
	net_sockets.c oid.c padlock.c pem.c pk.c pkcs11.c pkcs12.c pkcs5.c pkparse.c	\
	pk_wrap.c pkwrite.c platform.c ripemd160.c rsa.c sha1.c sha256.c sha512.c	\
	ssl_cache.c ssl_ciphersuites.c ssl_cli.c ssl_cookie.c ssl_srv.c ssl_ticket.c	\
	ssl_tls.c threading.c timing.c version.c version_features.c x509.c x509_create.c	\
	x509_crl.c x509_crt.c x509_csr.c x509write_crt.c x509write_csr.c xtea.c	\
	selftest.c ssl_client1.c

OBJS = $(patsubst %.c,$(ODIR)/%.o,$(SRCS))

.PHONY: clean depend

all: $(LIB)

$(LIB): $(OBJS)
	@$(AR) cr $@ $^
	@echo "    Archive    $(notdir $@)"

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

depend: $(SRCS)
	makedepend $(INCS) $<
