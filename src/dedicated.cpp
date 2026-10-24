/**
 * dedicated.cpp
 * Author: Sascha Hlusiak
 *
 * Standalone dedicated server that will endlessly run games and listen on a socket.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef WIN32
	#include <winsock2.h>
#else
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/un.h>
#endif

#include <string.h>
#include <errno.h>
#include <signal.h>
#include "gameserver.h"
#include "logger.h"



static int port = TCP_PORT;
static const char* _interface = nullptr;
static int max_humans = 4;
static GAMEMODE gamemode = GAMEMODE_4_COLORS_4_PLAYERS;
static int width=20,height=20;
static int ai_threads=2;
static int ai_strength=AI_HARD;
static int games_ran=0;
static int games_running=0;
static int max_running_games=5;
static bool force_delay=true;
#ifdef WIN32
static HANDLE mutex;
#else
static pthread_mutex_t mutex;
#endif
static char* logfile = nullptr;
/* Never destroyed on purpose: detached game threads may still log while main() returns */
static CStdoutWriter& logWriter = *new CStdoutWriter();
static CServerListener* listener = nullptr; /* the primary listener for new games */
static const char* stats_socket_file = "/tmp/freebloks_stats";

/* Shared with the stats thread, protected by mutex. clients and players describe the game that
   is currently waiting for players and are only written by the main thread; the stats thread
   must never touch the CGameServer itself, it is deleted by its game thread. */
static struct {
	time_t time_started;
	int connections_v4, connections_v6;
	int clients, players;
} stats;


#ifndef _WIN32
	static uid_t uid = 0;
	static gid_t gid = 0;
#endif


inline void init_mutex()
{
#ifdef WIN32
	mutex=CreateMutex(nullptr,FALSE,nullptr);
#else
	pthread_mutex_init(&mutex, nullptr);
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

#ifndef WIN32
/* client to connect to given unix domain socket and 'cat' the output from the server */
static int dump_stats(const char *file) {
	struct sockaddr_un addr;
	char buf[100];
	int fd,rc;

	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		return(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, file, sizeof(addr.sun_path)-1);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("connect error");
		return(-1);
	}

	while ( (rc=read(fd,buf,sizeof(buf))) > 0) {
		printf("%.*s", rc, buf);
	}
	if (rc == -1) {
		perror("read");
		return(-1);
	} else if (rc == 0) {
		close(fd);
	}

	return 0;
}

/* dump stats to given file descriptor */
static void print_stats(int fd) {
	lock_mutex();
	const time_t time_started = stats.time_started;
	const int clients = stats.clients;
	const int players = stats.players;
	const int running = games_running;
	const int ran = games_ran;
	const int connections_v4 = stats.connections_v4;
	const int connections_v6 = stats.connections_v6;
	unlock_mutex();

	dprintf(fd, "##### STATS BEGIN #####\n");
	dprintf(fd, "server_started %d\n", (int)time_started);
	dprintf(fd, "server_now %d\n", (int)time(nullptr));
	dprintf(fd, "server_running %d\n", (int)(time(nullptr) - time_started));
	dprintf(fd, "clients %d\n", clients);
	dprintf(fd, "players %d\n", players);
	dprintf(fd, "running %d\n", running);
	dprintf(fd, "ran %d\n", ran);
	dprintf(fd, "connections_v4 %d\n", connections_v4);
	dprintf(fd, "connections_v6 %d\n", connections_v6);
	dprintf(fd, "##### STATS END #####\n");
}

/* opens a unix domain socket, listens for connection and dumps the stat to
 * connected clients */
static void* stat_thread(void* param) {
	char *file = (char*)param;

	lock_mutex();
	stats.time_started = time(nullptr);
	unlock_mutex();

	struct sockaddr_un addr;
	char buf[100];
	int fd,cl,rc;

	unlink(file);
	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("error creating stats socket");
		return nullptr;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, file, sizeof(addr.sun_path)-1);

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("error binding stats socket");
		close(fd);
		return nullptr;
	}

	if (listen(fd, 5) == -1) {
		perror("error listening on stats socket");
		close(fd);
		return nullptr;
	}

	while (1) {
		if ( (cl = accept(fd, nullptr, nullptr)) == -1) {
			perror("stats socket accept error");
			/* Do not spin when descriptors run out */
			if (errno != EINTR && errno != ECONNABORTED) CTimer::sleep(1000);
			continue;
		}

		print_stats(cl);
		close(cl);
	}

	unlink(file);

	return nullptr;
}
#endif


static void help()
{
	printf("Usuage: dedicated [OPTIONS]\n");
	printf("v" VERSION ", build: " __DATE__ "\n\n");
	printf("  -p, --port        Specify the TCP port to accept connections. Default: %d\n",TCP_PORT);
	printf("  -L                Listen on specified IP address. Default: OS default\n");
	printf("  -h, --maxhumans   Define the maximum of human players per game (0-4)\n"
               "                    Default: %d\n",max_humans);
	printf("  -k  --ai          Strength of AI. Lower number means stronger.\n"
	       "                    Default: %d\n", ai_strength);
	printf("  -m, --mode        The game mode for the hosted game. Valid modes:\n"
	       "                    2: 2 colors, 2 players\n"
		   "                    3: 2 colors, 4 players\n"
		   "                    4: 4 colors, 4 players (Default)\n"
		   "                    5: Blokus Duo\n"
	       "      --width       Width of the field. Default: 20\n"
	       "      --height      Height of the field. Default: 20\n"
	       "  -t, --threads     Define number of threads to use for calculating moves\n"
	       "                    Default: %d\n",ai_threads);
	printf("      --no-delay    Default is to force computer moves to last at least 800ms.\n"
		   "                    Use this flag to turn this off.\n");
	printf("  -l, --limit       Maximum number of concurrent running games\n"
	       "                    Default: 5\n");
	printf("      --log         Log to file\n");
	printf("      --stats       Dump stats from running server and exit\n");
#ifndef _WIN32
	printf("      --user\n"
	       "      --group       Drop privileges to that uid/gid\n");
#endif
	printf("      --help        Display this help and exit\n\n");
	exit(0);
}

static void parse_params(int argc,char **argv)
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
		if (strcmp("-k",argv[i])==0 || strcmp("--ai",argv[i])==0)
		{
			i++;
			if (i==argc) {
				printf("%s: Expecting parameter\n",argv[i-1]);
				exit(1);
			}
			ai_strength=atoi(argv[i]);
			if (ai_strength<0 || ai_strength>2000)
			{
				printf("%s: Invalid strength number (%d)\n",argv[i-1],ai_strength);
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
			if (mode<2 || mode>5)
			{
				printf("%s: Invalid mode (%d). Must be one of 2, 3, 4 or 5.\n",argv[i-1],mode);
				exit(1);
			}
			if (mode==2)gamemode=GAMEMODE_2_COLORS_2_PLAYERS;
			if (mode==3)gamemode=GAMEMODE_4_COLORS_2_PLAYERS;
			if (mode==4)gamemode=GAMEMODE_4_COLORS_4_PLAYERS;
			if (mode==5)gamemode=GAMEMODE_DUO;
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
			if (w<FIELD_SIZE_MIN || w>30)
			{
				printf("%s: Invalid width (%d). Must be between %d and 30.\n",argv[i-1],w,FIELD_SIZE_MIN);
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
			if (h<FIELD_SIZE_MIN || h>30)
			{
				printf("%s: Invalid height (%d). Must be between %d and 30.\n",argv[i-1],h,FIELD_SIZE_MIN);
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
			ai_threads=t;
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
		if (strcmp("--no-delay", argv[i]) == 0)
		{
			i++;
			force_delay = 0;
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
		if (strcmp("--stats",argv[i])==0)
		{
			exit(dump_stats(stats_socket_file));
		}
#endif

		printf("Unrecognized commandline parameter: %s\n\n",argv[i]);
		help();
	};
}



/**
 * Block until the given game is fully established and running.
 **/
static int establish_game()
{
	int ret;

	do
	{
		sockaddr_storage client;
		client.ss_family = AF_UNSPEC;

		ret = listener->wait_for_player(true, &client);
		if (ret==-1)
		{
			perror("wait(): ");
			return -1;
		}

		lock_mutex();
		if (client.ss_family == AF_INET6)
			stats.connections_v6++;

		if (client.ss_family == AF_INET)
			stats.connections_v4++;

		stats.clients = listener->get_game()->num_clients();
		stats.players = listener->get_game()->num_players();
		unlock_mutex();

	} while (listener->get_game()->current_player()==-1);

	return 0;
}

#ifdef WIN32
unsigned long WINAPI gameRunThread(void* param)
#else
void* gameRunThread(void* param)
#endif
{
	CGameServer* game=(CGameServer*) param;
	lock_mutex();
	const int game_number = games_ran;
	unlock_mutex();
	CGameLogger logger(logWriter, game_number);
	
	game->setLogger(&logger);
	
	lock_mutex();
	/* The game is running now */
	logger.logFields(LOGLEVEL_INFO, "event=game_started", "Game started with %d clients / %d players. (%d games running)",game->num_clients(), game->num_players(),games_running);
	unlock_mutex();

	game->run();

	lock_mutex();
	games_running--;

	logger.logFields(LOGLEVEL_INFO, "event=game_terminating", "Game terminating. (%d games running)",games_running);

	unlock_mutex();
	delete game;

	return 0;
}


int main(int argc,char ** argv)
{
	int ret;

#ifndef WIN32
	/* A write to a closed socket must fail with EPIPE instead of killing the process */
	signal(SIGPIPE, SIG_IGN);
#endif

	/* Create a ServerListener that listens for connections
	   and lets clients connect */
	listener = new CServerListener();
	CGameLogger logger(logWriter, 0);

	/* Process the command line parameters */
	parse_params(argc,argv);

	/* Clients (and the server's own seeding code) reject configurations like DUO on a field below 5x5 */
	if (!is_valid_game_config(gamemode, width, height))
	{
		printf("Invalid combination of game mode and field size (%dx%d).\n", width, height);
		return 1;
	}

#ifdef WIN32
	/* Initialise Winsock */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,0),&wsadata);
#else
	pthread_t pt;
#endif

#ifndef _WIN32
	if (gid != 0)
	{
		if (setregid(gid,gid))
		{
			/* Never keep running with more privileges than requested */
			logger.log(LOGLEVEL_ERROR, "setregid failed: %s", strerror(errno));
			exit(1);
		}
	}
	if (uid != 0)
	{
		if (setreuid(uid,uid))
		{
			logger.log(LOGLEVEL_ERROR, "setreuid failed: %s", strerror(errno));
			exit(1);
		}
	}
#endif

	if (logfile) 
	{
		CLogFileWriter *fw = new CLogFileWriter();
		if (!fw->createFile(logfile))
		{
			fprintf(stderr, "Cannot open log file %s\n", logfile);
			delete fw;
			return 1;
		}
		logWriter.addLogWriter(fw);
		logger.log(LOGLEVEL_INFO, "Logging to: %s", logfile);
	}


	logger.logFields(LOGLEVEL_INFO, "event=server_starting", "Server starting");

	ret = listener->init(_interface,port);
	if (ret!=0)
	{
	    logger.log(LOGLEVEL_ERROR, "Error calling listen: %s", strerror(errno));
	    return -1;
	}
	init_mutex();

	listener->setLogger(&logger);


#ifndef WIN32
	pthread_create(&pt, nullptr, stat_thread, (void*)stats_socket_file);
#endif

	while (true)
	{
		/* A new game is to be created.
		   AI level is hard */
		listener->new_game(max_humans,ai_strength,gamemode,ai_threads,force_delay);
		/* Set the size */
		listener->get_game()->set_field_size(width, height);
		listener->get_game()->start_new_game(gamemode);
		lock_mutex();
		listener->get_game()->setLogger((CLogger*)&logger);
		unlock_mutex();

		/* A game was created and the listener is listening. Now let the game fill up
		   and wait until a client has started it */
		if (establish_game()==-1)return -1;

		/* Move the current game to a secondary thread */
		lock_mutex();
		games_ran++;
		games_running++;
		/* The game is no longer waiting for players */
		stats.clients = 0;
		stats.players = 0;

#ifdef WIN32
		DWORD threadid;
		CloseHandle(CreateThread(nullptr,0,gameRunThread,(void*)listener->get_game(),0,&threadid));
#else
		if (pthread_create(&pt, nullptr, gameRunThread, (void*)listener->get_game()))
		{
			logger.log(LOGLEVEL_ERROR, "pthread_create failed: %s", strerror(errno));
			/* Nobody will run this game: undo the bookkeeping, disconnect its clients
			   and do not detach a thread that was never created. */
			games_running--;
			delete listener->get_game();
		}
		else if (pthread_detach(pt))
			logger.log(LOGLEVEL_ERROR, "pthread_detach failed: %s", strerror(errno));
#endif

		if (games_running>=max_running_games)
		{
			listener->close();
			
			logger.logFields(LOGLEVEL_WARN, "event=listener_closed", "Stopping accepting new connections due to too many running games");

			while (games_running>=max_running_games)
			{
				unlock_mutex();
				CTimer::sleep(2000);
				lock_mutex();
			}
			logger.logFields(LOGLEVEL_INFO, "event=listener_reopened", "Resuming accepting new connections");

			ret=listener->init(_interface,port);
			if (ret!=0)
			{
				/* Could not "listen", maybe the port is used by another application? */
				logger.log(LOGLEVEL_ERROR, "Error calling listen: %s", strerror(errno));
				return -1;
			}
		}
		unlock_mutex();
	}

	// unreachable code
	delete listener;
	destroy_mutex();

#ifdef WIN32
	/* Shut down Winsock cleanly */
	WSACleanup();
#endif

	return 0;
}

