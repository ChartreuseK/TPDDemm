/*
 * Hayden Kroepfl - 2017
 */
#include "filesys.h"
#include "log.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// Static data
static struct DIRENTRY cur;
static struct DIRENTRY curref;	// Current referenced file
static struct dirent *curdent;
static DIR *curdir;
static const char *curpath;
static int curind;
static FILE *curfile;


void dirent_to_cur(void)
{
	// Stat it for filesize
	char *filepath = alloca(strlen(curpath)+1+strlen(curdent->d_name)+1);
	sprintf(filepath, "%s/%s", curpath, curdent->d_name);
	
	struct stat st;
	if (stat(filepath, &st) < 0) {
		logerror("Failed to stat file \"%s\": [%d] %s", filepath, errno, strerror(errno));
	}
	
	strcpy(cur.fname, curdent->d_name);
	
	int i,j,dotpos=-1;
	// Copy up to first 6 characters
	for (i = 0, j = 0; i < 6; i++) {
		if (curdent->d_name[j] == '.') {
			dotpos = j; 
			break;
		}
		if (curdent->d_name[j] == 0) {
			break;
		}
		
		cur.filename[i] = toupper(curdent->d_name[j]);
		j++;
	}
	// Pad with spaces to 6 characters
	for (; i < 6; i++)
		cur.filename[i] = ' ';
	// .
	cur.filename[i++] = '.';
	
	// Check if we found the dot
	if (dotpos < 0) {
		// Search for the dot
		for (; j < 256 && curdent->d_name[j]; j++)
			if (curdent->d_name[j] == '.')
				break;
		dotpos = j;
	}
	// If we found a dot		
	if (curdent->d_name[dotpos] == '.') {
		char ch = curdent->d_name[++j];
		cur.filename[i++] = ch ? toupper(ch) : 0;
		ch = curdent->d_name[++j];
		cur.filename[i++] = ch ? toupper(ch) : 0;
	} 
	cur.filename[i++] = 0;	
	
	
	cur.attrib = 'F';
	cur.len = st.st_size > 0xFFFF ? 0xFFFF : st.st_size;
}


/*
 * fs_init - Initialize the filesystem interface starting at the specified path
 */
int fs_init(const char *path)
{
	curdir = opendir(path);
	if (curdir == NULL) {
		logerror("Unabled to open directory \"%s\": [%d] %s", path, errno, strerror(errno));
		return -1;
	}
	
	curpath = path;
}

/*
 * fs_dir_first -- Returns the first directory entry
 */
struct DIRENTRY *fs_dir_first(void)
{	
	// Clear current entry
	memset(&cur.filename[0], 0, 24);
	cur.attrib = 0;
	cur.len = 0;
	
	curind = 0;
	
	rewinddir(curdir);
	
	do {
		curdent = readdir(curdir);
		
		if (!curdent) {
			loginfo("Current directory is empty, dir_first returning empty");
			return &cur;
		}	
	} while (curdent->d_type != DT_REG);
	
	// Found first regular file
	dirent_to_cur();
	return &cur;
	
}

struct DIRENTRY *fs_dir_next(void)
{
	// Clear current entry
	memset(&cur.filename[0], 0, 24);
	cur.attrib = 0;
	cur.len = 0;
	
	do {
		curdent = readdir(curdir);
		
		if (!curdent) {
			loginfo("Reached end of directory dir_next returning empty");
			return &cur;
		}	
	} while (curdent->d_type != DT_REG);
	
	curind++;
	
	// Found next regular file
	dirent_to_cur();
	return &cur;
}

struct DIRENTRY *fs_dir_prev(void)
{
	// Clear current entry
	memset(&cur.filename[0], 0, 24);
	cur.attrib = 0;
	cur.len = 0;
	
	if (curind == 0) {
		loginfo("Attempt to seek before first directory entry");
		return &cur;
	}
	
	rewinddir(curdir);
	for (int i = 0; i < curind; i++) {
		do {
			curdent = readdir(curdir);
			
			if (!curdent) {
				loginfo("Current directory is empty, dir_first returning empty");
				return &cur;
			}	
		} while (curdent->d_type != DT_REG);
	}
	
	curind--;
	
	// Found prev regular file
	dirent_to_cur();
	return &cur;
}

/*
 * fs_find_file Find a file in the directory by name
 */
struct DIRENTRY *fs_find_file(char *filename)
{
	rewinddir(curdir);
	do {		
		do {
			curdent = readdir(curdir);
			
			if (!curdent) {
				loginfo("Reached end of directory searching for \"%.*s\"", 24, filename);
				// Copy info to create new file
				cur.attrib = 'F';
				strcpy(cur.filename, filename);
				int i;
				for (i = 0; i < 6; i++) {
					if (cur.filename[i] == ' ')
						break;
					cur.fname[i] = cur.filename[i];
				}
				cur.fname[i++] = '.';
				cur.fname[i++] = cur.filename[7];
				cur.fname[i++] = cur.filename[8];
				cur.len = 0;
				
				// Copy into curref
				memcpy(&curref, &cur, sizeof(cur));
				
				return NULL;
			}	
		} while (curdent->d_type != DT_REG);
		dirent_to_cur();
		printf("F1: \"%.*s\" F2: \"%.*s\"\n", 9, filename, 9, cur.filename);
	} while (strncmp(filename, cur.filename, 9) != 0);
	
	// Copy into curref
	memcpy(&curref, &cur, sizeof(cur));
	
	rewinddir(curdir);
	curind = 0;
	
	return &curref;
}

/*
 * fs_open - Open the file specified by curref
 */
int fs_open(const char *mode)
{
	// Check if a file is specified
	if (curref.filename[0] == 0) {
		logwarn("OPEN - No file specified by curref");
		return -1;
	}
	// Create the full pathname
	char *filepath = alloca(strlen(curpath)+1+strlen(curref.fname)+1);
	sprintf(filepath, "%s/%s", curpath, curref.fname);
	
	curfile = fopen(filepath, mode);
	if (curfile == NULL) {
		logwarn("OPEN - Unable to open file: [%d] %s", errno, strerror(errno));
		return -2;
	}
	loginfo("OPEN - Opened file \"%s\"", filepath);
	return 0;
}

/*
 * Close an open file
 */
int fs_close(void)
{
	if (curfile == NULL) {
		logwarn("CLOSE - Attempt to close file when none open");
		return -1;
	}
	fclose(curfile);
	curfile = NULL;
	return 0;
}


/*
 * Read from the current file
 */
int fs_read(int bytes, uint8_t *buf)
{
	int rbytes;
	if (curfile == NULL) {
		logwarn("READ - Attempt to read file when none open");
		return -1;
	}

	rbytes = fread(buf, 1, bytes, curfile);
	return rbytes;
}

/*
 * Write to the current file
 */
int fs_write(int bytes, uint8_t *buf)
{
	if (curfile == NULL) {
		logwarn("WRITE - Attempt to write file when none open");
		return -1;
	}
	return fwrite(buf, 1, bytes, curfile);
}

/*
 * fs_exit - Close the filesystem interface
 */
void fs_exit(void)
{
	if (curdir) {
		closedir(curdir);
		curdir = NULL;
	}
}
