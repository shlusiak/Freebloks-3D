/**
 * spielserver.h
 * Autor: Sascha hlusiak
 *
 * Klassen zur Unterhaltung eines SpielServers, 
 * und eines Listeners, der neue Spiele errichtet
 **/

#ifndef __GAMESERVER_H_INCLUDED_
#define __GAMESERVER_H_INCLUDED_

#ifdef HAVE_SYS_SOCKET_H

#include <sys/socket.h>

#endif

#include "constants.h"
#include "game.h"
#include "network.h"
#include "timer.h"
#include "logger.h"


#define LISTEN_SOCKETS_MAX (8)

/**
 * Zentraler SpielServer, zu dem sich die SpielClients verbinden.
 * Wird vom SpielListener errichtet
 **/
class CGameServer : public CGame {
private:
    friend class CServerListener;

    CAI m_ai;

    /* Die sockets der verbundenen Clients */
    int clients[CLIENTS_MAX];

    /* seit 1.5: Namen der verbundenen Clients */
    char *names[CLIENTS_MAX];

    /* Maximale Anzahl menschlicher Spieler, der Rest ist fuer Computergegener reserviert */
    const int max_humans;

    /* Art der KI / Schwierigkeitsgrad */
    const int ai_mode;

    /* Ein Zeitzaehler, um die Dauer eines Spiels rauszukriegen */
    CTimer timer;

    /* If true, a computer turn will always take at least 800 ms */
    const bool forceDelay;

    /* Anzahl der Spielsteine bestimmter Groesse */
    int8 stone_numbers[STONE_COUNT_ALL_SHAPES];

    CLogger *logger;


    /* Entferne einen bestimmten Client aus der Liste. Bei notify=true wird ne
       Meldung ausgegeben. */
    void delete_client(int index, bool notify);

    void process_message(int client, NET_HEADER *data);

    void send_all(NET_HEADER *data, uint16 data_length, uint8 msg_type);

    void send_current_player();

    void send_server_status();

    void send_server_msg(const char *text);

    void next_player();

public:
    CGameServer(int v_max_humans, int v_ai_mode, GAMEMODE v_gamemode, bool v_forceDelay);

    virtual ~CGameServer();

    /* Verarbeite alle Netzwerknachrichten des angegebenen Clients */
    void handle_client(int index);

    /* Gibt Anzahl der verbundenen Clients (nicht Spieler) zurueck */
    int num_clients() const;

    void add_client(int s);

    /* Hauptschleife, verwaltet ein Spiel bis zum bitteren Ende */
    void run();

    /* Setzt das Spiel auf gestartet, sofern es noch nicht laeuft. */
    void start_game();

    virtual void set_stone_numbers(int8 stone_numbers[]);

    /* Logdatei setzen */
    void setLogger(CLogger *l) { logger = l; }

    /* Startet einen Server lokal in einem Thread */
    static int
    run_server(const char *interface_, int port, int maxhumans, int ai_mode, int width, int height, GAMEMODE gamemode,
               int8 stone_numbers[], int ai_threads);

    void set_ai_threads(int ai_threads) {
        m_ai.set_num_threads(ai_threads);
    }

    /* assigns all PLAYER_LOCAL player to the first connected client */
    void assign_local_players();
};


/**
 * Lauscht an einem Socket und fuegt die verbindenden Clients einem CGameServer hinzu
 **/
class CServerListener {
private:
    int listen_sockets[LISTEN_SOCKETS_MAX];
    int num_listen_sockets;

    CGameServer *server = nullptr;
    /* logger */
    CLogger *logger = nullptr;
public:
    CServerListener();

    ~CServerListener();

    /* Bereitet den Listener vor, an der angegebenen Schnittstellt und
       dem Port Verbindungen zu akzeptieren. */
    int init(const char *interface_, int port);

    /* Lauscht an allen Netzwerksockets und verarbeitet eine Nachricht */
    int wait_for_player(bool verbose, sockaddr_storage *client);

    /* Schliesst das listen_socket, sodass keine Verbindungen mehr akzeptiert werden */
    void close();

    /* Richtet einen neuen CGameServer ein, mit max. max_humans menschlichen Spielern */
    void new_game(int max_humans, int ai_mode, GAMEMODE gamemode, int ai_threads, bool forceDelay);

    CGameServer *get_game() { return server; }

    /* Logdatei setzen */
    void setLogger(CLogger *l) { logger = l; }
};


#endif

