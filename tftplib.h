#ifndef tftplib_h
#define tftplib_h

#define TFTP_OFFSET 4 /* size of tftp header */

#ifndef OACK
#define OACK 6 /* option ACK */
#endif

union tftpbuf { struct tftphdr *hdr; char *buf; };

struct tftplib_ctrl {
	int netascii;
	size_t segsize;
	int retries;
	int timeout;
	int force_timeout;
	int force_stop;
	int send_garbage;
	int duplicate_ack;
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
};

extern int opt_verbose;


#define UTFTP_EC_OK 0
#define UTFTP_EC_LOCAL 1
#define UTFTP_EC_USAGE 2
#define UTFTP_EC_TIMEOUT 3
#define UTFTP_EC_PROTO 4
#define UTFTP_EC_NETWORK 5
#define UTFTP_EC_ERROR 6
#define UTFTP_EC_UNDEF 7




/* 
 * receive a file with TFTP
 */
int tftp_receive P__((const char *server, int port, const char *remotefilename, 
	const char *localfilename,struct tftplib_ctrl *flags));
/*
 * send a file to a TFTP server
 */
int tftp_send P__((const char *server, int port, const char *remotefilename, 
	const char *localfilename, struct tftplib_ctrl *flags));

int tftp_nak P__((int remotefd, int tftp_errcode, const char *errtext, struct tftplib_ctrl *flags));
size_t tftp_prepare_rq P__((int type, const char *server, unsigned short port, const char *remotefilename, 
	struct sockaddr_in *s_in, int *may_get_oack, struct tftplib_ctrl *flags));

struct tftplib_ctrl *tftp_setup_ctrl P__((struct tftplib_ctrl *old, size_t segsize));
void  tftp_free_ctrl P__((struct tftplib_ctrl *old));

#endif
