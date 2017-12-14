/*
 * Hayden Kroepfl - 2017
 */
#ifndef TPDD_CODES_H
#define TPDD_CODES_H

// Codes from:
//  http://bitchin100.com/wiki/index.php?title=TPDD_Base_Protocol#Type_12_-_Normal_Return
enum ERRCODES 
{
	ERR_NONE	= 0x00,	// Normal - No Error
	ERR_NOEXIST	= 0x10,	// File does not exist
	ERR_EXISTS	= 0x11,	// File exists
	ERR_NOFILE	= 0x30,	// No filename
	ERR_DIRSRCH	= 0x31, // Directory search error
	ERR_BANK	= 0x35,	// Bank error
	ERR_PARAM	= 0x36, // Parameter error
	ERR_OFMT	= 0x37, // Open format mismatch
	ERR_EOF		= 0x3f, // End of file
	ERR_NSTMK	= 0x40, // No start mark
	ERR_IDCRC	= 0x41, // CRC Error for ID
	ERR_SECLEN	= 0x42,	// Sector length error
	ERR_FMTVER	= 0x44, // Format verify error
	ERR_FMTINT	= 0x46, // Format interruption
	ERR_ERAOFF	= 0x47, // Erase offset error
	ERR_DATCRC	= 0x49, // CRC Error for ID
	ERR_SECNO	= 0x4a, // Sector number error
	ERR_RDTIM	= 0x4b,	// Read timeout
	ERR_SECNO2	= 0x4d, // Sector number error (DUPLICATE?)
	ERR_WPROT	= 0x50, // Write protected
	ERR_UNINIT	= 0x5e, // Uninitialized disk
	ERR_DIRFULL	= 0x60,	// Directory full
	ERR_DSKFULL	= 0x61, // Disk full
	ERR_TOOLONG	= 0x6e, // File too long
	ERR_NODSK	= 0x70,	// No disk present
	ERR_DSKCHG	= 0x71,	// Disk change error
};	 

enum RETCMDS
{
	RET_RDFILE	= 0x10,	// Read file return
	RET_DIRREF	= 0x11,	// Directory reference return
	RET_NORMAL	= 0x12,	// Normal return
	RET_COND	= 0x15,	// Drive condition return
};

enum REQCMDS
{
	REQ_DIRREF	= 0x00,	// Directory control (and file select)
	REQ_OPEN	= 0x01, 	// Open a file
	REQ_CLOSE	= 0x02,	// Close open file
	REQ_READ	= 0x03,	// Read file in
	REQ_WRITE	= 0x04,	// Write a file
	REQ_DELETE	= 0x05, 	// Delete file
	REQ_FORMAT	= 0x06,	// Format Disk
	REQ_STATUS	= 0x07,	// Drive status
	// REQ_SECMODE	= 0x08,	// Sector access mode? http://m100.bitchin100.narkive.com/XZu01NOx/fdc-mode-on-tpdd1
	REQ_COND	= 0x0C,	// Drive condition
	REQ_RENAME	= 0x0D,	// Rename file
};

enum DIRREF_SEARCH
{
	SEARCH_REF	= 0x00, // Reference file (for open/delete)
	SEARCH_FIRST	= 0x01, // First directory block
	SEARCH_NEXT     = 0x02, // Next directory block
	SEARCH_PREV	= 0x03, // Previous directory block
	SEARCH_END      = 0x04, // End directory reference
};

// Condition bitfield
#define COND_PWRLOW	0x00
#define COND_PWRNORM	0x01 // Normal power (battery)
#define COND_NOPROT	0x00
#define COND_PROT	0x02 // Write protected
#define COND_DISKIN	0x00
#define COND_DISKOUT	0x04 // Disk ejected
#define COND_NOCHG	0x00
#define COND_CHG	0x08 // Disk changed


#endif
