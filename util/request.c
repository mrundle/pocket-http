#include "request.h"

#include "log.h"
#include "io.h" // merge with this?

#include <sys/stat.h>

#define HTTP_HEADER_LEN_MAX 1024

enum http_method {
	HTTP_METHOD_GET,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_POST,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,
	HTTP_METHOD_OPTIONS,
	HTTP_METHOD_TRACE,
	HTTP_METHOD_UNKNOWN,
};

static bool
is_http_method_supported(const enum http_method method)
{
	switch (method) {
	case HTTP_METHOD_GET:
	case HTTP_METHOD_HEAD:
		return true;
	default:
		return false;
	}
}

static enum http_method
str_to_http_method(const char *str)
{
#define s2http(_method) do { \
	if (strcmp(str, #_method) == 0) { \
		return HTTP_METHOD_##_method; \
	} \
} while (0)
	str = str == NULL ? "unknown" : str;
	s2http(GET);
	s2http(HEAD);
	s2http(POST);
	s2http(PUT);
	s2http(DELETE);
	s2http(OPTIONS);
	s2http(TRACE);
	return HTTP_METHOD_UNKNOWN;
}

struct http_header {
	const char *method;
	const char *path;
	const char *version;
	const char *content_type;
	const char *content_length;
	const char *host;
	const char *user_agent;
	const char *accept;
	const char *accept_language;
	const char *accept_encoding;
	const char *connection;
	const char *upgrade_insecure_requests;
	const char *cache_control;
};

/*
 * Like strtok, but accepts a specific delimiter string rather than char.
 * Using the re-entrant version to support thread safety. See the strtok(3) man page.
 */
static char *
sstrtok_r(char *str, const char *delim_str, char **saveptr)
{
	if (saveptr == NULL) {
		/* error */
		return NULL;
	}
	if (str == NULL) {
		if (saveptr == NULL || *saveptr == NULL) {
			return NULL;
		}
		str = *saveptr;
	}
	*saveptr = strstr(str, delim_str);
	if (*saveptr == NULL) {
		return NULL;
	}

	for (unsigned i = 0; i < strlen(delim_str); i++) {
		**saveptr = '\0';
		*saveptr = *saveptr + 1;
	}

	return str;
}

/*
 * Reads a header and parses pointers into `struct http_response`
 */
int
parse_http_header(
	io_recv_fn_t io_recv_fn,
	const void *const io_arg,
	char *const buf,
	const unsigned buf_len,
	struct http_header *const hdr)
{
	if (io_arg == NULL || buf == NULL || hdr == NULL) {
		log_error("invalid inputs");
		return 500;
	}

	/* read the request */
	ssize_t nread = io_recv_fn(buf, buf_len, io_arg);
	if (nread < 0 || nread >= (ssize_t)buf_len) {
		log_error("failed to read http request, nread=%ld", nread);
		return 500;
	}

	if (strstr(buf, "\r\n\r\n") == NULL) {
		log_error("didn't find \\r\\n\\r\\n in http request");
		return 500;
	}

	char *save;
	hdr->method = sstrtok_r(buf, " ", &save);
	hdr->path = sstrtok_r(NULL, " ", &save);
	hdr->version = sstrtok_r(NULL, "\r\n", &save);

	while (true) {
		char *k = sstrtok_r(NULL, ": ", &save);
		if (k == NULL) {
			break;
		}
		char *v = sstrtok_r(NULL, "\r\n", &save);
		if (v == NULL) {
			log_error("no correspondign value for %s in http header", k);
			break;
		}
		if (strcmp(k, "Content-Type") == 0) {
			hdr->content_type = v;
		} else if (strcmp(k, "Content-Length") == 0) {
			hdr->content_length = v;
		} else if (strcmp(k, "Host") == 0) {
			hdr->host = v;
		} else if (strcmp(k, "User-Agent") == 0) {
			hdr->user_agent = v;
		} else if (strcmp(k, "Accept") == 0) {
			hdr->accept = v;
		} else if (strcmp(k, "Accept-Language") == 0) {
			hdr->accept_language = v;
		} else if (strcmp(k, "Accept-Encoding") == 0) {
			hdr->accept_encoding = v;
		} else if (strcmp(k, "Connection") == 0) {
			hdr->connection = v;
		} else if (strcmp(k, "Upgrade-Insecure-Requests") == 0) {
			hdr->upgrade_insecure_requests = v;
		} else if (strcmp(k, "Cache-Control") == 0) {
			hdr->cache_control = v;
		} else {
			log_info("Warning: unrecognized http header: %s %s", k, v);
		}
	}

	log_info("received '%s %s %s' from %s (%s)",
		hdr->method,
		hdr->path,
		hdr->version,
		hdr->host,
		hdr->user_agent);

	return 0;
}

int
handle_http_request(io_send_fn_t io_send_fn, io_recv_fn_t io_recv_fn, const void *const io_arg)
{
	log_info("received request");

	/* TODO use return code, return to client */
	struct http_header hdr;
	char buf[HTTP_HEADER_LEN_MAX] = {0};
	if (parse_http_header(io_recv_fn, io_arg, buf, sizeof(buf), &hdr) != 0) {
		log_error("failed to parse http header");
		return -1;
	}

	const enum http_method method = str_to_http_method(hdr.method);
	if (!is_http_method_supported(method)) {
		log_error("http method not supported: %s", hdr.method);
	}

	if (method == HTTP_METHOD_GET || HTTP_METHOD_HEAD) {
		/* locate the file */
		struct stat st;
		if (stat(hdr.path, &st) == -1) {
			log_strerror("failed to stat %s", hdr.path);
		}
		if (!S_ISREG(st.st_mode)) {
			char resp[] = "HTTP/1.1 500 Server Error\r\n\r\n";
			log_info("request for non-regular file %s, returning 500", hdr.path);
			io_send_fn(resp, strlen(resp), io_arg);
			goto out;
		}
		const ssize_t file_size = st.st_size;

		char resp[128];
		snprintf(resp, sizeof(resp), "HTTP/1.0 200 OK\r\n"); // XXX 1.0 vs. 1.1
		io_send_fn(resp, strlen(resp), io_arg);

		snprintf(resp, sizeof(resp), "Content-Type: text/html\r\n");
		io_send_fn(resp, strlen(resp), io_arg);

		snprintf(resp, sizeof(resp), "Content-Length: %lu\r\n\r\n", file_size);
		io_send_fn(resp, strlen(resp), io_arg);

		if (method == HTTP_METHOD_GET) {
			if (io_send_file(hdr.path, io_send_fn, io_arg) != 0) {
				log_error("failed to serve file");
				return -1;
			}
		}
	}

out:
	return 0;
}
