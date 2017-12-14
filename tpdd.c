/*
 * Hayden Kroepfl - 2017
 */
#include "tpdd_codes.h"
#include "tpdd.h"
#include "serial.h"
#include "log.h"
#include "filesys.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#define ARRLEN(x) (sizeof((x))/sizeof((x)[0]))


#define REQ_DIRREF_LEN    26
#define REQ_OPEN_LEN      1
#define REQ_CLOSE_LEN     0
#define REQ_READ_LEN      0
#define REQ_DELETE_LEN    0
#define REQ_FORMAT_LEN    0
#define REQ_STATUS_LEN    0
#define REQ_COND_LEN      0
#define REQ_RENAME_LEN    25

// Static Data
struct FILEREF {
	char name[24];
	uint8_t attrib;
} tpdd_fileref;

static uint8_t curchk;


// Prototypes
uint8_t tpdd_checksum(void);
void add_checksum(uint8_t *buf, int len);
void tpdd_read_file_return(uint8_t len, uint8_t *data);
void tpdd_dirref_return(char *filename, uint8_t attrib, uint16_t size, uint8_t free_sectors);
void tpdd_normal_return(uint8_t code);
void tpdd_conditon_return(uint8_t cond);
void tpdd_request_dirref(void);
void tpdd_dirref_ref(char *filename, uint8_t attrib);
void tpdd_dirref_first(void);
void tpdd_dirref_next(void);
void tpdd_dirref_prev(void);
void tpdd_dirref_end(void);
void tpdd_request_open(void);
void tpdd_request_close(void);
void tpdd_request_read(void);
void tpdd_request_write(void);
void tpdd_request_delete(void);
void tpdd_request_format(void);
void tpdd_request_status(void);
void tpdd_request_cond(void);
void tpdd_request_rename(void);

/*
 * tpdd_checksum - Calculates checksum for packet based on bytes in its 
 * request type, length, and data, but not including the preabmle.
 * (This seems like a kinda crappytpdd_checksum, is it actualy correct?)
 * (The length appears to be incorrect, seems to be the sum of bytes instead,
 *  which is a much better checksum)
 */
uint8_t tpdd_checksum(void)
{
	return curchk ^ 0xFF;
}
void add_checksum(uint8_t *buf, int len)
{
	for (int i = 0; i < len; i++)
		curchk += buf[i];
}

/****************************** RETURN COMMANDS ******************************/

/*
 * tpdd_read_file_return - Send a packet containing file data
 */
void tpdd_read_file_return(uint8_t len, uint8_t *data)
{
	uint8_t buf[132];
	
	buf[0] = RET_RDFILE; 
	buf[1] = len;        
	memcpy(&buf[2], data, len);
	
	curchk = 0; add_checksum(buf, len+2);
	buf[len+2] = tpdd_checksum();
	loginfo("RETURN READ - len %d chksum %02xh", buf[0], buf[len+2]);
	serial_write(buf, len+3);
}

/*
 * tpdd_dirref_return - Send a directory reference entry packet
 */
void tpdd_dirref_return(char *filename, uint8_t attrib, uint16_t size, uint8_t free_sectors)
{
	uint8_t buf[31];
	
	buf[0] = RET_DIRREF;    // Type
	buf[1] = 28;            // Length
	
	memset(&buf[2], 0, 24);
	strncpy(&buf[2], filename, 24);
	
	buf[26] = attrib;       // Attribute byte
	buf[27] = (size & 0xFF00) >> 8;	    // Big endian size of file
	buf[28] = size & 0xFF;  
	buf[29] = free_sectors;
	
	curchk = 0; add_checksum(buf, 30);
	buf[30] = tpdd_checksum();
	
	loginfo("RETURN DIRREF - len %d filename \"%.*s\" attrib '%c' size %d free sectors %d chksum %02xh", 
	        buf[1], 24, &buf[2], buf[26], size, free_sectors, buf[30]);
	serial_write(buf, ARRLEN(buf));
}

/*
 * tpdd_normal_return - Send a normal return packet with specified status code
 */
void tpdd_normal_return(uint8_t code)
{
	uint8_t buf[4];
	
	buf[0] = RET_NORMAL;    // Type
	buf[1] = 1;             // Length
	buf[2] = code;          // Error
	curchk = 0; add_checksum(buf, 3);
	buf[3] = tpdd_checksum();  // Checksum 
	
	loginfo("RETURN NORMAL - len %d code %02xh chksum %02xh", buf[1], buf[2], buf[3]);
	serial_write(buf, ARRLEN(buf));
	
}

/*
 * tpdd_condition_return - Send a drive condition return packet with specified
 * condition.
 */
void tpdd_conditon_return(uint8_t cond)
{
	uint8_t buf[4];
	
	buf[0] = RET_COND;      // Type
	buf[1] = 1;             // Length
	buf[2] = cond;          // Drive condition
	
	curchk = 0; add_checksum(buf, 3);
	buf[3] = tpdd_checksum();     // Checksum
	
	loginfo("RETURN CONDITION - len %d cond %02xh chksum %02xh", buf[1], buf[2], buf[3]);
	serial_write(buf, ARRLEN(buf));
}


/****************************** REQUEST COMMANDS ******************************/

/*
 * Assume we've read in the preamble already 5a5a (ZZ)
 */
void tpdd_request(void)
{
	uint8_t type;
	
	type = serial_read_byte();
	
	curchk = type;

	switch (type) {
	case REQ_DIRREF: tpdd_request_dirref(); break;
	case REQ_OPEN:   tpdd_request_open(); break;
	case REQ_CLOSE:  tpdd_request_close(); break;
	case REQ_READ:   tpdd_request_read(); break;
	case REQ_WRITE:  tpdd_request_write(); break;
	case REQ_DELETE: tpdd_request_delete(); break;
	case REQ_FORMAT: tpdd_request_format(); break;
	case REQ_STATUS: tpdd_request_status(); break;
	case REQ_COND:   tpdd_request_cond(); break;
	case REQ_RENAME: tpdd_request_rename(); break;
	default:
		// Invalid/Unrecognized type
		fprintf(stderr, "ERROR: Invalid Request %02x\n", type);
	}
}

/*
 * tpdd_request_dirref - Request for a directory reference/select a file
 */
void tpdd_request_dirref(void)
{
	uint8_t len;
	char filename[24];
	uint8_t attrib;
	uint8_t search;
	uint8_t chksum;
	
	len = serial_read_byte();
	curchk += len;
	
	if (len != REQ_DIRREF_LEN) {
		logerror("REQUEST DIRREF - Length was %d, expected %d",
		         len, 
		         REQ_DIRREF_LEN);
		return;
	}
	
	serial_read(filename, 24);   add_checksum(filename, 24);
	attrib = serial_read_byte(); curchk += attrib;
	search = serial_read_byte(); curchk += search;
	chksum = serial_read_byte();
	
	loginfo("REQUEST DIRREF - \"%.*s\" attrib '%c' search %02xh chksum %02xh", 24, filename, attrib, search, chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST DIRREF - Invalid checksum. Got %02x, expected %02x",
		         chksum, 
		         tpdd_checksum());
		return;
	}
	
	switch (search) {
	case SEARCH_REF:   tpdd_dirref_ref(filename, attrib); break;
	case SEARCH_FIRST: tpdd_dirref_first(); break;
	case SEARCH_NEXT:  tpdd_dirref_next(); break;
	case SEARCH_PREV:  tpdd_dirref_prev(); break;
	case SEARCH_END:   tpdd_dirref_end(); break;
	}
}
	
	
/*
 * tpdd_dirref_ref - Request to reference a file for open/delete
 */
void tpdd_dirref_ref(char *filename, uint8_t attrib)
{
	if (filename[0] == '\0') {
		tpdd_normal_return(ERR_NOFILE);
		return;
	}
	
	// Check if file exists
	struct DIRENTRY *d = fs_find_file(filename);
	if (d == NULL) {
		// If the file doesn't exist return a null reference
		// However the filename is kept as a curref so if an open
		// attempt is made to write to it it can.
		tpdd_dirref_return("", 0, 0, 100);
		loginfo("DIRREF REF - returning null reference - file not found");
	}
	else {
		tpdd_dirref_return(d->filename, d->attrib, d->len, 100);
		loginfo("DIRREF REF - returning entry '%.*s' attrib '%c' len %d",24,d->filename,d->attrib,d->len);
	}
}
	
/*
 * tpdd_dirref_first - Request for first entry in directory
 */
void tpdd_dirref_first(void)
{
	// Do a directory listing 
	// And get first item from directory
	struct DIRENTRY *d = fs_dir_first();
	
	// TEST ITEMS
	uint8_t free = 100;
	
	loginfo("DIRREF FIRST - returning entry '%.*s' attrib '%c' len %d", 24, d->filename, d->attrib, d->len);
	tpdd_dirref_return(d->filename, d->attrib, d->len, free);
}

/*
 * tpdd_dirref_next - Request for next entry in directory
 */
void tpdd_dirref_next(void)
{
	// Move to next item in directory, if no more then return null entry
	
	struct DIRENTRY *d = fs_dir_next();
	
	// TEST ITEMS
	uint8_t free = 100;
	
	loginfo("DIRREF NEXT- returning entry '%.*s' attrib '%c' len %d", 24, d->filename, d->attrib, d->len);
	tpdd_dirref_return(d->filename, d->attrib, d->len, free);
}


/*
 * tpdd_dirref_prev - Request for previous entry in directory
 */
void tpdd_dirref_prev(void)
{
	// Move to previous item in directory, 
	struct DIRENTRY *d = fs_dir_prev();
	
	// TEST ITEMS
	uint8_t free = 100;
	
	loginfo("DIRREF PREV - returning entry '%.*s' attrib '%c' len %d", 24, d->filename, d->attrib, d->len);
	tpdd_dirref_return(d->filename, d->attrib, d->len, free);
}
	
/*
 * tpdd_dirref_end - (?? What is this supposed to even do?)
 */
void tpdd_dirref_end(void)
{
	logwarn("DIRREF END - Unknown function!");
	tpdd_normal_return(ERR_NONE);
}
	
	
/*
 * tpdd_request_open - Open the currently referenced file
 */
void tpdd_request_open(void)
{
	uint8_t mode;
	uint8_t len;
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_OPEN_LEN) {
		logerror("REQUEST OPEN - Length of was %d, expected %d",
		         len, 
		         REQ_OPEN_LEN);
		return;
	}
	
	mode = serial_read_byte(); curchk += mode;
	chksum = serial_read_byte(); 
	
	loginfo("REQUEST OPEN - mode %02xh chksum %02xh", mode, chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST OPEN - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}
	
	// Open file for specified mode (1- WRITE 2- APPEND 3- READ)
	if (mode < 1 || mode > 3) {
		logerror("REQUEST OPEN - Invalid mode specified %02xh", mode);
		tpdd_normal_return(ERR_OFMT);
		return;
	}
	
	fs_open(mode == 1 ? "w" : mode == 2 ? "a" : "r");
	tpdd_normal_return(ERR_NONE);
}

/*
 * tpdd_request_close - Close the currently open file
 */
void tpdd_request_close(void)
{
	uint8_t len;
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_CLOSE_LEN) {
		logerror("REQUEST CLOSE - Length of was %d, expected %d",
		         len, 
		         REQ_CLOSE_LEN);
		return;
	}
	
	chksum = serial_read_byte();
	
	loginfo("REQUEST CLOSE - chksum %02xh", chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST CLOSE - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}

	// Close current file
	fs_close();
	tpdd_normal_return(ERR_NONE);
}

/*
 * tpdd_request_read - Read from currently open file
 */
void tpdd_request_read(void)
{
	uint8_t len;
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_READ_LEN) {
		logerror("REQUEST READ - Length of was %d, expected %d",
		         len, 
		         REQ_READ_LEN);
		return;
	}
	
	chksum = serial_read_byte();
	
	loginfo("REQUEST READ - chksum %02xh", chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST READ - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}

	uint8_t buf[128];
	int bytes = fs_read(128, &buf[0]);
	
	if (bytes < 0) {
		logerror("REQUEST READ - Error while reading from file");
		tpdd_normal_return(ERR_RDTIM);
	} else {
		loginfo("REQUEST READ - returning %d bytes", bytes);
		tpdd_read_file_return(bytes, &buf[0]);
	}
	
}

/*
 * tpdd_request_write - Write data to current file
 */
void tpdd_request_write(void)
{
	uint8_t len;
	uint8_t data[128];
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	if (len > 128) {
		logwarn("REQUEST WRITE - Length (%d) > 128 - truncating",
		         len);
		len = 128;
	}
	
	for (uint8_t i = 0; i < len; i++)  {
		data[i] = serial_read_byte();
		curchk += data[i];
	}
	
	chksum = serial_read_byte();
	
	loginfo("REQUEST WRITE - length %d chksum %02h", len, chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST WRITE - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}

	// TODO: Check if file is open for write or append, return error if not
	
	// Write data to file
	int bytes = fs_write(len, &data[0]);
	if (bytes < len) {
		tpdd_normal_return(ERR_DSKFULL);
		return;
	}
	
	// If successful
	tpdd_normal_return(ERR_NONE);
}

/*
 * tpdd_request_delete - Delete current file
 */
void tpdd_request_delete(void)
{
	uint8_t len;
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_DELETE_LEN) {
		logerror("REQUEST DELETE - Length of was %d, expected %d",
		         len, 
		         REQ_DELETE_LEN);
		return;
	}
	
	chksum = serial_read_byte();
	
	loginfo("REQUEST DELETE - chksum %02xh", chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST DELETE - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}
	
	// Delete file if it exists, and if we have a file open
	logwarn("REQUEST DELETE - Function Incomplete, ignored");
	tpdd_normal_return(ERR_NONE);
}

/*
 * tpdd_request_format - Format the current disk
 */
void tpdd_request_format(void)
{
	uint8_t len;
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_FORMAT_LEN) {
		logerror("REQUEST FORMAT - Length of was %d, expected %d",
		         len, 
		         REQ_FORMAT_LEN);
		return;
	}
	
	chksum = serial_read_byte();
	
	loginfo("REQUEST FORMAT - chksum %02xh", chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST FORMAT - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}
	
	
	// Format disk - IGNORE for now
	logwarn("REQUEST FORMAT - Function Incomplete, ignored");
	tpdd_normal_return(ERR_NONE);
}

/*
 * tpdd_request_status - Get query of drive status
 * TODO: What is the expected returns of this? Are they the normal status returns?
 *  Or is it the same as condition?
 */
void tpdd_request_status(void)
{
	uint8_t len;
	uint8_t chksum, chksum2;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_STATUS_LEN) {
		logerror("REQUEST STATUS - Length of was %d, expected %d",
		         len, 
		         REQ_STATUS_LEN);
		return;
	}
	
	chksum = serial_read_byte();
	
	loginfo("REQUEST STATUS - chksum %02xh", chksum);
	
	chksum2 = REQ_STATUS + len;
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST STATUS - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}
	
	// Hardcoded status for now
	logwarn("REQUEST STATUS - Function unknown, no error returned");
	tpdd_normal_return(ERR_NONE);
}

/*
 * tpdd_request_cond - Get condition of drive
 */
void tpdd_request_cond(void)
{
	uint8_t len;
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_COND_LEN) {
		logerror("REQUEST COND - Length of was %d, expected %d",
		         len, 
		         REQ_COND_LEN);
		return;
	}
	
	chksum = serial_read_byte();
	
	loginfo("REQUEST COND - chksum %02xh", chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST COND - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}
	
	// Hardcoded condition for now
	logwarn("REQUEST COND - Hardcoded status returned");
	tpdd_conditon_return(COND_PWRNORM | COND_NOPROT | COND_DISKIN | COND_NOCHG);
}
	
/*
 *  tpdd_request_rename - Rename the current file
 */
void tpdd_request_rename(void)
{
	uint8_t len;
	char filename[24];
	uint8_t attrib;
	uint8_t chksum;
	
	len = serial_read_byte(); curchk += len;
	
	if (len != REQ_RENAME_LEN) {
		logerror("REQUEST RENAME - Length of was %d, expected %d",
		         len, 
		         REQ_RENAME_LEN);
		return;
	}
	
	serial_read(filename, 24); add_checksum(filename, 24);
	attrib = serial_read_byte(); curchk += attrib;
	chksum = serial_read_byte();
	
	loginfo("REQUEST RENAME - filename \"%.*s\" attrib '%c' chksum %02xh", 24, filename, attrib, chksum);
	
	if (chksum != tpdd_checksum()) {
		logerror("REQUEST RENAME - Invalid checksum. Got %02x, expected %02x", 
		         chksum, 
		         tpdd_checksum());
		return;
	}
	
	// Rename file, error if file already exists or none open
	
	// Hardcoded condition for now
	logwarn("REQUEST RENAME - Function Incomplete, ignored");
	tpdd_normal_return(ERR_NONE);
}
