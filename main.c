/*
 * Hayden Kroepfl - 2017
 * 
 * Tandy Portable Disk Drive Emulator
 * 
 * Emulates the file access mode of the TPDD with a linux system over a serial 
 * port. Currently only implements a single directory like the actual drive, 
 * and doesn't implement the extensions for subdirectory access.
 * 
 * Filenames are truncated down to the 6.2 format used with the TPDD, if 
 * multiple files in the specified directory would truncate to the same name
 * then the first one in the directory listing will be accessed.
 */
#include "tpdd.h"
#include "serial.h"
#include "log.h"
#include "filesys.h"
#include <stdint.h>


int main(int argc, char **argv)
{
	int rval;
	
	loginfo("Program starting");
	
	loginfo("Filesystem test");
	fs_init("./testdir");
	struct DIRENTRY *d = fs_dir_first();
	
	printf("DENT: %.*s - attrib '%c' len = %hu\n", 24, d->filename, d->attrib, d->len);
	
	while (d->filename[0]) {
		d = fs_dir_next();
		if (d->filename[0]) {
			printf("DENT: %.*s - attrib '%c' len = %hu\n", 24, d->filename, d->attrib, d->len);
		}
	}
	fs_dir_first();
	
	
	if (serial_init() < 0) {
		rval = -1;
		goto exit;
	}
	
	while (1) {
		if (serial_read_byte() == 0x5A) {
			if (serial_read_byte() == 0x5A) {
				tpdd_request();
			}
		}	
	}
	
	
	fs_exit();
exit_serial:
	serial_close();
exit:
	loginfo("Program exiting - code %d", rval);
	return rval;
}

