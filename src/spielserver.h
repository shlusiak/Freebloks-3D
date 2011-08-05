/**
 * spielserver.h
 * Autor: Sascha hlusiak
 *
 * Klassen zur Unterhaltung eines SpielServers, 
 * und eines Listeners, der neue Spiele errichtet
 **/

#ifndef __SPIELSERVER_H_INCLUDED_
#define __SPIELSERVER_H_INCLUDED_

#include "spielleiter.h"
#include "network.h"
#include "timer.h"
#include "logger.h"


/* Maximale Anzahl Clients, die zu einem Spiel verbunden sein kann. 
   Es gibt auch Zuschauer! */
#define CLIENTS_MAX (8)
#define LISTEN_SOCKETS_MAX (8)

/**
 * Zentraler SpielServer, zu dem sich die SpielClients verbinden. 
 * Wird vom SpielListener errichtet
 **/
class CSpielServer:public CSpielleiter
{
private:
	friend class CServerListener;

	CKi m_ki;

	/* Die sockets der verbundenen Clients */
	int clients[CLIENTS_MAX];
	/* Maximale Anzahl menschlicher Spieler, der Rest ist fuer Computergegener reserviert */
	const int max_humans;

	/* Art der KI / Schwierigkeitsgrad */
	const int ki_mode;

	/* Ein Zeitzaehler, um die Dauer eines Spiels rauszukriegen */
	CTimer timer;

	/* Anzahl der Spielsteine bestimmter Groesse */
	int8 stone_numbers[STONE_SIZE_MAX];

	/* ggf. Logdatei */
	CLogger* logger;


	/* Entferne einen bestimmten Client aus der Liste. Bei notify=true wird ne
	   Meldung ausgegeben. */
	void delete_client(int index,bool notify);

	/* Verarbeite eine empfangene Netzwerknachricht eines Clients */
	void process_message(int client,NET_HEADER* data);

	/* Sende eine Netzwerknachricht an alle bekannten Clients */
	void send_all(NET_HEADER* data,uint16 data_length,uint8 msg_type);

	/* Verteile die aktuelle Spielernummer an alle Clients */
	void send_current_player();

	/* Schicke Server-Status an alle Clients */
	void send_server_status();

	/* Schicke eine Server-Chat-Nachricht an alle Clients */
	void send_server_msg(const char *text);

	/* Ermittle naechsten Spieler */
	void next_player();
public:
	CSpielServer(int v_max_humans,int v_ki_mode,GAMEMODE v_gamemode);
	virtual ~CSpielServer();

	/* Verarbeite alle Netzwerknachrichten des angegebenen Clients */
	void handle_client(int index);

	/* Gibt Anzahl der verbundenen Clients (nicht Spieler) zurueck */
	int num_clients()const;

	/* Fuege einen neuen Client der Liste bekannter Clients hinzu */
	void add_client(int s);

	/* Hauptschleife, verwaltet ein Spiel bis zum bitteren Ende */
	void run();

	/* Setzt das Spiel auf gestartet, sofern es noch nicht laeuft. */
	void start_game();

	/* Setzt die Anzahl der Spielsteine bestimmter Groessen */
	void set_stone_numbers(int einer,int zweier,int dreier,int vierer,int fuenfer);

	/* Logdatei setzen */
	void setLogger(CLogger* l) { logger=l; }

	/* Startet einen Server lokal in einem Thread */
	static int run_server(const char* interface_,int port,int maxhumans,int ki_mode,int width,int height,GAMEMODE gamemode,int einer,int zweier,int dreier,int vierer,int fuenfer,int ki_threads);

	void set_ki_threads(int ki_threads) { m_ki.set_num_threads(ki_threads); }
};



/**
 * Lauscht an einem Socket und fuegt die verbindenden Clients einem CSpielServer hinzu
 **/
class CServerListener
{
private:
	/* Socket, das Verbindungen akzeptieren wird */
	int listen_sockets[LISTEN_SOCKETS_MAX];
	int num_listen_sockets;

	/* Der CSpielServer, den der Listener aufbaut */
	CSpielServer* server;
	/* logger */
	CLogger* logger;
public:
	CServerListener();
	~CServerListener();
	
	/* Bereitet den Listener vor, an der angegebenen Schnittstellt und 
	   dem Port Verbindungen zu akzeptieren. */
	int init(const char* interface_,int port);

	/* Lauscht an allen Netzwerksockets und verarbeitet eine Nachricht */
	int wait_for_player(bool verbose=false);

	/* Schliesst das listen_socket, sodass keine Verbindungen mehr akzeptiert werden */
	void close();

	/* Richtet einen neuen CSpielServer ein, mit max. max_humans menschlichen Spielern */
	void new_game(int max_humans,int ki_mode,GAMEMODE gamemode,int ki_threads);

	/* Gibt aktuell erbautes Spiel zurueck */
	CSpielServer* get_game() { return server; }
	
	/* Logdatei setzen */
	void setLogger(CLogger* l) { logger=l; }
};


#endif

