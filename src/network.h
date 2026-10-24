/**
 * network.h
 * Author: Sascha Hlusiak
 *
 * Central definitions of the network messages of our own protocol
 **/

#ifndef __NETWORK_H_INCLUDED_
#define __NETWORK_H_INCLUDED_


#include "constants.h"

const int TCP_PORT=59995;

/* Pseudo types for signed/unsigned 8 bit/16 bit numbers */
typedef signed short int16;
typedef signed char int8;
typedef unsigned short uint16;
typedef unsigned char uint8;

/* The following data must not be aligned to byte boundaries,
   to be independent of compiler and operating system. */
#pragma pack(1)

/* Header, always the head of a network message.
   Other data always contains a NET_HEADER as its first element, so the packets
   can be cast and the header is universally accessible. */
typedef struct
{
	uint8  check1;		/* A checksum to detect bad headers */
	uint16 data_length;	/* Length of the packet in bytes (network byte order) */
	uint8 msg_type;		/* Identifier of the message, to determine its type */
	uint8  check2;		/* A checksum to detect bad headers */
} NET_HEADER,NET_START_GAME,NET_GAME_FINISH,NET_REQUEST_UNDO;

/**
 * Message to request a player. Contains the preferred colour, or -1 for any
 **/

typedef struct
{
	NET_HEADER header;
	/* added in 1.5 */
	int8 player;
	uint8 name[16];
} NET_REQUEST_PLAYER;

/**
 * NET_GRANT_PLAYER: the server grants the client a player
 * NET_CURRENT_PLAYER: the server has set the current player
 **/

typedef struct
{
	NET_HEADER header;	/* The first element is always NET_HEADER, so it can be cast */
	int8 player;
} NET_GRANT_PLAYER,NET_CURRENT_PLAYER,NET_REQUEST_HINT,NET_REVOKE_PLAYER;

/**
 * Message about an action on a stone (placing it or taking it back)
 **/
typedef struct
{
	NET_HEADER header;
	int8 player;	/* Number of the player placing the stone */
	uint8 stone;	/* Number of the player's stone */
	uint8 mirror_count,rotate_count;	/* Mirroring and rotation of the stone */
	int8 x,y;	/* Coordinates where the stone is placed on the board */
} NET_SET_STONE,NET_UNDO_STONE;

/**
 * Server status packet that the server sends to the clients from time to time
 **/
typedef struct
{
	NET_HEADER header;
	int8 player,computer,clients; /* Number of human players, computer players and connected clients */
	int8 width,height; /* Size of the board */
	int8 stone_numbers_obsolete[STONE_SIZE_MAX]; /* Number of stones of certain sizes, OBSOLETE */
	int8 gamemode;
	/* added in 1.5, version 2 */
	int8 player_owner[PLAYER_MAX];
	uint8 client_names[CLIENTS_MAX][16]; /* names for each client */
	/* added in 1.6, version 3 */
	int8 version;
	int8 version_min;
	int8 stone_numbers[STONE_COUNT_ALL_SHAPES]; /* Number of stones of each shape */
} NET_SERVER_STATUS;

#define NET_SERVER_STATUS_VERSION (3)



/**
 * A chat message with text. Sent from the client to the server and back.
 **/
typedef struct
{
	NET_HEADER header;
	int8 client;	/* The server enters the number of the client that sent the message here */
	uint8 length;	/* Text length */
	uint8 text[1];	/* Array containing the text. If NET_CHAT is allocated dynamically and large
			   enough, later parts of the array can be accessed without problems */
} NET_CHAT;


/**
 * The client wants to change the game mode (while in the lobby)
 */
typedef struct
{
	NET_HEADER header;
	int8 version;	/* Version of this message, 1 */
	int8 width, height;
	int8 gamemode;
	int8 stone_numbers[STONE_COUNT_ALL_SHAPES]; /* Number of stones of each shape */
} NET_REQUEST_GAME_MODE;

#define NET_REQUEST_GAME_MODE_VERSION (1)



/* Be sure to reset the byte alignment */
#pragma pack()

/* Message type constants of the network messages above. These constants are what
   identifies the packets. */
const int MSG_REQUEST_PLAYER=1;
const int MSG_GRANT_PLAYER=2;
const int MSG_CURRENT_PLAYER=3;
const int MSG_SET_STONE=4;
const int MSG_START_GAME=5;
const int MSG_GAME_FINISH=6;
const int MSG_SERVER_STATUS=7;
const int MSG_CHAT=8;
const int MSG_REQUEST_UNDO=9;
const int MSG_UNDO_STONE=10;
const int MSG_REQUEST_HINT=11;
const int MSG_STONE_HINT=12;
const int MSG_REVOKE_PLAYER=13;
const int MSG_REQUEST_GAME_MODE=14;


/* Range check for the player, stone and orientation fields of a received NET_SET_STONE.
   The fields are used as array indices and rotation counters and must never be trusted. */
inline bool network_check_set_stone(const NET_SET_STONE *s)
{
	if (s->player < 0 || s->player >= PLAYER_MAX) return false;
	if (s->stone >= STONE_COUNT_ALL_SHAPES) return false;
	if (s->mirror_count > 1) return false;
	if (s->rotate_count > 3) return false;
	return true;
}

int network_send(int target,NET_HEADER *header,uint16 data_length,uint8 msg_type);

/* Checks a received message for (in)validity */
bool network_check_message(NET_HEADER *data);

/* Reads a complete network message from sock and stores it in *data */
const char* read_network_message(int sock,NET_HEADER *data,int size);


#ifndef WIN32
/* On Windows it is called closesocket(s); on Linux this provides it too, but it means close(s) */
int closesocket(int s);
#endif



#endif
