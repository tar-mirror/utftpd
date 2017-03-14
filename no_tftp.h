#ifndef NO_TFTP_H
/* use system header if available */
#ifdef HAVE_ARPA_TFTP_H
#include <arpa/tftp.h>
#else
#define	SEGSIZE		512		/* data segment size */
#define	RRQ	01				/* read request */
#define	WRQ	02				/* write request */
#define	DATA	03				/* data packet */
#define	ACK	04				/* acknowledgement */
#define	ERROR	05				/* error code */

struct	tftphdr {
	short	th_opcode;			/* packet type */
	union {
		unsigned short	tu_block;	/* block # */
		short	tu_code;		/* error code */
		char	tu_stuff[1];		/* request packet stuff */
	} th_u;
	char	th_data[1];			/* data or error string */
};

#define	th_block	th_u.tu_block
#define	th_code		th_u.tu_code

#define	EUNDEF		0		/* not defined */
#define	ENOTFOUND	1		/* file not found */
#define	EACCESS		2		/* access violation */
#define	ENOSPACE	3		/* disk full or allocation exceeded */
#define	EBADOP		4		/* illegal TFTP operation */
#define	EBADID		5		/* unknown transfer ID */
#define	EEXISTS		6		/* file already exists */
#define	ENOUSER		7		/* no such user */
#endif
#endif
