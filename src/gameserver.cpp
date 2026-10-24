/**
 * gameserver.cpp
 * Author: Sascha Hlusiak
 *
 * CGameServer: server for one game
 * CServerListener: accepts connections on a network socket and sets up a CGameServer
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define snprintf _snprintf
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#endif

#include "game.h"
#include "constants.h"
#include "network.h"
#include "gameserver.h"
#include "timer.h"


/**
 * Constructor: create an empty game server with default settings
 **/
CGameServer::CGameServer(const int v_max_humans, const int v_ai_mode, const GAMEMODE v_gamemode, bool v_forceDelay)
:ai_mode(v_ai_mode), max_humans(v_max_humans), forceDelay(v_forceDelay)
{
	int i;
	start_new_game(v_gamemode);
	for (i=0;i<CLIENTS_MAX;i++) {
		clients[i]=0;
		names[i] = nullptr;
	}
	m_game_mode=v_gamemode;
	if (m_game_mode == GAMEMODE_4_COLORS_2_PLAYERS)
		set_teams(0,2,1,3);
	for (i = 0; i < STONE_COUNT_ALL_SHAPES; i++)
		stone_numbers[i] = 1;
	set_stone_numbers(stone_numbers);
	logger=nullptr;
}

/**
 * Destructor: close the connections to all clients
 **/
CGameServer::~CGameServer()
{
	int i;
	for (i=0;i<CLIENTS_MAX;i++) {
		if (clients[i]!=0)
			closesocket(clients[i]);
		if (names[i])
			free(names[i]);
		names[i] = nullptr;
	}
}

int CGameServer::num_clients()const
{
	int n;
	n=0;
	for (int i=0;i<CLIENTS_MAX;i++)if (clients[i]!=0)n++;
	return n;
}

/**
 * Add a client to the list of known clients
 * If there is no free slot (CLIENTS_MAX clients connected),
 * the connection is simply closed
 **/
void CGameServer::add_client(int s)
{
	for (int i=0;i<CLIENTS_MAX;i++)if (clients[i]==0)
	{
		char c[50];
		/* Inform all clients about the new player. */
		snprintf(c, sizeof(c), "Client %d joined", i);
		if (logger)
		{
			logger->logLine(c);
		}
		send_server_msg(c);

		/* Found a free slot, store the client */
		clients[i]=s;
		send_server_status();
		return;
	}
	closesocket(s);
}

/**
 * Removes client[index], closes its TCP connection and replaces it with a COMPUTER
 **/

void CGameServer::delete_client(int index,bool notify)
{
	/* All players the client occupied are replaced by a PLAYER_COMPUTER */
	for (int i=0;i<PLAYER_MAX;i++)if (player[i] == clients[index]) {
			player[i]=PLAYER_COMPUTER;
	}
	if (names[index]) {
		free(names[index]);
		names[index] = nullptr;
	}
	if (closesocket(clients[index])==-1)perror("close: ");
	clients[index]=0;
	/* Send the current server status to the remaining clients */
	send_server_status();
	if (notify)
	{
		char c[256];
		snprintf(c, sizeof(c), "Client %d left\n", index);
		if (logger)
		{
			logger->logLine(c);
		}
		send_server_msg(c);
	}
}

/**
 * Main loop of the game server.
 * Called once the game is running, and runs it to the end.
 * Alternately processes the clients' network messages and computes
 * the AI's turns.
 **/
void CGameServer::run()
{
    /* filedescriptors receives all sockets, and select() then waits for a network message
       from any client */
    fd_set filedescriptors;

    timeval tv;
    int retval;
    int max;
    int heartbeat;
	CTimer timer;

    heartbeat = 0;

    do
    {
	/* If the game is running and the current player is a computer player,
	   compute the AI's turn */
	if (m_current_player!=-1 && player[m_current_player] == PLAYER_COMPUTER)
	{
		/* Determine the CTurn the AI would play now */
		timer.reset();
		const CTurn *turn = m_ai.get_ai_turn(*this, current_player(),ai_mode);

		if (forceDelay && timer.elapsed() < 800) {
			timer.sleep(800 - timer.elapsed());
		}

		if (turn != nullptr)
		{
			NET_SET_STONE data;

			data.player=current_player();
			data.stone=turn->stone_number;
			data.mirror_count=turn->mirror_count;
			data.rotate_count=turn->rotate_count;
			data.x=turn->x;
			data.y=turn->y;

			/* Actually place the turn locally; if that fails, the game is probably out of sync */
			if ((!CBoard::is_valid_turn(*turn)) ||
                (!CBoard::set_stone(*turn)))
			{
				printf("Game not in sync (2)\n");
				exit(2);
			}
			/* otherwise send the turn to all clients */
			send_all((NET_HEADER*)&data,sizeof(data),MSG_SET_STONE);
			addHistory(turn);
		}
		next_player();

		send_current_player();
	} else
		heartbeat++;

	if (heartbeat > 60) {
		/* send out a server status message. this should result in
		   timeouts, if the client disconnected and there is no data
		   flowing.

		   in worst case, heartbeats will be sent about every 30 sec */
		heartbeat = 0;
		send_server_status();
	}


	if (m_current_player!=-1 && player[current_player()] == PLAYER_COMPUTER)
	{
		/* If a computer is to move now, do not wait (timeout 0) */
		tv.tv_sec=0;
		tv.tv_usec=0;
	}else{
		/* For human players use a timeout of 0.5 s */
		tv.tv_sec = 0;
		tv.tv_usec = 500*1000;
	}

	do
	{
		FD_ZERO(&filedescriptors);
		for (int i=max=0;i<CLIENTS_MAX;i++)if (clients[i]!=0)
		{
			FD_SET((unsigned int)clients[i],&filedescriptors);
			if (max<clients[i])max=clients[i];
		}
		/* select blocks until the timeout expires or data is available on one of the sockets */

		retval = select(max+1, &filedescriptors, nullptr, nullptr, &tv);
		if (retval>0)
		{
			/* Data is available on a socket; process all messages of all affected clients */
			for (int i=0;i<CLIENTS_MAX;i++)
				if (clients[i]!=0 && FD_ISSET(clients[i],&filedescriptors))
			{
				/* Process the specific client i */
	 			handle_client(i);
			}
		}
	}while (retval>0);

    /* The server keeps looping while at least one client is connected.
       Once all clients have disconnected, the server ends here. */
    }while (num_clients()>0);
}

/**
 * Processes a single message of a specific client that has data pending
 */
void CGameServer::handle_client(int index)
{
	const char *err;
	char buffer[512];
	/* Read exactly one network message of the client into buffer. */
	err=read_network_message(clients[index],(NET_HEADER*)buffer,sizeof(buffer));
	/* nullptr means success, otherwise an error occurred. */
	if (err==nullptr)process_message(index,(NET_HEADER*)buffer);
	/* -1 would mean no data is available, which must not happen here
	   (since select() says data is pending) */
	else {
		delete_client(index,true);
		return;
	}
}

void CGameServer::process_message(int client,NET_HEADER* data)
{
	switch(data->msg_type)
	{
		case MSG_REQUEST_PLAYER: {
			if (ntohs(data->data_length) < sizeof(NET_REQUEST_PLAYER))
				return;

			NET_REQUEST_PLAYER *req = (NET_REQUEST_PLAYER*)(data);
			NET_GRANT_PLAYER msg;
			int n;
			/* If no more humans are allowed, discard the message */
			if (num_players()>=max_humans)return;

			/* Likewise if the game is already running */
			if (m_current_player!=-1) return;

			if (m_game_mode == GAMEMODE_2_COLORS_2_PLAYERS || m_game_mode == GAMEMODE_DUO || m_game_mode == GAMEMODE_JUNIOR)
			{
				/* If two players are already in, bail out */
				if (num_players()>=2)return;
				/* Pick a random player */
				n=(rand()%2)*2;
				/* since 1.5: a preferred player may be given */
				if (ntohs(data->data_length) > sizeof(NET_HEADER)) {
					if (req->player == 0 || req->player == 2)
						n = req->player;
				}
				/* Find the next one that is free (!=PLAYER_COMPUTER) */
				if (player[n] != PLAYER_COMPUTER)
					n = 2 - n;
				if (player[n] != PLAYER_COMPUTER)
					return;
			}else{
				/* If all players are taken, bail out */
				if (num_players()>=PLAYER_MAX)return;
				/* Pick a random player */
				n=rand()%PLAYER_MAX;
				/* since 1.5: a preferred player may be given */
				if (ntohs(data->data_length) > sizeof(NET_HEADER)) {
					if (req->player >= 0 && req->player < PLAYER_MAX)
						n = req->player;
				}
				/* Find the next one that is free (!=PLAYER_COMPUTER) */
				while (player[n] != PLAYER_COMPUTER)n= (n + 1) % PLAYER_MAX;
			}

			/* Send back a message granting it the player */
			msg.player=n;
			network_send(clients[client],(NET_HEADER*)&msg,sizeof(msg),MSG_GRANT_PLAYER);
			/* Store the player's socket in the player[] array
			   This is how players are mapped back to clients */
			player[n]=clients[client];

			if (m_game_mode == GAMEMODE_4_COLORS_2_PLAYERS)
			{
				/* Every player gets 2 colours, so give it its second colour too */
				n=(n+2)%PLAYER_MAX;
				if (player[n] != PLAYER_COMPUTER)
				{
					printf("ERROR: player already assigned (2 colors per player)\n");
					delete_client(client,false);
					return;
				}
				msg.player=n;
				network_send(clients[client],(NET_HEADER*)&msg,sizeof(msg),MSG_GRANT_PLAYER);
				/* Store the player's socket in the player[] array
			   	This is how players are mapped back to clients */
				player[n]=clients[client];
			}


			if (ntohs(data->data_length) > sizeof(NET_HEADER)) {
				req->name[sizeof(req->name) - 1] = '\0';
				if (strlen((char*)req->name) > 0) {
					// TODO: Allow setting name per player, not per client

					/* store client name */
					if (names[client] != nullptr)
						free(names[client]);

					names[client] = strdup((char*)req->name);
				}
			}

			/* Send the current server status to the clients */
			send_server_status();
// 			printf("Client %d requested player (#%d)\n",client,n);
			break;
		}

		case MSG_REVOKE_PLAYER: {
			if (ntohs(data->data_length) < sizeof(NET_REVOKE_PLAYER))
				return;

			/* client requests to revoke an assigned player */
			NET_REVOKE_PLAYER *rev = (NET_REVOKE_PLAYER*)(data);
			if (m_current_player > -1)
				break;
			if (rev->player < 0)
				break;
			if (rev->player >= PLAYER_MAX)
				break;

			if (player[rev->player] == clients[client]) {
				player[rev->player]=PLAYER_COMPUTER;

				network_send(clients[client],(NET_HEADER*)rev,sizeof(NET_REVOKE_PLAYER),MSG_REVOKE_PLAYER);

				if (m_game_mode == GAMEMODE_4_COLORS_2_PLAYERS)
				{
					int n = rev->player;
					n=(n+2)%PLAYER_MAX;
					player[n]=PLAYER_COMPUTER;
					rev->player = n;
					network_send(clients[client],(NET_HEADER*)rev,sizeof(NET_REVOKE_PLAYER),MSG_REVOKE_PLAYER);
				}

				send_server_status();
			}

			break;
		}

		case MSG_SET_STONE:{
			if (ntohs(data->data_length) < sizeof(NET_SET_STONE))
				return;

			NET_SET_STONE *s=(NET_SET_STONE*)data;
			/* Without a running game there is no current player, never index player[] with -1 */
			if (m_current_player < 0) {
				if (logger)
					logger->logLine("WARNING: Ignoring move, no game running.\n");
				return;
			}
			/* Assemble the corresponding stone from the data */
			if (s->player != m_current_player) {
				if (logger)
					logger->logLine("WARNING: Ignoring move from non-current player.\n");
				send_current_player();
				return;
			}
			if (clients[client] != player[s->player]) {
				if (logger)
					logger->logLine("WARNING: Client does not own player! Ignoring move.\n");
				send_current_player();
				return;
			}

			if (!network_check_set_stone(s)) {
				if (logger)
					logger->logLine("WARNING: Ignoring move with invalid stone or orientation.\n");
				send_current_player();
				return;
			}

			CStone &stone = get_player(s->player)->get_stone(s->stone);
			if (stone.get_available() <= 0) {
				if (logger)
					logger->logLine("WARNING: Ignoring move with a stone that is not available.\n");
				send_current_player();
				return;
			}
			stone.mirror_rotate_to(s->mirror_count,s->rotate_count);

 			if ((is_valid_turn(stone, s->player, s->y, s->x)) &&
                (set_stone(stone, s->player, s->y, s->x)))
			{
				/* On success the message is sent straight back to all
				   clients */
				send_all(data,ntohs(data->data_length),MSG_SET_STONE);
				addHistory(s->player, stone, s->y, s->x);
				next_player();
			}else{ // The game seems to be out of sync
				if (logger) {
					logger->logLine("WARNING: Game seems to be out of sync, ignoring move.\n");
//					delete_client(client, true);
				}
			}

			send_current_player();
			break;
		}

		case MSG_START_GAME:
// 			printf("Request game start\n");
			start_game();
			break;

		// Received a chat message from a client
		case MSG_CHAT:
			if (ntohs(data->data_length) < sizeof(NET_CHAT))
				break;

			// Record sending client who sent the message
			((NET_CHAT*)data)->client = client;

			// Add terminating zero to the end of the buffer, just in case
			((NET_CHAT*)data)->text[ntohs(data->data_length) - sizeof(NET_CHAT)]='\0';

			// Now broadcast that modified message to all clients
			send_all(data,ntohs(data->data_length),MSG_CHAT);

			if (logger)
			{
				logger->logLine("Client %d: %s\n",client,((NET_CHAT*)data)->text);
			}
			break;

		case MSG_REQUEST_UNDO: {
			/* Undo is only allowed with a single client or a single human
			   player. */
			if (num_clients()>1 && num_players()>1)
			{
// 				printf("Client %d requested undo. IGNORED.\n",client);
				return;
			}
// 			printf("Client %d requesting undo. ",client);
			NET_UNDO_STONE undo{};
			int i=0;

			/* Keep taking back stones until there are none left in the history,
			   or a human player is to move again. */
			do
			{
				const CTurn *turn = history.get_last_turn();
				if (turn==nullptr) break;
				i++;
				send_all((NET_HEADER*)(&undo),sizeof(undo),MSG_UNDO_STONE);
				// The player of the undone stone is to move again
				m_current_player=turn->player;
				undo_turn(history, m_game_mode);
				// While computer turns are being undone

				timer.sleep(100);
			}while (player[m_current_player] == PLAYER_COMPUTER);

			send_current_player();

// 			printf("Removed %d turns.\n",i);
			break;
		}

		case MSG_REQUEST_HINT: {
			if (ntohs(data->data_length) < sizeof(NET_REQUEST_HINT))
				return;

			int hint_player = ((NET_REQUEST_HINT*)data)->player;
			/* Only hints for valid players owned by the requesting client */
			if (hint_player < 0 || hint_player >= PLAYER_MAX)
				return;
			if (player[hint_player] != clients[client])
				return;

			const CTurn *turn = m_ai.get_ai_turn(*this, hint_player, AI_HARD);
			if (turn == nullptr)
				return;

			NET_SET_STONE d;

			d.player=hint_player;
			d.stone=turn->stone_number;
			d.mirror_count=turn->mirror_count;
			d.rotate_count=turn->rotate_count;
			d.x=turn->x;
			d.y=turn->y;

			network_send(clients[client],(NET_HEADER*)&d,sizeof(d),MSG_STONE_HINT);

			break;
		}

		case MSG_REQUEST_GAME_MODE: {
			if (ntohs(data->data_length) < sizeof(NET_REQUEST_GAME_MODE))
				return;

			NET_REQUEST_GAME_MODE* r = (NET_REQUEST_GAME_MODE*)data;
			if (m_current_player > -1) {
				send_server_status();
				break;
			}
			if (r->version >= 1) {
				if (ntohs(r->header.data_length) < sizeof(NET_HEADER) + 25)
					break;
				if (!is_valid_game_config(r->gamemode, r->width, r->height))
					break;

				set_field_size(r->width, r->height);
				m_game_mode = (GAMEMODE)r->gamemode;

				if (m_game_mode == GAMEMODE_2_COLORS_2_PLAYERS || m_game_mode == GAMEMODE_DUO || m_game_mode == GAMEMODE_JUNIOR) {
					NET_REVOKE_PLAYER rev;

					for (int i = 1; i <= 3; i += 2) if (player[i] != PLAYER_COMPUTER) {
						rev.player = i;
						network_send(player[i], (NET_HEADER*)&rev, sizeof(NET_REVOKE_PLAYER), MSG_REVOKE_PLAYER);

						// searching for free slot and grant player
						for (int j = 0; j <= 2; j += 2) if (player[j] == PLAYER_COMPUTER) {
								player[j] = player[i];
							NET_GRANT_PLAYER g;
							g.player = j;
							network_send(player[j], (NET_HEADER*)&g, sizeof(NET_GRANT_PLAYER), MSG_GRANT_PLAYER);
							break;
						}

							player[i]=PLAYER_COMPUTER;
					}
				}

				if (m_game_mode == GAMEMODE_4_COLORS_2_PLAYERS) {
					for (int i = 0; i <= 3; i ++) if (player[i] != PLAYER_COMPUTER) {
						int j = (i+2)%PLAYER_MAX;
						if (player[i] == player[j])
							continue;

						if (player[j] == PLAYER_COMPUTER) {
							// if next one is free, grant
							player[j] = player[i];
							NET_GRANT_PLAYER g;
							g.player = j;
							network_send(player[i], (NET_HEADER*)&g, sizeof(NET_GRANT_PLAYER), MSG_GRANT_PLAYER);
						} else {
							// otherwise revoke
							NET_REVOKE_PLAYER rev;
							rev.player = i;
							network_send(player[i], (NET_HEADER*)&rev, sizeof(NET_REVOKE_PLAYER), MSG_REVOKE_PLAYER);
							player[i] = PLAYER_COMPUTER;
						}
					}
				}

				// Turns recorded on the old board must not be replayed on the new one
				history.delete_all_turns();
				start_new_game((GAMEMODE)r->gamemode);
				set_stone_numbers(r->stone_numbers);
			}
			send_server_status();
			break;
		}

		default: printf("ERROR: unhandled network message: #%d\n",data->msg_type);
			break;
	}
}

void CGameServer::send_all(NET_HEADER* data,uint16 data_length,uint8 msg_type)
{
	for (int i=0;i<CLIENTS_MAX;i++)if (clients[i]!=0)
		if (network_send(clients[i],data,data_length,msg_type)==-1)
	{
		/* Error while sending, disconnect the client. */
		delete_client(i,true);
	}
}

void CGameServer::send_current_player()
{
	NET_CURRENT_PLAYER data;
	data.player=current_player();
	send_all((NET_HEADER*)&data,sizeof(data),MSG_CURRENT_PLAYER);
}

/**
 * Send the server status to all clients
 * (number of players, number of computer players, number of connected clients, width and height)
 **/
void CGameServer::send_server_status()
{
	NET_SERVER_STATUS status;
	int i;
	// Do not send uninitialised stack bytes to the clients
	memset(&status, 0, sizeof(status));
	for (i = 0; i < PLAYER_MAX; i++)
		status.player_owner[i] = PLAYER_COMPUTER;
	int max= (m_game_mode == GAMEMODE_2_COLORS_2_PLAYERS || m_game_mode == GAMEMODE_DUO || m_game_mode == GAMEMODE_JUNIOR) ? 2 : PLAYER_MAX;
	status.player=num_players();
	status.computer=max-num_players();
	status.clients=num_clients();
	status.width=get_field_size_x();
	status.height=get_field_size_y();
	for (i=0;i<STONE_SIZE_MAX;i++)
		status.stone_numbers_obsolete[i]=1; // OBSOLETE, NOT SUPPORTED ANYMORE
	status.gamemode=m_game_mode;
	for (i = 0; i < PLAYER_MAX; i++) {
		if (player[i] == PLAYER_COMPUTER || player[i] == PLAYER_LOCAL)
			status.player_owner[i] = player[i];
		else {
			int j;
			for (j = 0; j < CLIENTS_MAX; j++)
				if (clients[j] == player[i])
					status.player_owner[i] = j;
		}
	}
	for (i = 0; i < CLIENTS_MAX; i++) {
		status.client_names[i][0] = '\0';
		if (clients[i] != 0) {
			if (names[i])
				strcpy((char*)status.client_names[i], names[i]);
		}
	}
	status.version = NET_SERVER_STATUS_VERSION;
	status.version_min = 1;
	for (i = 0; i < STONE_COUNT_ALL_SHAPES; i++)
		status.stone_numbers[i] = stone_numbers[i];

	send_all((NET_HEADER*)&status,sizeof(status),MSG_SERVER_STATUS);
}

/**
 * Sends a chat message to all clients, with the server as sender
 **/
void CGameServer::send_server_msg(const char *text)
{
	NET_CHAT *chat=(NET_CHAT*)malloc(sizeof(NET_CHAT)+strlen(text)+1);
	memset(chat, 0, sizeof(NET_CHAT)+strlen(text)+1);
	/* The sender is -1, the server */
	chat->client=-1;
	strcpy((char*)(&chat->text[0]),text);
	chat->length=strlen(text);
	send_all((NET_HEADER*)chat,sizeof(NET_CHAT)+strlen(text)+1,MSG_CHAT);
	free(chat);
}

void CGameServer::start_game()
{
	if (m_current_player!=-1)return;
// 	printf("Starting game\n");

	history.delete_all_turns();

	CBoard::start_new_game(m_game_mode);
	CBoard::set_stone_numbers(stone_numbers);

	/* When playing with only two colours, take all stones away from players 1 and 3 */
	if (m_game_mode == GAMEMODE_2_COLORS_2_PLAYERS || m_game_mode == GAMEMODE_DUO || m_game_mode == GAMEMODE_JUNIOR)
	{
		for (int n = 0 ; n < STONE_COUNT_ALL_SHAPES; n++){
			get_player(1)->get_stone(n).set_available(0);
			get_player(3)->get_stone(n).set_available(0);
		}
		refresh_player_data();
	}

	/* The starting player is always player 0 (= blue) */
	m_current_player=0;
	/* Send the server status, the game start notice and the current player */
	send_server_status();

	NET_START_GAME data{};
	send_all((NET_HEADER*)&data,sizeof(data),MSG_START_GAME);
	send_current_player();

	timer.reset();
}

void CGameServer::next_player()
{
	int i;
	for (i=0;i<PLAYER_MAX;i++)
	{
		m_current_player=(m_current_player+1)%PLAYER_MAX;
		/* If the next player in turn still has at least one free move,
		   it is their turn. Otherwise they pass and the one after is tried. */
		if (get_number_of_possible_turns(m_current_player)>0)
		{
// 			printf("Player %d to move: has %d possible turns.\n",m_current_player,get_number_of_possible_turns(m_current_player));
			return;
		}//else printf("Player %d has to pass.\n",m_current_player);
	}
	/* Reaching this point, none of the PLAYER_MAX players has a free move left.
	   The game is over. */
	m_current_player=-1;

	/* Send the clients an MSG_GAME_FINISH message announcing the end of the game. */
	NET_GAME_FINISH data{};
	send_all((NET_HEADER*)&data,sizeof(data),MSG_GAME_FINISH);

	/* Print status messages to the console (e.g. for the dedicated server) */
	if (logger){
		logger->logLine("-- Game finished! -- Took %.2f sek. --\n",timer.elapsed());
	}
	for (i=0;i<PLAYER_MAX;i++)
	{
		CPlayer * player=get_player(i);
		if (logger) {
			logger->logLine("Player %d has %d stones left and %d points.\n",i,get_stone_count(i),-player->get_stone_points_left());
		}
	}
}

/**
 * Sets the number of stones of the game. The numbers are kept locally
 * so they can be sent to the clients.
 **/
void CGameServer::set_stone_numbers(int8 stone_numbers[])
{
	if (stone_numbers == nullptr) {
		for (int i = 0; i < STONE_COUNT_ALL_SHAPES; i++)
			this->stone_numbers[i] = 1;
	} else {
		for (int i = 0; i < STONE_COUNT_ALL_SHAPES; i++)
			this->stone_numbers[i] = stone_numbers[i] < 0 ? 0 : stone_numbers[i];
	}
	/* CBoard sets up the players correctly here */
	CBoard::set_stone_numbers(this->stone_numbers);
}


/**
 * The server thread that hosts the game in parallel
 * Called by CGameServer::run_server(...)
 **/
#ifdef WIN32
DWORD WINAPI LocalServerThread(void* param)
#else
void* LocalServerThread(void* param)
#endif
{
	CServerListener* listener=(CServerListener*)param;
	/* The thread should die if a player connected and disconnected again
	   without the game being started */
	bool hadClient=false;
#ifdef WIN32
	srand((unsigned int)time(nullptr));
#endif
	do
	{
		sockaddr_storage client;
		listener->wait_for_player(false, &client);
		// Remember whether a client was ever connected
		if (listener->get_game()->num_clients()>=1)hadClient=true;
		// Die when the last client has disconnected
		if (listener->get_game()->num_clients()==0 && hadClient)
			break;
		// Keep collecting players as long as the game has not been started
	} while(listener->get_game()->current_player()==-1);
	/* The game has started, so accept no more connections. */
	listener->close();
	// Only run the game if any client is connected at all
	if (listener->get_game()->num_clients()>=1)listener->get_game()->run();
	delete listener;
// 	printf("Local server terminating.\n");
	return 0;
}

/**
 * Starts a listener, spins it off into a second thread and returns
 * Returns 0 on success
 * Called by the GUI to start a local server, for single or multiplayer
 **/
int CGameServer::run_server(const char* interface_,int port,int maxhumans,int ai_mode,int width,int height,GAMEMODE gamemode,int8 stone_numbers[],int ai_threads)
{
	if (!is_valid_game_config(gamemode, width, height))
		return EINVAL;

	CServerListener *listener=new CServerListener();
	int r=listener->init(interface_,port);
	if (r!=0)
	{
	    delete listener;
	    return errno;
	}

	listener->new_game(maxhumans, ai_mode, gamemode, ai_threads, true);
	listener->get_game()->set_field_size(width, height);
	listener->get_game()->start_new_game(gamemode);
	listener->get_game()->set_stone_numbers(stone_numbers);

#ifdef WIN32
	DWORD id;
	CloseHandle(CreateThread(nullptr,0,LocalServerThread,(void*)listener,0,&id));
#else
	pthread_t thread;
	pthread_create(&thread,nullptr,LocalServerThread,(void*)listener);
#endif
	// Success
	return 0;
}

/**
 * Assigns all players that have been marked as PLAYER_LOCAL to the first connected client.
 * This is for the server, which does not have local players to support "resume", where local players
 * can only be assigned to clients after they have joined.
 */
void CGameServer::assign_local_players()
{
	int s = -1;
	int i;
	for (i = 0; i < CLIENTS_MAX; i++) {
		if (clients[i] != -1) {
			s = clients[i];
			break;
		}
	}
	if (s == -1)
		return;

	for (i = 0; i < PLAYER_MAX; i++) {
		if (player[i] == PLAYER_LOCAL) {
			setPlayer(i, s);
		}
	}
}




/**
 * Create an empty CServerListener, without socket and without CGameServer
 **/
CServerListener::CServerListener()
{
	int i;
	for (i=0;i<LISTEN_SOCKETS_MAX; i++)
		listen_sockets[i]=0;
	num_listen_sockets = 0;
	logger = nullptr;
	server = nullptr;
}

CServerListener::~CServerListener()
{
	close();
	delete server;
}

/**
 * Close the socket listening for incoming connections
 **/
void CServerListener::close()
{
	int i;
	for (i=0;i<LISTEN_SOCKETS_MAX;i++)
	{
		if (listen_sockets[i])closesocket(listen_sockets[i]);
		listen_sockets[i]=0;
	}
}

/**
 * Creates a socket, binds it to interface interface_ and port port,
 * and sets it up for incoming connections
 * If interface_==nullptr, listen on all interfaces.
 * Returns 0 on success, otherwise an error code (errno)
 **/
int CServerListener::init(const char* interface_,int port)
{
	errno=0;

#ifndef WIN32
	if (port == 0)
	{
		// Try abstract unix domain socket

		int listen_socket;
		int i;
		struct sockaddr_un my_addr;
		errno=0;

		num_listen_sockets = 0;
		listen_socket=socket(PF_UNIX,SOCK_STREAM,0);
		if (listen_socket <= 0)
			return errno;

		my_addr.sun_family=AF_UNIX;
		memset(my_addr.sun_path, 0, sizeof(my_addr.sun_path));
		// This moves it into the abstract namespace, copying interface_ after the null byte
		int addr_len = 0;
		if (interface_ != nullptr) {
			/* One byte is taken by the leading null, and the address length must fit as well */
			if (strlen(interface_) + 1 > sizeof(my_addr.sun_path)) {
				closesocket(listen_socket);
				return ENAMETOOLONG;
			}
			strcpy(&my_addr.sun_path[1], interface_);
			addr_len = strlen(interface_);
		}
		my_addr.sun_path[0] = '\0';

		i = 1;
		if (setsockopt(listen_socket,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i))==-1)
		{
			// nop
		}
		if (errno) return errno;

		if (bind(listen_socket,(struct sockaddr*)&my_addr,sizeof(my_addr.sun_family) + addr_len + 1)!=0)
		{
			closesocket(listen_socket);
			if (errno) return errno;
		}
		if (listen(listen_socket,5)==0)
		{
			listen_sockets[num_listen_sockets++] = listen_socket;
		} else {
			closesocket(listen_socket);
		}
		if (num_listen_sockets == 0)
		{
			if (errno) return errno;
			return -1;
		}
		/* Success */

		return 0;
	}
#endif


#if (defined HAVE_GETADDRINFO) || (defined WIN32)
	addrinfo hints,*res,*ressave;
	char s_port[32];

	snprintf(s_port,sizeof(s_port), "%d",port);

	memset(&hints,0,sizeof(hints));
	hints.ai_flags=AI_PASSIVE;
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;

	// getaddrinfo reports errors as non-zero EAI_* codes, whose sign differs between platforms
	if (getaddrinfo(interface_,s_port,&hints,&res)!=0)
		return errno?errno:-1;

	num_listen_sockets = 0;
	ressave=res;
	while (res)
	{
		int listen_socket;
		listen_socket=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
		if (listen_socket>=0)
		{
#ifndef WIN32
			int i;
			/* On Linux use SO_REUSEADDR after all, so bind succeeds even if a socket
			  with the port already exists. */
			i = 1;
			if (setsockopt(listen_socket,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i))==-1)
			{}

			if (res->ai_family == AF_INET6) {
				i = 1;
				if (setsockopt(listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, &i, sizeof(i)) < 0) {
					perror("setsockopt");
				}
			}

#endif
			if (bind(listen_socket,res->ai_addr,res->ai_addrlen)==0)
			{
				if (listen(listen_socket,5)==0 && num_listen_sockets < LISTEN_SOCKETS_MAX)
				{
					listen_sockets[num_listen_sockets++] = listen_socket;
				} else {
					closesocket(listen_socket);
				}
			} else {
				closesocket(listen_socket);
			}
		}
		res=res->ai_next;
	}
	if (ressave)
		freeaddrinfo(ressave);
	if (num_listen_sockets == 0)
	{
		if (errno) return errno;
		return -1;
	}

#else

	sockaddr_in addr;

	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);

	/* Listen either on one IP address or on all */
	if (interface_)addr.sin_addr.s_addr=inet_addr(interface_);
		else addr.sin_addr.s_addr=0;

	num_listen_sockets = 0;
	listen_sockets[0]=socket(AF_INET,SOCK_STREAM,0);
	if (listen_sockets[0]==-1)return errno;
	num_listen_sockets = 1;

#ifndef WIN32	
	/* On Linux use SO_REUSEADDR after all, so bind succeeds even if a socket
	   with the port already exists. */
	if (setsockopt(listen_sockets[0],SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i))==-1)return errno;
#endif

	/* Bind the socket to the source address */
	if (bind(listen_socket[0],(sockaddr*)&addr,sizeof(addr))==-1)return errno;
	if (listen(listen_sockets[0],5)==-1)return errno;
#endif

	/* Success */
	return 0;
}

/**
 * Processes exactly one network event: either data from a client already connected
 * to the server, or an incoming connection.
 **/
int CServerListener::wait_for_player(bool verbose, sockaddr_storage *client)
{
	/* Address of the client that wants to connect */

	/* File descriptors of the sockets; one of them is the listen_socket */
	fd_set filedescriptors;
	int retval;
	int max;
	int i,j;

#ifdef WIN32
	int l;
#else
	socklen_t l;
#endif
	l=sizeof(*client);

	/* Fill filedescriptors with the sockets of the connected clients and the listen_socket */
	FD_ZERO(&filedescriptors);
	max=listen_sockets[0];
	for (i=0;i<num_listen_sockets;i++)
	{
		FD_SET((unsigned int)listen_sockets[i],&filedescriptors);
		if (max<listen_sockets[i])max=listen_sockets[i];
	}
	for (i=0;i<CLIENTS_MAX;i++)if (get_game()->clients[i]!=0)
	{
		FD_SET((unsigned int)get_game()->clients[i],&filedescriptors);
		if (max<get_game()->clients[i])max=get_game()->clients[i];
	}

	/* Block indefinitely until data is available on a socket */
	retval = select(max+1, &filedescriptors, nullptr, nullptr, nullptr);

	if (retval>0)
	{
		/* Go through the clients to see whether data is pending. */
		for (i=0;i<CLIENTS_MAX;i++)
			if (get_game()->clients[i]!=0 && FD_ISSET(get_game()->clients[i],&filedescriptors))
		{
			get_game()->handle_client(i);
		}

		/* If data is pending on the listen_socket, someone wants to connect */
		for (j=0;j<num_listen_sockets;j++)
			if (FD_ISSET(listen_sockets[j],&filedescriptors))
		{
			/* Accept the connection; cl is the new client socket */
			l=sizeof(*client);
			int cl=accept(listen_sockets[j],(sockaddr*)client,&l);
			if (cl==-1)
			{
				int err = errno;
				perror("accept: ");
				/* Transient failures (aborted connection, out of file descriptors, ...) must
				   not take down the whole server. Back off briefly, as the pending connection
				   keeps the listen socket readable. */
				if (err == EINTR || err == EAGAIN || err == ECONNABORTED || err == EMFILE ||
				    err == ENFILE || err == ENOBUFS || err == ENOMEM)
				{
					CTimer::sleep(100);
					continue;
				}
				return -1;
			}

			if (verbose)
			{
#if (defined HAVE_GETADDRINFO) || (defined WIN32)
				char clienthost[NI_MAXHOST];
				clienthost[0]='\0';

				if (logger) {
					logger->logTime();
					logger->logHeader();
					logger->log("Connection from: ");
				}


				/* First resolve the FQDN */
				retval = getnameinfo((sockaddr*)client,l,
					clienthost,sizeof(clienthost),
					nullptr,0,
					NI_NAMEREQD);

				if (retval == 0 && logger)
					logger->log("%s, ",clienthost);

				/* Then resolve the IP */
				getnameinfo((sockaddr*)client,l,
					clienthost,sizeof(clienthost),
					nullptr,0,
					NI_NUMERICHOST);

				if (logger)
					logger->log("%s\n",clienthost);
#else
				hostent *host;
				host=gethostbyaddr((char*)client->sin_addr.s_addr,sizeof(client->sin_addr.s_addr),AF_INET);
				unsigned int a,b,c,d,i;
				i=ntohl(client->sin_addr.s_addr);
				a=(i>>24)&0xFF;
				b=(i>>16)&0xFF;
				c=(i>>8)&0xFF;
				d=(i>>0)&0xFF;
				printf("Connection from: ");
				if (host)printf("%s, ",host->h_name);
				printf("%d.%d.%d.%d\n",a,b,c,d);
				if (CLogger::logfile)
				{
					fprintf(CLogger::logfile,"Connection from: ");
					if (host)fprintf(CLogger::logfile,"%s, ",host->h_name);
					fprintf(CLogger::logfile,"%d.%d.%d.%d\n",a,b,c,d);
					CLogger::flush();
				}
#endif
			}

#ifndef WIN32
			/* select() cannot handle descriptors beyond FD_SETSIZE, FD_SET would write out of bounds */
			if (cl >= FD_SETSIZE)
			{
				if (logger)
					logger->logLine("WARNING: Too many open file descriptors, rejecting connection.");
				closesocket(cl);
				continue;
			}
#endif

			get_game()->add_client(cl);
		}
	}

	/* Success */
	return 0;
}

/**
 * Creates a new game with max_humans human players; the rest is reserved
 * for computers
 **/
void CServerListener::new_game(int max_humans, int ai_mode, GAMEMODE gamemode, int ai_threads, bool forceDelay)
{
	server = new CGameServer(max_humans, ai_mode, gamemode, forceDelay);
	server->set_ai_threads(ai_threads);
}

