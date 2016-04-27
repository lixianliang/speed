

/*
 * Copyright (C) xianliang.li
 */


#ifndef LXL_FILES_H_INCLUDE
#define LXL_FILES_H_INCLUDE


#include <lxl_config.h>
//#include <lxl_core.h>


#define LXL_MAX_PATH        	1024


// lxl_lread_file
ssize_t lxl_read_file(lxl_file_t *file, char *buf, size_t size, off_t offset);
ssize_t lxl_read_file1(lxl_file_t *file, char *buf, size_t size);
#define lxl_read_file_n			"read"

ssize_t lxl_write_file(lxl_file_t *file, char *buf, size_t size);
ssize_t lxl_write_file_offset(lxl_file_t *file, char *buf, size_t size, off_t offset);
#define lxl_write_file_n 		"write"

#define lxl_file_info(file, sb)	stat(file, sb)
#define lxl_file_info_n			"stat()"

#define lxl_fd_info(fd, sb)		fstat(fd, sb)
#define lxl_fd_info_n			"fstat()"


#define lxl_file_size(sb)		(sb)->st_size

#define lxl_file_separator(c)	((c) == '/')
#define	lxl_is_slash(c)			(c == '/')
#define lxl_is_obspath(path)	lxl_is_slash(path[0])
#define lxl_is_path(path)		lxl_is_slash(path[strlen(path) - 1])


int lxl_mkdir(char *dir);


#endif	/* LXL_FILES_H_INCLUDE */
