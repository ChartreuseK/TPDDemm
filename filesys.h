/*
 * Hayden Kroepfl - 2017
 */
#ifndef FILESYS_H
#define FILESYS_H

#include <stdint.h>

struct DIRENTRY
{
	char filename[24]; // TRS-80 only uses 6.2 format names, rest null padded
	uint8_t attrib;
	uint16_t len;      // Files larger than 0xFFFF will be hidden
	
	char fname[256];   // Host filename
};

int fs_init(const char *path);

struct DIRENTRY *fs_dir_first(void);
struct DIRENTRY *fs_dir_next(void);
struct DIRENTRY *fs_dir_prev(void);
struct DIRENTRY *fs_find_file(char *filename);
void fs_exit(void);

int fs_open(const char *mode);
int fs_close(void);
int fs_read(int bytes, uint8_t *buf);
int fs_write(int bytes, uint8_t *buf);



#endif
