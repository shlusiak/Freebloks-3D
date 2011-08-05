/**
 * client.cpp
 * Autor: Sascha Hlusiak
 *
 * Hauptprogramm eines dedizierten Freebloks Clients!!
 * Das Programm verbindet sich zu einem Server und spielt
 * fuer eine angegebene Anzahl Spieler.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "spielclient.h"
#include "turn.h"
#include "ki.h"
#include "timer.h"


/* Globale Variablen. Koennen durch Kommandozeilenparameter geaendert werden. */
static int port=TCP_PORT;
static char* server = NULL;
static int max_players=1;
static int ki_threads=2;
static int auto_start=0;
static int ki_strength=KI_HARD;



/* Hilfetext ausgeben */
static void help()
{
	printf("Usuage: client [OPTIONS] server\n\n");
	printf("  -p, --port        Specify the TCP port to accept connections. Default: %d\n",TCP_PORT);
	printf("  -n, --maxplayers  Define the maximum of players to play by client\n"
               "                    Default: %d\n",max_players);
	printf("  -k  --ki          Strength of AI. Lower number means stronger.\n"
	       "                    Default: %d\n", ki_strength);
	printf("  -s, --autostart   Start the game after connecting to server\n"
	       "                    Default: not set\n");
	printf("  -t, --threads     Define number of threads to use for calculating moves\n"
	       "                    Default: %d\n",ki_threads);
	printf("      --help        Display this help and exit\n");
	exit(0);
}

/* Kommandozeilenparameter verarbeiten */
static void parseParams(int argc,char **argv)
{
	int i=1;
	while (i<argc)
	{
		if (strcmp("--help",argv[i])==0) help();
		if (strcmp("-p",argv[i])==0 || strcmp("--port",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			port=atoi(argv[i]);
			if (port<0 || port>65535)
			{
				printf("%s: Invalid port number (%d)\n",argv[i-1],port);
				exit(1);
			}
			i++;
			continue;
		}

		if (strcmp("-n",argv[i])==0 || strcmp("--maxplayers",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			max_players=atoi(argv[i]);
			if (max_players<0 || max_players>4)
			{
				printf("%s: Invalid number (%d). Must be between 0 and 4.\n",argv[i-1],max_players);
				exit(1);
			}
			i++;
			continue;
		}

		if (strcmp("-k",argv[i])==0 || strcmp("--ki",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			ki_strength=atoi(argv[i]);
			if (ki_strength<0 || ki_strength>2000)
			{
				printf("%s: Invalid strength number (%d)\n",argv[i-1],ki_strength);
				exit(1);
			}
			i++;
			continue;
		}

		if (strcmp("-t",argv[i])==0 || strcmp("--threads",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			int t=atoi(argv[i]);
			if (t<1 || t>8)
			{
				printf("%s: Invalid (%d). Please specify number between 1 and 8.\n",argv[i-1],t);
				exit(1);
			}
			ki_threads=t;
			i++;
			continue;
		}

		if (strcmp("-s",argv[i])==0 || strcmp("--autostart",argv[i])==1)
		{
			auto_start = 1;
			i++;
			continue;
		}

		if (i == argc-1)
		{
			server = argv[i];
			i++;
			continue;
		}

		printf("Unrecognized commandline parameter: %s\n\n",argv[i]);
		help();
	};
}


class CKISpielClient:public CSpielClient
{
private:
	CTimer timer;
	CKi ki;
	virtual void gameStarted();
	virtual void newCurrentPlayer(const int player);
	virtual void chatReceived(NET_CHAT* c);
	virtual void gameFinished();

public:
	CKISpielClient(int ki_threads) {
		ki.set_num_threads(ki_threads);
	}
};


void CKISpielClient::gameStarted()
{
	int i;
	printf("Game started. Local player(s): ");
	for (i=0;i<PLAYER_MAX;i++) if (is_local_player(i)) printf("%d ", i);
	printf("\n");
	timer.reset();
}

void CKISpielClient::newCurrentPlayer(const int player)
{
	if (!is_local_player())return;

	/* Ermittle CTurn, den die KI jetzt setzen wuerde */
	CTurn *turn=ki.get_ki_turn(this, current_player(),ki_strength);
	CStone *stone;
	if (turn == NULL)
	{
		printf("Player %d: Did not find a valid move\n", player);
		return;
	}
	stone = get_current_player()->get_stone(turn->get_stone_number());
	stone->mirror_rotate_to(turn->get_mirror_count(),turn->get_rotate_count());
	set_stone(stone, turn->get_stone_number(), turn->get_y(), turn->get_x());
}

void CKISpielClient::chatReceived(NET_CHAT* c)
{
	if (c->client == -1)
		printf("  *  ");
	else printf("Client %d: ", c->client);
	printf("%s\n",c->text);
}

void CKISpielClient::gameFinished()
{
	int i;
	printf("-- Game finished! -- Took %.2f sek. --\n",timer.elapsed());
	for (i=0;i<PLAYER_MAX;i++)
	{
		CPlayer * player=get_player(i);
		printf("%c Player %d has %d stones left and %d points.\n",is_local_player(i)?'*':' ',i,get_stone_count(i),-player->get_stone_points_left());
	}

	Disconnect();
}


void runGame(CSpielClient* client)
{
	const char *s;
	int i;
	s = client->Connect(server, port, 1);
	if (s) {
		printf("\n\nCan't connect to %s: %s\n", server, s);
		return;
	} else {
		printf("Connected!\n\n");
	}

	for (i=0;i<max_players;i++) client->request_player();

	if (auto_start) client->request_start();
	do {
		s = client->poll();
		if (s)
		{
			printf("Disconnected: %s\n", s);
			return;
		}
	} while (client->isConnected());
}

int main(int argc,char ** argv)
{
	int ret;


	/* Einen ServerListener erstellen, der auf Verbindungen lauschen kann
	   und Clients connecten laesst */
	CSpielClient* client=NULL;

	/* Kommandozeilenparameter verarbeiten */
	parseParams(argc,argv);

	if (server == NULL)
	{
		printf("You need to specify at least 'server'.\n");
		help();
		return 0;
	}
#ifdef WIN32
	/* Winsock initialisieren */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,0),&wsadata);
#endif
	printf("This is the Dedicated Freebloks Client. \n");
	printf("Show help with `client --help`\n");
	printf("Connecting to %s... ", server);

	client = new CKISpielClient(ki_threads);
	runGame(client);
	delete client;

#ifdef WIN32
	/* Winsock sauber beenden */
	WSACleanup();
#endif
	return 0;
}
