

/*
 * Copyright (C) xianliang.li
 */


//#include <lxl_log.h>
//#include <lxl_file.h>
//#include <lxl_files.h>
#include <lxl_config.h>
#include <lxl_core.h>


ssize_t 
lxl_read_file(lxl_file_t *file, char *buf, size_t size, off_t offset)
{
	ssize_t n;
	
	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "read: %d, %p, %lu, %ld", file->fd, buf, size, offset);

	if (file->sys_offset != offset) {
		if (lseek(file->fd, offset, SEEK_CUR) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "lseek() \"%s\" failed", file->name.data);
			return -1;
		}

		file->sys_offset = offset;
	}

	n = read(file->fd, buf, size);
	if (n == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "read() \"%s\" failed", file->name.data);
		return -1;
	}

	file->sys_offset += n;

	file->offset += n;

	return n;
}

ssize_t 
lxl_write_file_offset(lxl_file_t *file, char *buf, size_t size, off_t offset)
{
	ssize_t n, written;

	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "write: %d, %p, %ld, %ld", file->fd, buf, size, offset);

	if (file->sys_offset != offset) {
		if (lseek(file->fd, offset, SEEK_SET) == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "lseek() %s failed", file->name.data);
			return -1;
		}

		file->sys_offset = offset;
	}

	written = 0;
	for (; ;) {
		n = write(file->fd, buf + written, size);
		if (n == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "write() %s failed", file->name.data);
			return -1;
		}

		file->offset +=n;
		written += n;
		if ((size_t) n == size) {
			return written;
		}

		size -= n;
	}
}

ssize_t 
lxl_read_file1(lxl_file_t *file, char *buf, size_t size)
{
	ssize_t n;
	
	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "read: %d, %p, %lu", file->fd, buf, size);

	n = read(file->fd, buf, size);
	if (n == -1) {
		lxl_log_error(LXL_LOG_ALERT, errno, "read() \"%s\" failed", file->name.data);
		return -1;
	}

	return n;
}

ssize_t 
lxl_write_file(lxl_file_t *file, char *buf, size_t size)
{
	ssize_t n, written;

	lxl_log_debug(LXL_LOG_DEBUG_CORE, 0, "write: %d, %p, %ld", file->fd, buf, size);

	written = 0;
	for (; ;) {
		n = write(file->fd, buf + written, size);
		if (n == -1) {
			lxl_log_error(LXL_LOG_ALERT, errno, "write() %s failed", file->name.data);
			return -1;
		}

		written += n;
		if ((size_t) n == size) {
			return written;
		}

		size -= n;
	}
}

int 
lxl_mkdir(char *dir)
{
	char *pos;
	struct stat st;
	char d[256];

	snprintf(d, sizeof(d), "%s", dir);
	
	pos = d;
	while (pos) {
		pos = strchr(pos + 1, '/');
		if (pos) {
			*pos = '\0';
			if (stat(d, &st) == -1) {
				if (mkdir(d, 0744) == -1) {	/* access dir */
					*pos = '/';
					return -1;
				}
			}
			*pos = '/';
		}
	}

	if (stat(d, &st) == -1) {
		if (mkdir(d, 0744) == -1) {
			return -1;
		}
	}

	return 0;
}
