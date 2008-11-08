/**
 * dedicated.cpp
 * Autor: Sascha Hlusiak
 *
 * Hauptprogramm des dedizierten Freebloks Servers!!
 * Der Server laeuft endlos durch, akzeptiert Verbindungen und laesst ggf. mehrere Spiele
 * gleichzeitig laufen.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif
#include <string.h>
#include <errno.h>
#include "spielserver.h"
#include "logger.h"



/* Globale Variablen. Koennen durch Kommandozeilenparameter geaendert werden. */
static int port=TCP_PORT;
static char* _interface = NULL;
static int max_humans=4;
static GAMEMODE gamemode=GAMEMODE_4_COLORS_4_PLAYERS;
static int width=20,height=20;
static int ki_threads=2;
static int ki_strength=KI_HARD;
static int games_ran=0;
static int games_running=0;
static int max_running_games=5;
#ifdef WIN32
static HANDLE mutex;
#else
static pthread_mutex_t mutex;
#endif
static char* logfile = NULL;
#ifndef _WIN32
	static uid_t uid = 0;
	static gid_t gid = 0;
#endif


inline void init_mutex()
{
#ifdef WIN32
	mutex=CreateMutex(NULL,FALSE,NULL);
#else
	pthread_mutex_init(&mutex,NULL);
#endif
}

inline void destroy_mutex()
{
#ifdef WIN32
	CloseHandle(mutex);
#else
	pthread_mutex_destroy(&mutex);
#endif
}

inline void lock_mutex()
{
#ifdef WIN32
	WaitForSingleObject(mutex,INFINITE);
#else
	pthread_mutex_lock(&mutex);
#endif
}

inline void unlock_mutex()
{
#ifdef WIN32
	ReleaseMutex(mutex);
#else
	pthread_mutex_unlock(&mutex);
#endif

}


/* Hilfetext ausgeben */
static void help()
{
	printf("Usuage: dedicated [OPTIONS]\n\n");
	printf("  -p, --port        Specify the TCP port to accept connections. Default: %d\n",TCP_PORT);
	printf("  -L                Listen on specified IP address. Default: OS default\n");
	printf("  -h, --maxhumans   Define the maximum of human players per game (0-4)\n"
               "                    Default: %d\n",max_humans);
	printf("  -k  --ki          Strength of AI. Lower number means stronger.\n"
	       "                    Default: %d\n", ki_strength);
	printf("  -m, --mode        The game mode for the hosted game. Valid modes:\n"
	       "                    2: 2 colors, 2 players\n"
               "                    3: 2 colors, 4 players\n"
               "                    4: 4 colors, 4 players (Default)\n"
	       "      --width       Width of the field. Default: 20\n"
	       "      --height      Height of the field. Default: 20\n"
	       "  -t, --threads     Define number of threads to use for calculating moves\n"
	       "                    Default: %d\n",ki_threads);
	printf("  -l, --limit       Maximum number of concurrent running games\n"
	       "                    Default: 5\n");
	printf("      --log         Log to file.\n");
#ifndef _WIN32
	printf("      --user\n"
	       "      --group       Drop privileges to that uid/gid\n");
#endif
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

		if (strcmp("-L",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			_interface=strdup(argv[i]);
			i++;
			continue;
		}

		if (strcmp("-h",argv[i])==0 || strcmp("--maxhumans",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			max_humans=atoi(argv[i]);
			if (max_humans<0 || max_humans>4)
			{
				printf("%s: Invalid number (%d). Must be between 0 and 4.\n",argv[i-1],max_humans);
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

		if (strcmp("-m",argv[i])==0 || strcmp("--mode",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			int mode=atoi(argv[i]);
			if (mode<2 || mode>4)
			{
				printf("%s: Invalid mode (%d). Must be one of 1, 2 or 3.\n",argv[i-1],mode);
				exit(1);
			}
			if (mode==2)gamemode=GAMEMODE_2_COLORS_2_PLAYERS;
			if (mode==3)gamemode=GAMEMODE_4_COLORS_2_PLAYERS;
			if (mode==4)gamemode=GAMEMODE_4_COLORS_4_PLAYERS;
			i++;
			continue;
		}

		if (strcmp("--width",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			int w=atoi(argv[i]);
			if (w<3 || w>30)
			{
				printf("%s: Invalid width (%d). Must be between 3 and 30.\n",argv[i-1],w);
				exit(1);
			}
			width=w;
			i++;
			continue;
		}
		if (strcmp("--height",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			int h=atoi(argv[i]);
			if (h<3 || h>30)
			{
				printf("%s: Invalid height (%d). Must be between 3 and 30.\n",argv[i-1],h);
				exit(1);
			}
			height=h;
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
		if (strcmp("-l",argv[i])==0 || strcmp("--limit",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			int l=atoi(argv[i]);
			if (l<1 || l>512)
			{
				printf("%s: Invalid (%d). Please specify number between 1 and 512.\n",argv[i-1],l);
				exit(1);
			}
			max_running_games=l;
			i++;
			continue;
		}
		if (strcmp("--log",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			logfile = strdup(argv[i]);
			i++;
			continue;
		}
#ifndef _WIN32
		if (strcmp("--user",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			int l=atoi(argv[i]);
			if (l<=0 || l>=65536)
			{
				printf("%s: Invalid (%d). uids must be between 1 and 65536.\n",argv[i-1],l);
				exit(1);
			}
			uid=l;
			i++;
			continue;
		}
		if (strcmp("--group",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			int l=atoi(argv[i]);
			if (l<=0 || l>=65536)
			{
				printf("%s: Invalid (%d). gids must be between 1 and 65536.\n",argv[i-1],l);
				exit(1);
			}
			gid=l;
			i++;
			continue;
		}
#endif

		printf("Unrecognized commandline parameter: %s\n\n",argv[i]);
		help();
	};
}




/**
 * Funktion kriegt einen CServerListener und wartet so lange, bis das Spiel gestartet wurde
 **/
static int EstablishGame(CServerListener* listener)
{
    int ret;
    do
    {
	/* Listener auf einen Client warten oder eine Netzwerknachricht verarbeiten lassen */
	ret=listener->wait_for_player(true);
	/* Bei Fehler: Raus hier */
	if (ret==-1)
	{
	        perror("wait(): ");
		return -1;
	}
	/* Solange, wie kein aktueller Spieler festgelegt ist: Spiel laeuft noch nicht */
    }while (listener->get_game()->current_player()==-1);
    return 0;
}

#ifdef WIN32
unsigned long WINAPI gameRunThread(void* param)
#else
void* gameRunThread(void* param)
#endif
{
	CSpielServer* game=(CSpielServer*)param;

	lock_mutex();
	CGameLogger logger(games_ran);
	/* Das Spiel laeuft jetzt */
	logger.logHeader(stdout);
	printf("Game started with %d clients / %d players.\n",game->num_clients(),game->num_players());
	if (logger.logfile)
	{
		logger.logTime();
		logger.logHeader();
		fprintf(logger.logfile,"Game started with %d clients / %d players. (%d games running)\n",game->num_clients(),game->num_players(),games_running);
		logger.flush();
	}
	unlock_mutex();

	game->run();

	lock_mutex();
	games_running--;
	logger.logHeader(stdout);
	printf("Game terminating.\n");
	if (logger.logfile)
	{
		logger.logTime();
		logger.logHeader();
		fprintf(logger.logfile,"Game terminating. (%d games running)\n",games_running);
		logger.flush();
	}
	unlock_mutex();
	delete game;

	return 0;
}


int main(int argc,char ** argv)
{
	int ret;

	/* Einen ServerListener erstellen, der auf Verbindungen lauschen kann
	   und Clients connecten laesst */
	CServerListener* listener=new CServerListener();

	/* Kommandozeilenparameter verarbeiten */
	parseParams(argc,argv);

#ifdef WIN32
	/* Winsock initialisieren */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,0),&wsadata);
#endif
	printf("This is the almighty Dedicated Freebloks Server. Have a nice day!\n");
	printf("Show help with `dedicated --help`\n");
	printf("Waiting for Clients for connect...\n\n");

#ifndef _WIN32
	if (gid != 0)
	{
		if (setregid(gid,gid))
			perror("setregid: ");
	}
	if (uid != 0)
	{
		if (setreuid(uid,uid))
			perror("setreuid: ");
	}
#endif

	if (logfile) 
	{
		printf("Logging to: %s\n",logfile);
		CLogger::createFile(logfile);
	}

	/* Listener fuer Akzeptieren von Verbindungen einrichten */
	ret=listener->init(_interface,port);
	if (ret!=0)
	{
	    /* Konnte nicht "listen", evtl. Port durch andere Anwendung belegt? */
	    printf("Error calling listen: %s\n",strerror(errno));
	    return -1;
	}
	init_mutex();

	/* Dedizierter Server laeuft endlos */
	while (true)
	{
		/* Ein neues Spiel soll erstellt werden.
		   KI-Stufe ist schwer*/
		listener->new_game(max_humans,ki_strength,gamemode,ki_threads);
		/* Groesse setzen */
		listener->get_game()->set_field_size_and_new(height,width);
		lock_mutex();
		listener->get_game()->setLogger(new CGameLogger(games_ran+1));
		unlock_mutex();

		/* Ein Spiel wurde erstellt, der Listener lauscht. Jetzt Spiel fuellen lassen
		   und solange warten, bis es von einem Client gestartet wurde */
		if (EstablishGame(listener)==-1)return -1;

		/* Das aktuelle Spiel in einen sekundaeren Thread auslagern */
		lock_mutex();
		games_ran++;
		games_running++;

#ifdef WIN32
		DWORD threadid;
		CloseHandle(CreateThread(NULL,0,gameRunThread,(void*)listener->get_game(),0,&threadid));
#else
		pthread_t pt;
		if (pthread_create(&pt,NULL,gameRunThread,(void*)listener->get_game()))
			perror("pthread_create");
		if (pthread_detach(pt))perror("pthread_detach");
#endif

		if (games_running>=max_running_games)
		{
			listener->close();
			printf("!! Stopping to accept new connections due to too many running games\n");
			fflush(stdout);
			if (CLogger::logfile)
			{
				CLogger::logTime();
				fprintf(CLogger::logfile,"!! Stopping accepting new connections due to too many running games\n");
				CLogger::flush();
			}
			while (games_running>=max_running_games)
			{
				unlock_mutex();
				CTimer::sleep(2000);
				lock_mutex();
			}
			printf("!! Resuming to accept new connections\n");
			if (CLogger::logfile)
			{
				CLogger::logTime();
				fprintf(CLogger::logfile,"!! Resuming accepting new connections\n");
				CLogger::flush();
			}

			ret=listener->init(_interface,port);
			if (ret!=0)
			{
				/* Konnte nicht "listen", evtl. Port durch andere Anwendung belegt? */
				printf("Error calling listen: %s\n",strerror(errno));
				return -1;
			}
		}
		unlock_mutex();
	}
	/* Aufraeumen */
	// TODO: Diese Codezeilen werden doch eh nie erreicht, aber egal.
	delete listener;
	destroy_mutex();
	CLogger::flush();
	CLogger::closeFile();

#ifdef WIN32
	/* Winsock sauber beenden */
	WSACleanup();
#endif
	return 0;
}
