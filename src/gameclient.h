/**
 * spielclient.h
 * Autor: Sascha Hlusiak
 **/

#ifndef __GAMECLIENT_H_INCLUDED_
#define __GAMECLIENT_H_INCLUDED_

#include "game.h"
#include "network.h"

/**
 * Die Klasse erweitert den Spielleiter um Funktionen eines Spielclients
 * Die GUI kommuniziert direkt mit dem Spielclient
 **/

class CGameClient:public CGame
{
private:
	/* Socket Verbindung des Clients zum Spielserver */
	int client_socket;

	/* Letztes empfangene Status Pakets des Spielservers */
	NET_SERVER_STATUS status;
public:
	CGameClient();
	virtual ~CGameClient();

	/* Spielclient mit angegebenen Server an port verbinden
	   Gibt Fehlerstring zurueck, oder nullptr bei Erfolg */
	const char* Connect(const char* host,int port, int blocking = 0);

	void Connect(int client_socket, int blocking = 0);

	void Disconnect();

	const bool isConnected()const { return client_socket!=0; }

	/* Verarbeitet alle anstehenden Netzwerknachrichten.
	 * nullptr: Erfolg
	 * sonst: Zeiger auf Zeichenkette mit Fehlermeldung
	 */
	const char* poll();

	void request_player(int wish_player, const char* name)const;

	void request_start()const;

	void request_undo()const;

	const int send_message(NET_HEADER *header,uint16 data_length,uint8 msg_type) const
	{ return network_send(client_socket,header,data_length,msg_type); }

	void process_message(NET_HEADER* data);

	/* Von GUI aufgerufen: schickt eine Nachricht zum Setzen eines Steins an den Server */
	TSingleField request_set_stone(const CStone& stone, int y, int x);

	/* Gibt Pointer auf zuletzt empfangenes Status-Paket des Servers zurueck */
	NET_SERVER_STATUS* getServerStatus() { return &status; }

	void chat(const char *text);

	const bool is_local_player()const { return is_local_player(m_current_player); }
	/* true, wenn der Spieler ein lokaler Spieler ist */
	const bool is_local_player(const int player)const;

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
