#include "http.h"

#include "log.h"
#include "types.h"

#include <unistd.h>

/*
 * TODO implement buffered reader
 */
int http_handle(const int socket_desc)
{
	log_info("received request");

	/* read line */
	char buf[1028];
	char *nwln = NULL;
	const ssize_t nread = read(socket_desc, buf, sizeof(buf));
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
	
	const char resp[] = "HTTP/1.0 200 OK\n";
	write(socket_desc, resp, strlen(resp));
	return 0;
}
