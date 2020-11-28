#include "https.h"

#include "log.h"
#include "types.h"

#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* created via `make` */
#define CERT_PATH "./certs/cert.pem"

static SSL_CTX *ctx = NULL;

#define log_error_ssl(msg) do {                            \
	char err[256];                                         \
	ERR_error_string_n(ERR_get_error(), err, sizeof(err)); \
	log_error("%s: %s", msg, err);                         \
} while(0)

static int
load_certs(void)
{
	if (ctx == NULL) {
		return -1;
	}

	/* load the cert from file */
	if (SSL_CTX_use_certificate_file(ctx, CERT_PATH, SSL_FILETYPE_PEM) != 1) {
		log_error_ssl("SSL_CTX_use_certificate_file");
		return -1;
	}

	/* load private key, in this case from the same file */
	if (SSL_CTX_use_PrivateKey_file(ctx, CERT_PATH, SSL_FILETYPE_PEM) != 1) {
		log_error_ssl("SSL_CTX_use_PrivateKey_file");
		return -1;
	}

	/* verify */
	if (SSL_CTX_check_private_key(ctx) != 1) {
		log_error_ssl("SSL_CTX_check_private_key");
		return -1;
	}

	return 0;
}
#if 0
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
#endif

static int
ssl_init(void)
{
	SSL_library_init();
	OpenSSL_add_all_algorithms(); /* load crypto libraries */
	SSL_load_error_strings();     /* register error messages */

	/* create new client-method instance */	
	const SSL_METHOD *const method = TLSv1_2_server_method();
	if (method == NULL) {
		log_error("failed to init ssl method");
		return -1;
	}

	/* create new context */
	if ((ctx = SSL_CTX_new(method)) == NULL) {
		log_error_ssl("SSL_CTX_new");
		return -1;
	}

	if (load_certs() != 0) {
		log_error("failed to load certs");
	}
	
	return 0;
}

static SSL *
ssl_connect_desc(const int socket_desc)
{
	if (ctx == NULL) {
		log_error("null ctx");
		return NULL;
	}

	SSL *ssl = SSL_new(ctx);

	if (ssl == NULL) {
		log_error_ssl("SSL_new");
		goto err;
	}

	if (SSL_set_fd(ssl, socket_desc) == 0)  {
		log_error_ssl("SSL_set_fd");
		goto err;
	}

	if (SSL_accept(ssl) == -1) {
		log_error_ssl("SSL_accept");
		goto err;
	}

	return ssl;

err:
	if (ssl != NULL) {
		SSL_free(ssl);
	}
	return NULL;
}

int
https_setup(void)
{
	if (ssl_init() != 0) {
		log_error("failed to init ssl");
		return -1;
	}

	return 0;
}

/* TODO use? */
static void
log_certs(const SSL *const ssl)
{
    X509 *const cert = SSL_get_peer_certificate(ssl);
	if (cert == NULL) {
		log_info("no client certificate");
		return;
	}
	char *const subject = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
	char *const issuer = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
	log_info("Client Certificate: ");
	log_info("  Subject: %s", subject == NULL ? "(null)" : subject);
	log_info("  Issuer: %s", issuer == NULL ? "(null)" : issuer);
	if (subject) free(subject);
	if (issuer) free(issuer);

	X509_free(cert);
	return;
}

/*
 * TODO implement buffered reader
 */
int https_handle(const int sd)
{
	log_info("received request");

	SSL *ssl = ssl_connect_desc(sd);
	if (ssl == NULL) {
		log_error("failed to connect");
		return -1;
	}

	log_certs(ssl);

	/* read line */
	char buf[1028];
	char *nwln = NULL;
	int nread = SSL_read(ssl, buf, sizeof(buf));
	if (nread <= 0 || (nwln = strchr(buf, '\n')) == NULL) {
		log_error_ssl("no info read, or no newline found");
		//return -1; // XXX
	} else {
		const unsigned len = (unsigned)(nwln - buf);
		char buf2[1028];
		strncpy(buf2, buf, len);
		buf2[len] = '\0';
		log_info("got line: %s", buf2);

		buf[sizeof(buf)-1] = '\0';
		log_info("got all: '%s'", buf);
	}
	
	const char resp[] = "HTTP/1.0 200 OK\n";
	write(sd, resp, strlen(resp));
	return 0;
}