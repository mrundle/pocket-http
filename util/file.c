#include "file.h"

#include "log.h"

#include <sys/stat.h>

int
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
