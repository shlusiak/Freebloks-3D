/**
 * spielclient.h
 * Autor: Sascha Hlusiak
 **/

#ifndef __SPIELCLIENT_H_INCLUDED_
#define __SPIELCLIENT_H_INCLUDED_

#include "spielleiter.h"
#include "network.h"

/**
 * Die Klasse erweitert den Spielleiter um Funktionen eines Spielclients
 * Die GUI kommuniziert direkt mit dem Spielclient
 **/

class CSpielClient:public CSpielleiter
{
private:
	/* Socket Verbindung des Clients zum Spielserver */
	int client_socket;

	/* Letztes empfangene Status Pakets des Spielservers */
	NET_SERVER_STATUS status;
public:
	CSpielClient();
	virtual ~CSpielClient();

	/* Spielclient mit angegebenen Server an port verbinden
	   Gibt Fehlerstring zurueck, oder NULL bei Erfolg */
	const char* Connect(const char* host,int port, int blocking = 0);

	/* Aktuelle Verbindung zum Spielserver trennen */
	void Disconnect();

	/* true, wenn eine Verbindung hergestellt ist */
	const bool isConnected()const { return client_socket!=0; }

	/* Verarbeitet alle anstehenden Netzwerknachrichten.
	 * NULL: Erfolg
	 * sonst: Zeiger auf Zeichenkette mit Fehlermeldung
	 */
	const char* poll();

	/* Schickt eine Anfrage fuer einen lokalen Spieler an den Server */
	void request_player()const;

	/* Schickt eine Anfrage fuer Spielstart an den Server */
	void request_start()const;

	/* Erfragt freundlich eine Zugzuruecknahme */
	void request_undo()const;

	/* Sendet eine Netzwerknachricht an den Server */
	const int send_message(NET_HEADER *header,uint16 data_length,uint8 msg_type) const
	{ return network_send(client_socket,header,data_length,msg_type); }

	/* Verarbeitet eine einzelne empfangene Nachricht */
	void process_message(NET_HEADER* data);

	/* Von GUI aufgerufen: schickt eine Nachricht zum Setzen eines Steins an den Server */
	TSingleField set_stone(CStone* stone, int stone_number, int y, int x);

	/* Gibt Pointer auf zuletzt empfangenes Status-Paket des Servers zurueck */
	NET_SERVER_STATUS* getServerStatus() { return &status; }

	/* Schickt eine Chat-Nachricht an den Server los */
	void chat(const char *text);

	/* true, wenn aktueller Spieler ein lokaler Spieler ist */
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
	virtual void stoneUndone(CStone *s, CTurn *t){};
};

#endif
