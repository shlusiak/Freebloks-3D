/**
 * gameclient.cpp
 * Author: Sascha Hlusiak
 *
 * Class for a network client.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#include "gameclient.h"



CGameClient::CGameClient()
{
	start_new_game(GAMEMODE_4_COLORS_4_PLAYERS);
	client_socket=0;
	memset(&status, 0, sizeof(status));
	status.version=NET_SERVER_STATUS_VERSION;
	status.version_min=1;
	status.width=status.height=20;
	for (int i=0;i<PLAYER_MAX;i++)
		status.player_owner[i]=PLAYER_COMPUTER;
	set_stone_numbers(0,0,0,0,0);
}

CGameClient::~CGameClient()
{
	/* Disconnect */
	Disconnect();
}

/**
 * Connects the game client to a game server
 * host: host name or IP of the server
 * port: port to connect to
 *
 * Returns: nullptr on success, otherwise a pointer to a string with the error message
 **/

const char* CGameClient::Connect(const char* host,int port, int blocking)
{
#ifndef WIN32
	if (port == 0)
	{
		sockaddr_un addr;

		errno=0;
		socklen_t addr_len = local_socket_address(&addr, host);
		if (addr_len == 0)
			return "Socket path too long";

		client_socket=socket(PF_UNIX,SOCK_STREAM,0);

		if (client_socket==-1)
		{
			// -1 would be reported as connected by isConnected()
			client_socket=0;
			return strerror(errno);
		}
		if (connect(client_socket,(sockaddr*)&addr,addr_len)==-1)
		{
			/* Connection failed */
			closesocket(client_socket);
			client_socket=0;
			return strerror(errno);
		}
		if (blocking == 0)
		{
			fcntl(client_socket,F_SETFL,O_NONBLOCK);
		}

		return nullptr;
	}
#endif


#if (defined HAVE_GETADDRINFO) || (defined WIN32)
	/* This includes IPv6 */
	sockaddr_in *addr=nullptr;
	struct addrinfo *addr_info,*p;
	int addr_len;
	const char* errormessage;

	errno=0;
	errormessage = nullptr;
	/* Resolve the host name to an IP */
	errno=getaddrinfo(host,nullptr,nullptr,&addr_info);
	if (errno)return gai_strerror(errno);

	p=addr_info;
	client_socket = 0;
	while (p)
	{
		if (p->ai_protocol==0 || p->ai_protocol==IPPROTO_TCP)
		{
			addr=(sockaddr_in*)malloc(p->ai_addrlen);
			memcpy(addr,p->ai_addr,p->ai_addrlen);

			addr->sin_port=htons(port);
			addr_len=p->ai_addrlen;
			client_socket=socket(p->ai_family,SOCK_STREAM,0);
			if (connect(client_socket,(sockaddr*)addr,addr_len) == 0)
			{
				errormessage = nullptr;
				free(addr);
				break;
			}
			/* Connection failed */
			closesocket(client_socket);
			client_socket=0;
		#ifdef WIN32
			errormessage = "Connection refused";
		#else
			errormessage = strerror(errno);
		#endif
			free(addr);
		}
		
		p=p->ai_next;
	}
	freeaddrinfo(addr_info);
	
	if (errormessage) return errormessage;
	if (p==nullptr)
	{
		return "No IPv4 or IPv6 adresses found";
	}

#else
	sockaddr_in _addr,*addr=&_addr;
	int addr_len=sizeof(_addr);
	hostent *he;

	errno=0;
	_addr.sin_family=AF_INET;
	_addr.sin_port=htons(port);

	/* Resolve the host name to an IP */
	he=gethostbyname(host);
	if (he==nullptr)return "Host not found";
	
	/* Store the host address */
	memcpy(&_addr.sin_addr,he->h_addr,he->h_length);
	client_socket=socket(AF_INET,SOCK_STREAM,0);

	if (client_socket==-1)return strerror(errno);
	if (connect(client_socket,(sockaddr*)addr,addr_len)==-1)
	{
		/* Connection failed */
		closesocket(client_socket);
		client_socket=0;
	#ifdef WIN32
		return "Connection refused";
	#else
		return strerror(errno);
	#endif
	}
#endif
	
	/* Make the socket non-blocking so recv() calls return
	 * immediately when no data is available.
	 * The client's socket is polled periodically.
	 */
	if (blocking == 0)
	{
#ifdef WIN32
		unsigned long block=1;
		ioctlsocket(client_socket,FIONBIO,&block);
#else
		fcntl(client_socket,F_SETFL,O_NONBLOCK);
#endif
	}

	return nullptr;
}


void CGameClient::Connect(int client_socket, int blocking)
{
	this->client_socket = client_socket;
	if (blocking == 0)
	{
#ifdef HAVE_FCNTL_H
		fcntl(client_socket,F_SETFL,O_NONBLOCK);
#else
		unsigned long block=1;
		ioctlsocket(client_socket,FIONBIO,&block);
#endif
	}
}

void CGameClient::Disconnect()
{
	if (client_socket)closesocket(client_socket);
	client_socket=0;
}

/**
 * Fetch and process all pending network messages.
 * Returns nullptr on success, otherwise a pointer to an error message indicating
 * that the connection to the server was lost
 **/
const char* CGameClient::poll()
{
	const char *err;
	static char buffer[512];
	do
	{
		/* Read one complete message from the socket into buffer */
		err=read_network_message(client_socket,(NET_HEADER*)buffer,sizeof(buffer));
		/* nullptr means a message was read successfully. Process it! */
		if (err==nullptr)process_message((NET_HEADER*)buffer);
		/* -1 means no more data is available */
		else if (err!=(char*)(-1))
		{
			/* Otherwise a read error occurred
			   and the connection is closed */
			Disconnect();
			return err;
		}
	}while (err==nullptr);
	return nullptr;
}

/**
 * Ask the game server for a local player.
 * The server sends a player number back to the client.
 **/
void CGameClient::request_player(int wish_player, const char* name)const
{
	NET_REQUEST_PLAYER data;
	data.player = wish_player;
	if (name == nullptr)
		data.name[0] = '\0';
	else
		strncpy((char*)&data.name[0], name, sizeof(data.name));
	data.name[sizeof(data.name) - 1] = '\0';
	send_message((NET_HEADER*)&data,sizeof(data),MSG_REQUEST_PLAYER);
}

void CGameClient::process_message(NET_HEADER* data)
{
	int i;
	switch(data->msg_type)
	{
		/* The server grants the client a local player */
		case MSG_GRANT_PLAYER:
			if (ntohs(data->data_length) < sizeof(NET_GRANT_PLAYER))break;
			i=((NET_GRANT_PLAYER*)data)->player;
			if (i < 0 || i >= PLAYER_MAX)break;
			player[i]=PLAYER_LOCAL;
			break;

		/* The server has set a current player */
		case MSG_CURRENT_PLAYER:
			if (ntohs(data->data_length) < sizeof(NET_CURRENT_PLAYER))break;
			/* -1 means no current player */
			if (((NET_CURRENT_PLAYER*)data)->player < -1 || ((NET_CURRENT_PLAYER*)data)->player >= PLAYER_MAX)break;
			m_current_player=((NET_CURRENT_PLAYER*)data)->player;
			newCurrentPlayer(m_current_player);
			break;

		/* Message from the server about the final placement of a stone on the board */
		case MSG_SET_STONE: {
			if (ntohs(data->data_length) < sizeof(NET_SET_STONE))break;
			NET_SET_STONE *s=(NET_SET_STONE*)data;
			if (!network_check_set_stone(s))break;
			CStone &stone=get_player(s->player)->get_stone(s->stone);
			stone.mirror_rotate_to(s->mirror_count,s->rotate_count);
			/* Place the stone on the real board */
			if ((!is_valid_turn(stone, s->player, s->y, s->x)) ||
                (!set_stone(stone, s->player, s->y, s->x)))
			{	// The game seems to be out of sync
				// REALLY bad!!
				printf("Game not in sync!\n");
				exit(1);
			}
			addHistory(s->player, stone, s->y, s->x);
			stoneWasSet(s);
			break;
		}
		case MSG_STONE_HINT: {
			if (ntohs(data->data_length) < sizeof(NET_SET_STONE))break;
			NET_SET_STONE *s=(NET_SET_STONE*)data;
			if (!network_check_set_stone(s))break;
			hintReceived(s);
			break;
		}

		/* The server has decided that the game is over */
		case MSG_GAME_FINISH:{
			gameFinished();
			break;
		}

		/* A server status packet arrived, remember its content */
		case MSG_SERVER_STATUS:
		{
			/* The oldest protocol version ends right before player_owner */
			if (ntohs(data->data_length) < offsetof(NET_SERVER_STATUS, player_owner))break;
			NET_SERVER_STATUS *s=(NET_SERVER_STATUS*)data;
			/* The field size is used for allocations and as loop bounds, never trust it blindly */
			if (!is_valid_game_config(s->gamemode, s->width, s->height))break;
			int version = 1;

			if (ntohs(data->data_length) >= 143 + 5)
				version = 2;

			if (ntohs(data->data_length) >= 143 + 7) {
				version = s->version;
				status.version_min = s->version_min;
			} else
				status.version_min = 1;

			/* Version 3 added stone_numbers at the end, do not read beyond the received data */
			if (version >= 3 && ntohs(data->data_length) < sizeof(NET_SERVER_STATUS))
				version = 2;

			status.version = version;
			status.clients=s->clients;
			status.player=s->player;
			status.computer=s->computer;
			/* If the board size differs from the server's,
			   adjust the local board size here */
			status.width=s->width;
			status.height=s->height;
			if (status.width!=get_field_size_x() || status.height!=get_field_size_y()) {
				set_field_size(status.width, status.height);
				// Turns recorded on the old board must not be replayed on the new one
				history.delete_all_turns();
				start_new_game((GAMEMODE)s->gamemode);
			}
			if (version <= 2)
			{
				bool changed=false;
				for (int i=0;i<STONE_SIZE_MAX;i++)
				{
					changed |= (status.stone_numbers_obsolete[i] != s->stone_numbers_obsolete[i]);
					status.stone_numbers_obsolete[i]=s->stone_numbers_obsolete[i];
				}
				if (changed)set_stone_numbers(status.stone_numbers_obsolete[0],status.stone_numbers_obsolete[1],status.stone_numbers_obsolete[2],status.stone_numbers_obsolete[3],status.stone_numbers_obsolete[4]);
			} else {
				bool changed = false;
				for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
				{
					changed |= (status.stone_numbers[i] != s->stone_numbers[i]);
					status.stone_numbers[i]=s->stone_numbers[i];
				}
				if (changed) set_stone_numbers(status.stone_numbers);
			}
			m_game_mode=(GAMEMODE)s->gamemode;
			if (m_game_mode == GAMEMODE_4_COLORS_2_PLAYERS)
				set_teams(0,2,1,3);
			if (m_game_mode == GAMEMODE_2_COLORS_2_PLAYERS || m_game_mode == GAMEMODE_DUO || m_game_mode == GAMEMODE_JUNIOR)
			{
				for (int n = 0 ; n < STONE_COUNT_ALL_SHAPES; n++){
					get_player(1)->get_stone(n).set_available(0);
					get_player(3)->get_stone(n).set_available(0);
				}
			}

			if (version >= 2) {
				int i;
				memcpy(status.client_names, s->client_names, sizeof(s->client_names));
				// The names come from the network and need not be terminated
				for (i = 0; i < CLIENTS_MAX; i++)
					status.client_names[i][sizeof(status.client_names[i]) - 1] = '\0';
				for (i = 0; i < PLAYER_MAX; i++) {
					status.player_owner[i] = s->player_owner[i];
				}
			}
		}
		break;

		case MSG_CHAT: {
			if (ntohs(data->data_length) < sizeof(NET_CHAT))break;
			NET_CHAT* chat=(NET_CHAT*)data;
			/* The text is a C string for the consumers, but nothing guarantees a terminator */
			((char*)data)[ntohs(data->data_length) - 1] = '\0';
			chatReceived(chat);
			break;
		}
		/* The server started a new round. Reset the game */
		case MSG_START_GAME: {
			CBoard::start_new_game(m_game_mode);
			/* Be sure to clear the history. */
			history.delete_all_turns();

			if (status.version <= 2)
				set_stone_numbers(status.stone_numbers_obsolete[0],status.stone_numbers_obsolete[1],status.stone_numbers_obsolete[2],status.stone_numbers_obsolete[3],status.stone_numbers_obsolete[4]);
			else
				set_stone_numbers(status.stone_numbers);
			if (m_game_mode == GAMEMODE_4_COLORS_2_PLAYERS)
				set_teams(0,2,1,3);
			if (m_game_mode == GAMEMODE_2_COLORS_2_PLAYERS || m_game_mode == GAMEMODE_DUO || m_game_mode == GAMEMODE_JUNIOR)
			{
				for (int n = 0 ; n < STONE_COUNT_ALL_SHAPES; n++){
					get_player(1)->get_stone(n).set_available(0);
					get_player(3)->get_stone(n).set_available(0);
				}
				refresh_player_data();
			}
			m_current_player=-1;
			gameStarted();
			break;
		}

		/* The server undoes the last turn */
		case MSG_UNDO_STONE: {
			const CTurn *t=history.get_last_turn();
			/* Nothing to undo, the server is out of sync or broken */
			if (t == nullptr)break;
			const CStone &stone=get_player(t->player)->get_stone(t->stone_number);
			stoneUndone(stone, t);
			undo_turn(history, m_game_mode);
			break;
		}
		default: printf("ERROR: unknown message received: #%d\n",data->msg_type);
			break;
	}
}

/**
 * Called by the GUI when a player wants to place a stone
 * The action is only sent to the server; the stone is NOT placed locally
 **/
TSingleField CGameClient::request_set_stone(const CStone& stone, int y, int x)
{
	NET_SET_STONE data;
	if (m_current_player==-1)return FIELD_DENIED;

	data.player=m_current_player;
	data.stone=stone.get_stone_shape();
	data.mirror_count=stone.get_mirror_counter();
	data.rotate_count=stone.get_rotate_counter();
	data.x=x;
	data.y=y;

	send_message((NET_HEADER*)&data,sizeof(data),MSG_SET_STONE);

	/* Locally set no player as active.
	   The server will send us the new active player afterwards */
	set_no_player();
	return FIELD_ALLOWED;
}

void CGameClient::request_start()const
{
	NET_START_GAME data;
	send_message((NET_HEADER*)&data,sizeof(data),MSG_START_GAME);
}

void CGameClient::request_undo()const
{
	NET_REQUEST_UNDO data;
	send_message((NET_HEADER*)&data,sizeof(data),MSG_REQUEST_UNDO);
}

/**
 * Sends a chat message to the server. It forwards it as soon as possible
 * to all clients (including this client itself).
 **/
void CGameClient::chat(const char *text)
{
	if (strlen(text)<1)return;
	/* Compute the size of the NET_CHAT message so the text fits exactly at the end */
	int len=sizeof(NET_CHAT)+sizeof(char)*strlen(text)+1;
	/* Allocate the memory properly */
	NET_CHAT *chat=(NET_CHAT*)malloc(len);
	memset(chat, 0, len);
	chat->length=strlen(text);
	strcpy((char*)&chat->text[0],text);
	send_message((NET_HEADER*)chat,len,MSG_CHAT);
	free(chat);
}

/**
 * Returns true if the player is not a computer player
 **/
bool CGameClient::is_local_player(const int player_number)const
{
	if (player_number == -1) return false;
	return (player[player_number] != PLAYER_COMPUTER);
}
