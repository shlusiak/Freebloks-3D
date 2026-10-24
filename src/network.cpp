/**
 * network.cpp
 * Author: Sascha Hlusiak
 *
 * Functions for the network messages of our own protocol
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "network.h"

/**
 * Prepares the network message *header
 * with length data_length
 * and type msg_type,
 * and sends it to target
 *
 * Returns: 0 on success, -1 on error
 **/
int network_send(int target,NET_HEADER *header,uint16 data_length,uint8 msg_type)
{
	/* Set checksums wildly computed from the available data in the structure */
	header->check1=(uint8)(data_length & 0x0055) ^ msg_type;
	/* Store data_length (in NETWORK BYTE ORDER) in the structure */
	header->data_length=htons(data_length);
	header->msg_type=msg_type;
	/* Compute and set another checksum. */
	header->check2=(uint8)(header->check1 ^ 0xD6)+msg_type;

#ifndef WIN32
	char c;
	/* Windows might block in send() if the socket is no longer connected;
	   check that with a recv() and bail out immediately on error. */
	if (recv(target,&c,1,MSG_PEEK|MSG_DONTWAIT)==0)return -1;
#endif
	if (send(target,(char*)header,data_length,0)!=-1)return 0;
	perror("send: ");
	return -1;
}

/**
 * Check a message for (in)validity
 * by simply repeating the computations (as above)
 * and comparing them with the checksums
 **/
bool network_check_message(NET_HEADER *data)
{
	/* If the length of the packet is (allegedly) too small: error! */
	if (ntohs(data->data_length)<sizeof(NET_HEADER))return false;
	/* Recompute both checksums */
	uint8 c1=(uint8)(ntohs(data->data_length) & 0x0055) ^ data->msg_type;
	uint8 c2=(c1 ^ 0xD6)+data->msg_type;
	/* Error if they differ, otherwise the message is OK */
	if (c1!=data->check1)return false;
	if (c2!=data->check2)return false;
	return true;
}

/* How long to wait for the rest of a message after its first bytes arrived */
#define RECV_TIMEOUT_MS 3000

/**
 * Reads exactly len bytes from sock into buf. A message may arrive split over several TCP
 * segments, so keep reading, but wait at most RECV_TIMEOUT_MS for each further chunk: a peer
 * that announces data and never sends it must not block the (single threaded) server forever.
 *
 * Returns len on success, -1 on error, timeout or when the peer closed the connection.
 **/
static int recv_exact(int sock, char *buf, int len)
{
	int got = 0;
	while (got < len)
	{
#ifdef WIN32
		int r = recv(sock, buf + got, len - got, 0);
		if (r <= 0)return -1;
#else
		/* Never block in recv(), wait in poll() with a timeout instead */
		int r = recv(sock, buf + got, len - got, MSG_DONTWAIT);
		if (r == 0)return -1;
		if (r < 0)
		{
			if (errno == EINTR)continue;
			if (errno != EAGAIN && errno != EWOULDBLOCK)return -1;

			struct pollfd pfd;
			pfd.fd = sock;
			pfd.events = POLLIN;
			pfd.revents = 0;
			int pr = poll(&pfd, 1, RECV_TIMEOUT_MS);
			if (pr < 0 && errno == EINTR)continue;
			if (pr <= 0)return -1;
			continue;
		}
#endif
		got += r;
	}
	return got;
}

/**
 * Reads a complete message from sock and stores it in data
 *
 * Returns: nullptr on success
 *           (char*)(-1) if no message is available
 *           otherwise a pointer to a string with the error message.
 **/
const char* read_network_message(int sock,NET_HEADER *data,int size)
{
	static const char* S_INVALID_PACKAGE="Received invalid network message";

	/* If not connected at all, "no message is available" */
	if (sock==0)return (char*)(-1);
	errno=0;
	/* Read exactly the NET_HEADER */
	int r=recv(sock,(char*)data,sizeof(NET_HEADER),0);
	if (r==-1)
	{
		/* EAGAIN (and <0> on Windows) means no packet is available for the socket */
		if (errno==EAGAIN)return (char*)(-1);
#ifdef WIN32
		if (errno==0)return (char*)(-1);
#endif
		return strerror(errno);
	}

	/* r==0 means the socket was closed properly. The connection to the server
	   was lost. */
	if (r==0)
	{
// 		printf("Connection lost.\n");
		return "Connection reset by peer";
	}
	/* If less data than a NET_HEADER was read, the header arrived in several
	   segments. Read the rest, otherwise it is an error */
	if ((unsigned int)r!=sizeof(NET_HEADER)){
		if (recv_exact(sock,&((char*)(data))[r],sizeof(NET_HEADER)-r)==-1)
			return S_INVALID_PACKAGE;
	}
	/* Check that the message is reasonably plausible */
	if (!network_check_message(data)){
// 		printf("Received malformed package!\n");
		return S_INVALID_PACKAGE;
	}
	/* The message seems to be OK. Determine the remaining amount of data */
	int remaining=ntohs(data->data_length)-sizeof(NET_HEADER);

	/* If there is no remaining data, the message consists only of a NET_HEADER. Success! */
	if (remaining==0)return nullptr;

	/* If the whole packet exceeds the size of the buffer: a mighty big ERROR */
	if (ntohs(data->data_length)>size)
	{
// 		printf("Received too big package!\n");
		return S_INVALID_PACKAGE;
	}

	/* Otherwise read the remaining data and write it into data after the NET_HEADER */
	r=recv_exact(sock,&((char*)(data))[sizeof(NET_HEADER)],remaining);
	/* If not exactly as much data could be read as NET_HEADER claims,
	   it was a packet error. */
	if (r!=remaining)
	{
// 		printf("Package size mismatch!\n");
		return S_INVALID_PACKAGE;
	}
	/* SUCCESS! */
	return nullptr;
}

#ifndef WIN32
/**
 * Closes a socket. It only exists to map closesocket() to close(),
 * since the functions have different names on Windows and *NIX
 **/
int closesocket(int s)
{
	return close(s);
}

/**
 * The abstract namespace (leading NUL in sun_path) only exists on Linux, see unix(7).
 * Other systems such as macOS and the BSDs treat it as an empty path and fail with ENOENT,
 * so they get a filesystem path instead. The length is computed from offsetof(sun_path),
 * because some systems have a sun_len byte before sun_family.
 **/
socklen_t local_socket_address(sockaddr_un *addr, const char *name)
{
	memset(addr, 0, sizeof(*addr));
	addr->sun_family = AF_UNIX;
	/* Linux: the leading NUL takes one byte. Elsewhere: the terminating NUL does. */
	if (strlen(name) + 1 > sizeof(addr->sun_path)) return 0;
#ifdef __linux__
	memcpy(&addr->sun_path[1], name, strlen(name));
	return (socklen_t)(offsetof(sockaddr_un, sun_path) + 1 + strlen(name));
#else
	strcpy(addr->sun_path, name);
	return (socklen_t)(offsetof(sockaddr_un, sun_path) + strlen(name) + 1);
#endif
}
#endif

