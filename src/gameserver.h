/**
 * gameserver.h
 * Author: Sascha Hlusiak
 *
 * Classes for running a game server
 * and a listener that sets up new games
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
 * Central game server that the game clients connect to.
 * Set up by CServerListener
 **/
class CGameServer : public CGame {
private:
    friend class CServerListener;

    CAI m_ai;

    /* The sockets of the connected clients */
    int clients[CLIENTS_MAX];

    /* since 1.5: names of the connected clients */
    char *names[CLIENTS_MAX];

    /* Maximum number of human players; the rest is reserved for computer opponents */
    const int max_humans;

    /* Kind of AI / difficulty */
    const int ai_mode;

    /* A timer to measure the duration of a game */
    CTimer timer;

    /* If true, a computer turn will always take at least 800 ms */
    const bool forceDelay;

    /* Number of stones of each shape */
    int8 stone_numbers[STONE_COUNT_ALL_SHAPES];

    CLogger *logger;


    /* Remove a specific client from the list. With notify=true the other
       clients are informed. reason is a trusted token for the log. */
    void delete_client(int index, bool notify, const char* reason);

    void process_message(int client, NET_HEADER *data);

    void send_all(NET_HEADER *data, uint16 data_length, uint8 msg_type);

    void send_current_player();

    void send_server_status();

    void send_server_msg(const char *text);

    void next_player();

public:
    CGameServer(int v_max_humans, int v_ai_mode, GAMEMODE v_gamemode, bool v_forceDelay);

    virtual ~CGameServer();

    /* Process the network messages of the given client */
    void handle_client(int index);

    /* Returns the number of connected clients (not players) */
    int num_clients() const;

    void add_client(int s);

    /* Main loop, runs a game to the bitter end */
    void run();

    /* Marks the game as started, unless it is already running. */
    void start_game();

    virtual void set_stone_numbers(int8 stone_numbers[]);

    /* Set the log file */
    void setLogger(CLogger *l) { logger = l; }

    /* Starts a server locally in a thread */
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
 * Listens on a socket and adds connecting clients to a CGameServer
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

    /* Prepares the listener to accept connections on the given interface and
       port. */
    int init(const char *interface_, int port);

    /* Listens on all network sockets and processes one message */
    int wait_for_player(bool verbose, sockaddr_storage *client);

    /* Closes the listen_socket so no more connections are accepted */
    void close();

    /* Sets up a new CGameServer with at most max_humans human players */
    void new_game(int max_humans, int ai_mode, GAMEMODE gamemode, int ai_threads, bool forceDelay);

    CGameServer *get_game() { return server; }

    /* Set the log file */
    void setLogger(CLogger *l) { logger = l; }
};


#endif

