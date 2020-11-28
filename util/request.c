#include "request.h"

#include "log.h"
#include "io.h" // merge with this?

#include <sys/stat.h>

static int
get_file_size(const char *const filepath, ssize_t *size_out)
{
	if (filepath == NULL || size_out == NULL) {
		return -1;
	}

	struct stat st;
	if (stat(filepath, &st) == -1) {
		log_strerror("failed to stat %s", filepath);
		return -1;
	}

	*size_out = st.st_size;
	return 0;
}

// arg will either be a socket or SSL
int
handle_http_request(io_send_fn_t io_send_fn, io_recv_fn_t io_recv_fn, const void *const io_arg)
{
	log_info("received request");

	/* read line */
	char buf[1028];
	char *nwln = NULL;
	const ssize_t nread = io_recv_fn(buf, sizeof(buf), io_arg);
	if (nread <= 0 || (nwln = strchr(buf, '\n')) == NULL) {
		log_error("no info read, or no newline found");
		return -1;
	}
	const unsigned len = (unsigned)(nwln - buf);
	char buf2[1028];
	strncpy(buf2, buf, len);
	buf2[len] = '\0';
	log_info("got line: %s", buf2);

	buf[sizeof(buf)-1] = '\0';
	log_info("got all: '%s'", buf);


	/* write the file */
	const char *const file = "docroot/index.html";
	ssize_t file_size;
	if (get_file_size(file, &file_size) != 0) {
		log_error("failed to get size of %s", file);
		return -1;
	}

	char resp[128];	
	snprintf(resp, sizeof(resp), "HTTP/1.0 200 OK\r\n");
	io_send_fn(resp, strlen(resp), io_arg);

	snprintf(resp, sizeof(resp), "Content-Type: text/html\r\n");
	io_send_fn(resp, strlen(resp), io_arg);

	snprintf(resp, sizeof(resp), "Content-Length: %lu\r\n\r\n", file_size);
	io_send_fn(resp, strlen(resp), io_arg);

	log_info("size of %s is %ld", file, file_size);
	if (io_send_file(file, io_send_fn, io_arg) != 0) {
		log_error("failed to serve file");
		return -1;
	}

	return 0;
}
