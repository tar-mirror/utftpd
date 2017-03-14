#ifndef tftplib_h
#define tftplib_h

#define TFTP_OFFSET 4 /* size of tftp header */

#ifndef OACK
#define OACK 6 /* option ACK */
#endif

union tftpbuf { struct tftphdr *hdr; char *buf; };

struct utftpd_ctrl {
	int netascii;
	size_t segsize;
	int retries;
	int timeout;
	char *revision;
	union tftpbuf sendbuf; /* of segsize+TFTP_OFFSET size */
	union tftpbuf recvbuf; /* of segsize+TFTP_OFFSET size */
	size_t first_packet_length; /* see tftp_recv.c for a description */
	int recognize_oack;
	struct sockaddr_in s_in;
	/* */
	const char *filename;     /* qualified absolute filename */
	const char *origfilename; /* exactly what we received */
	int filefd;
	int remotefd;
	unsigned long bytes;
	pid_t pid;
	struct utftpd_vc *vc; /* version control */
};
struct utftpd_vc {
	int (* test) P__((const char *filename, struct utftpd_ctrl *flags));
	int (* commit) P__((const char *comment, struct utftpd_ctrl *flags));
	int (* checkout) P__((int mode, struct utftpd_ctrl *flags));
	int (* unget) P__((struct utftpd_ctrl *flags));
};

extern struct utftpd_vc utftpd_novc;
extern struct utftpd_vc utftpd_rcs;
extern struct utftpd_vc utftpd_sccs;

extern int wait_check_and_log(pid_t pid); 

/* prototypes for utftpd internal stuff */
int utftpd_recv P__((struct utftpd_ctrl *flags));
int utftpd_send P__((struct utftpd_ctrl *flags));
#define TSG_READ 0 /* get */
#define TSG_WRITE 1 /* get -e */
#define TSG_CREATE 2 /*  treat as "get -e", except in the "novc" driver. */
void do_nak P__((int remotefd, int ec, const char *et));
extern char *opt_rcs_ci;
extern char *opt_rcs_co;
extern char *opt_sccs_delta;
extern char *opt_sccs_get;
extern char *opt_sccs_unget;
extern char *opt_sccs_clean;
extern char *remoteip;
extern int nullfd;
extern int opt_suppress_naks;
extern int opt_verbose;


#define UTFTP_EC_OK 0
#define UTFTP_EC_LOCAL 1
#define UTFTP_EC_USAGE 2
#define UTFTP_EC_TIMEOUT 3
#define UTFTP_EC_PROTO 4
#define UTFTP_EC_NETWORK 5
#define UTFTP_EC_ERROR 6
#define UTFTP_EC_UNDEF 7



int utftpd_nak P__((int remotefd, int tftp_errcode, const char *errtext, struct utftpd_ctrl *flags));

#endif
