/**
 * gameclient.h
 * Author: Sascha Hlusiak
 **/

#ifndef __GAMECLIENT_H_INCLUDED_
#define __GAMECLIENT_H_INCLUDED_

#include "game.h"
#include "network.h"

/**
 * The class extends the game master with the functions of a game client
 * The GUI communicates directly with the game client
 **/

class CGameClient:public CGame
{
private:
	/* Socket connection of the client to the game server */
	int client_socket;

	/* Last status packet received from the game server */
	NET_SERVER_STATUS status;
public:
	CGameClient();
	virtual ~CGameClient();

	/* Connect the game client to the given server at port
	   Returns an error string, or nullptr on success */
	const char* Connect(const char* host,int port, int blocking = 0);

	void Connect(int client_socket, int blocking = 0);

	void Disconnect();

	bool isConnected()const { return client_socket!=0; }

	/* Processes all pending network messages.
	 * nullptr: success
	 * otherwise: pointer to a string with the error message
	 */
	const char* poll();

	void request_player(int wish_player, const char* name)const;

	void request_start()const;

	void request_undo()const;

	int send_message(NET_HEADER *header,uint16 data_length,uint8 msg_type) const
	{ return network_send(client_socket,header,data_length,msg_type); }

	void process_message(NET_HEADER* data);

	/* Called by the GUI: sends a message to place a stone to the server */
	TSingleField request_set_stone(const CStone& stone, int y, int x);

	/* Returns a pointer to the last status packet received from the server */
	NET_SERVER_STATUS* getServerStatus() { return &status; }

	void chat(const char *text);

	bool is_local_player()const { return is_local_player(m_current_player); }
	/* true if the player is a local player */
	bool is_local_player(const int player)const;

private:
	virtual void newCurrentPlayer(const int player){};
	virtual void stoneWasSet(NET_SET_STONE *s){};
	virtual void hintReceived(NET_SET_STONE *s){};
	virtual void gameFinished(){};
	virtual void chatReceived(NET_CHAT* c){};
	virtual void gameStarted(){};
	virtual void stoneUndone(const CStone &s, const CTurn *t){};
};

#endif
