#include "send.h"

#include "log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int
send_file(const char *const filepath, sender_fn_t sendf, const void *const arg)
{
	if (filepath == NULL || sendf == NULL) {
		return -1;
	}

	const int fd = open(filepath, O_RDONLY);
	if (fd < 0) {
		log_error("failed to open file %s", filepath);
		return -1;
	}

	char buf[128];
	ssize_t nread;
	ssize_t nwritten;
	while ((nread = read(fd, buf, sizeof(buf))) > 0) {
		nwritten = sendf(buf, (size_t)nread, arg);
		if (nwritten != nread) {
			log_error("failed to send");
			goto err;
		}
	}
	if (nread < 0) {
		log_error("failed to read");
		goto err;
	}

	close(fd);
	return 0;
err:
	close(fd);
	return -1;
}
