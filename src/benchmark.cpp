/**
 * benchmark.cpp
 * Author: Sascha Hlusiak
 *
 * Benchmark to test the AI
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
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "gameserver.h"
#include "gameclient.h"


static int width=20;
static int height=20;
static int ai_threads=1;
static int ai_strength=AI_PERFECT;


static void help()
{
	printf("Usuage: benchmark [OPTIONS]\n\n");
	printf("  -k  --ai          Strength of AI. Lower number means stronger.\n"
	       "                    Default: %d\n", ai_strength);
	printf("      --width       Width of the field. Default: 20\n"
	       "      --height      Height of the field. Default: 20\n"
	       "  -t, --threads     Define number of threads to use for calculating moves\n"
	       "                    Default: %d\n", ai_threads);
	printf("      --help        Display this help and exit\n");
	exit(0);
}

static void parseParams(int argc,char **argv)
{
	int i=1;
	while (i<argc)
	{
		if (strcmp("--help",argv[i])==0) help();

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
			ai_threads=t;
			i++;
			continue;
		}

		printf("Unrecognized commandline parameter: %s\n\n",argv[i]);
		help();
	};
}

/**
 * An observer of a game, does not play but records game start and end events and prints time elapsed.
 */
class CBenchmarkClient:public CGameClient
{
private:
	CTimer timer;
	int count;

	virtual void gameStarted();
	virtual void gameFinished();

public:
	CBenchmarkClient();
};

CBenchmarkClient::CBenchmarkClient()
{
	count = 0;
}

void CBenchmarkClient::gameStarted()
{
	timer.reset();
}

void CBenchmarkClient::gameFinished()
{
	printf("-- Game finished! -- Took %.2f sek. --\n",timer.elapsed());
	if (++count >= 3) Disconnect();
	else request_start();
}


static void* clientThread(void* param) {
	int sock = *(int*)param;
	const char* s;

	CBenchmarkClient client;
	client.Connect(sock, 1);

	client.request_start();
	do {
		s = client.poll();
		if (s)
		{
			printf("Disconnected: %s\n", s);
			break;
		}
	} while (client.isConnected());

	return nullptr;
}

int main(int argc,char ** argv)
{
	int ret;

#ifndef WIN32
	/* A write to a closed socket must fail with EPIPE instead of killing the process */
	signal(SIGPIPE, SIG_IGN);
#endif

	int s[2];
	pthread_t pt;

	parseParams(argc,argv);

	printf("This is the almighty Freebloks Benchmark program. Have fun!\n");
	printf("Show help with `benchmark --help`\n\n");

	if (socketpair(PF_UNIX, SOCK_STREAM, 0, s) < 0) {
		perror("socketpair");
		return 1;
	}

    CGameServer server(0, ai_strength, GAMEMODE_4_COLORS_4_PLAYERS, false);
	server.set_ai_threads(ai_threads);
	server.set_field_size(width, height);
	server.start_new_game(GAMEMODE_4_COLORS_4_PLAYERS);
	server.add_client(s[1]);

	if (pthread_create(&pt, nullptr, clientThread, (void*)&s[0])) {
        perror("pthread_create");
    }

	if (pthread_detach(pt)) {
        perror("pthread_detach");
    }

	server.run();

	return 0;
}
