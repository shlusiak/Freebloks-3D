/**
 * gui.cpp
 * Author: Sascha Hlusiak
 * Implementation of the GUI class, responsible for the graphics and the heart of the
 * game
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#endif

#include "gui.h"
#include "constants.h"
#include "gameserver.h"
#include "guigameclient.h"
#include "player.h"
#include "dialogs.h"
#include "intro.h"
#include "chatbox.h"
#include "gamedialogs.h"
#include "helpbox.h"



// Width of the bevel on the board
const double bevel_size=0.18;
// The border of the board has this height
const double border_bottom=-0.6;
static float light0_ambient[] = {0.35f, 0.35f, 0.35f, 1.0f};
static float light0_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
static float light0_specular[] = {1.0,1.0,1.0, 1.0f};


/**
 * Constructor, initialises everything sensibly, mostly with nullptr
 * Properly initialised in the run() method.
 **/
CGUI::CGUI()
{
#ifdef WIN32
	/* Initialise Winsock so we can use sockets on Windows */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,0),&wsadata);
#endif
	/* Create the window */
	window=new CWindow(this);

	game=nullptr;
	lastx=lasty=0;
	clickx=clicky=0;
	selectedx=selectedy=-1;
	selected_allowed=false;
	current_stone=-1;
	hovered_stone=-1;
	selected_arrow=0;
	anim=0.0;
	effects=nullptr;
	menu=nullptr;
	chatbox=nullptr;
	font=nullptr;
	intro=nullptr;

	startupParams.remotehost=nullptr;
	startupParams.remoteport=TCP_PORT;
	startupParams.firststart=true;
	startupParams.intro=true;
	startupParams.humans=1;
	startupParams.threads=2;


	/* Initialise the camera */
	angx=60.0;
	angy=0.0;
	camera_distance=30.0;

	// Set the options to sensible defaults; the default is 1!
	options.set(OPTION_SHOW_FPS,0);
}

CGUI::~CGUI()
{
	/* Remove dynamic variables cleanly */
	if (intro)delete intro;
	if (window)delete window;
	if (font)delete font;
	if (effects)delete effects;
	if (game)delete game;
#ifdef WIN32
	/* Unload Winsock cleanly */
	WSACleanup();
#endif
}

/**
 * Print the help text
 **/
void CGUI::printHelp()const
{
	printf("Usuage: freebloks [OPTIONS]\n\n");
	printf("  -c, --connect     Connect to a running network game\n");
	printf("  -p, --port        Specify the TCP port to connect to. Default: %d\n",TCP_PORT);
	printf("  -h, --humans      Number of huban players for the first game (0-4)\n"
               "                    Default: %d\n",1);
	printf("  -t, --threads     Use multiple threads for the AI\n"
	       "                    Default: 1\n");
	printf("      --nointro     Disable the intro sequence and start immediately\n");

	printf("      --help        Display this help and exit\n");
}

/**
 * Parse the command line
 **/
bool CGUI::parseCmdLine(int argc,char **argv)
{
	int i;
	i=1;
	while (i<argc)
	{
		if (strcmp(argv[i],"--help")==0)
		{
			printHelp();
			return false;
		}else if ((strcmp(argv[i],"-c")==0)||(strcmp(argv[i],"--connect")==0))
		{
			if (++i>=argc) {
				printf("--connect: Missing parameter\n\n");
				printHelp();
				return false;
			}
			if (startupParams.remotehost)free(startupParams.remotehost);
				startupParams.remotehost=strdup(argv[i]);
		}else if ((strcmp(argv[i],"-p")==0)||(strcmp(argv[i],"--port")==0))
		{
			if (++i>=argc) {
				printf("--port: Missing parameter\n\n");
				printHelp();
				return false;
			}
			if (atoi(argv[i])<0){
				printf("--port: Invalid parameter. Excepting port number\n\n");
				printHelp();
				return false;
			}
			startupParams.remoteport=atoi(argv[i]);
		}else if ((strcmp(argv[i],"-h")==0)||(strcmp(argv[i],"--humans")==0))
		{
			if (++i>=argc) {
				printf("--humans: Missing parameter\n\n");
				printHelp();
				return false;
			}
			if (atoi(argv[i])>4){
				printf("--humans: Invalid parameter. Excepting number between 0 and 4\n\n");
				printHelp();
				return false;
			}
			startupParams.humans=atoi(argv[i]);
		}else if ((strcmp(argv[i],"-t")==0)||(strcmp(argv[i],"--threads")==0))
		{
			if (++i>=argc) {
				i--;
				startupParams.threads=2;
			}else if ((atoi(argv[i])<1)||(atoi(argv[i])>8)){
				printf("--threads: Invalid parameter. Excepting number between 1 and 8\n\n");
				printHelp();
				return false;
			}else startupParams.threads=atoi(argv[i]);
		}else if (strcmp(argv[i],"--nointro")==0)
		{
			startupParams.intro=false;
		}else{
			printf("Unknown parameter: %s\n\n",argv[i]);
			printHelp();
			return false;
		}
		i++;
	}
	return true;
}


/**
 * Starts a single player game with the given
 * gamemode, players (local players), diff (difficulty),
 * width/height and the stone counts per shape.
 * Returns: success
 **/
bool CGUI::startSingleplayerGame(GAMEMODE gamemode,int players,int diff,int width,int height,int8 stone_numbers[],int ai_threads)
{
	int r;
	const char *err;
	char path[256];
	int port;

	/* Delete all existing stone effects (except the head of the list) */
	if (effects)effects->clear(); else effects=new CStoneEffect();
	newCurrentPlayer(-1);
	current_stone=-1;
	if (game)delete game;

	/* Start a local server in a secondary thread
	   with the given parameters. The server runs on
	   port (TCP_PORT+1) */
#ifdef WIN32
	strcpy(path, "127.0.0.1");
	port = TCP_PORT+1;
#else
	sprintf(path, P_tmpdir"/freebloks-%d", rand()%131072);
	port = 0;
#endif


	r=CGameServer::run_server(path,port,PLAYER_MAX,diff,width,height,gamemode,stone_numbers,ai_threads);
	if (r!=0)
	{
		char t[512];
		sprintf(t,"Could not run local server:\n%s.",strerror(errno));
		widgets.addSubChild(new CMessageBox(270,150,"Error",t));
		/* But do not abort; a server might be running anyway. */
	}

	game=new CGUIGameClient(this);

	err=game->Connect(path, port);
	if (port == 0)
		unlink(path);
		
	/* Reset the timer so the connect does not cause a jump in the
	   animation. */
	timer.reset();
	if (err!=nullptr)
	{
		char t[300];
		sprintf(t,"Could not connect to local server:\n%s.",err);
		widgets.addSubChild(new CMessageBox(300,150,"Error",t));
		return false;
	}
	/* Now request [players] local players. */
	for (int i=0;i<4;i++)
		if ((players & (1 << i)) != 0)
			game->request_player(i, nullptr);
	/* Since the game is local, it can start right away. Request the game start */
	game->request_start();
	/* Success */
	return true;
}

/**
 * Create (host) a multiplayer game.
 * Parameters as above,
 * maxhumans is the maximum number of human players; the rest is taken by
 * computer opponents
 **/
bool CGUI::startMultiplayerGame(GAMEMODE gamemode,int localplayers,int diff,int width,int height,int8 stone_numbers[],int ai_threads, const char* name)
{
	int r=CGameServer::run_server(nullptr,TCP_PORT,4,diff,width,height,gamemode,stone_numbers,ai_threads);
	if (r!=0)
	{
		char t[300];
		sprintf(t,"Could not run server:\n%s.",strerror(errno));
		widgets.addSubChild(new CMessageBox(270,150,"Error",t));
		return false;
	}

	return joinMultiplayerGame("localhost",TCP_PORT,localplayers, name);
}

/**
 * Join a network game.
 * _host is the name of the server (in the form "host[:port]")
 * players local players are requested
 **/
bool CGUI::joinMultiplayerGame(const char *host, int port, int players, const char* name)
{
	const char *err;

	if (effects)effects->clear();else effects=new CStoneEffect();
	/* no current player, and no current stone selected */
	newCurrentPlayer(-1);
	current_stone=-1;
	if (game)delete game;
	game=new CGUIGameClient(this);

	/* Try to connect to the server */
	err=game->Connect(host, port);
	/* Reset the timer so the connect does not cause a jump in the
	   animation. */
	timer.reset();
	if (err!=nullptr)
	{
		char t[300];
		sprintf(t,"Could not connect to remote server:\n%s.",err);
		widgets.addSubChild(new CMessageBox(300,150,"Error",t));
		return false;
	}else{
		for (int i=0;i<4;i++)
			if ((players & (1 << i)) != 0)
				game->request_player(i, name);
		/* Then show the StartGameDialog modally as the lobby. */
		widgets.addSubChild(new CStartGameDialog(game,this,host));
		/* Success, the connection is up. */
		return true;
	}
}

/**
 * Main method of the GUI, called from main()
 * Takes care of proper initialisation before starting the
 * main loop, and runs it
 **/
void CGUI::run()
{
	if (!window->createWindow())return;

	game=new CGUIGameClient(this);

	/* Make the window visible. */
	window->show();
	initGL();

	/* Create the global font object here. Even though we do not use it,
	   it keeps the global texture alive. */
	font=new CGLFont(1.0,2.0);
	texture_ground.load("ground.bmp");
	texture_wall.load("wall.bmp");
	texture_field.load("field.bmp");

	createDisplayLists();

	{
		/* A transparent frame as the toolbar */
		CFrame *frame=new CFrame(0,0,640,25);
		frame->setAlpha(0.70);
		widgets.add(frame);

		/* Add subchilds to the frame, on the same level
		   as the toolbar. Subwindows of subchilds are
		   modal to each other, but the toolbar still responds */
		frame->add(subchilds=new CWidget());

		frame->addChild(new CButton(5,3,60,19,9000,frame,"Menu"));
		frame->addChild(hint=new CButton(70,3,65,19,8,frame,"Hint"));
		frame->addChild(undo=new CButton(140,3,65,19,9,frame,"Undo"));
		frame->addChild(new CButton(575,3,60,19,2,frame,"Quit"));

		/* Add a frame for the active player and make it invisible */
		frame->add(playerframe=new CFrame(310,0,250,25));
		playerframe->setAlpha(0.0);
		playerframe->add(playertext=new CStaticText(0,5,250,"",playerframe));
	}
	/* Create the chat box as a subchild. The main menu would be modal to it. */
	subchilds->addSubChild(chatbox=new CChatBox(0,25,this));
	/* Add the FPS counter at the bottom of the screen. It only renders itself
	   when the corresponding option is enabled. */
	widgets.add(new CFPS(2,465,this));

	/* Instantiate the intro sequence shown at start-up. */
	if (startupParams.intro)showIntro();
		else startFirstGame();

	render();

	/* Then reset the timer so things start smoothly */
	timer.reset();
	while (!window->isClosed())
	{
		/* Keep processing window messages alternately. */
		window->processEvents();
		const char *err=game->poll();
		if (err!=nullptr)
		{
			/* And on error show the corresponding error message,
			   probably "Connection Lost" or similar. */
			char c[1000];
			sprintf(c,"Connection to server lost:\n%s.",err);
			/* Show a fully modal dialog */
			widgets.addSubChild(new CMessageBox(300,200,"Error",c));
		}
		/* checkPoint checks whether it is time for a frame;
		   then it animates and renders, otherwise it sleeps for ~5 ms
		   to relieve the CPU. */
		if (checkPoint(options.get(OPTION_LIMITFPS)?(1.0/(double)options.get(OPTION_FPS)):0))timer.sleep(5);
	}
}

/**
 * If at least mintime seconds have passed, render and animate a frame.
 * Returns true if it is not time yet, in which case the caller sleeps.
 * On false a frame was rendered, so do not sleep.
 **/
bool CGUI::checkPoint(double mintime)
{
	/* Time elapsed since the last frame. */
	const double elapsed=timer.elapsed();
	if (elapsed>=mintime)
	{
		timer.reset();
		render();
		execute(elapsed);
		return false;
	}
	return true;
}

/**
 * End the intro cleanly. If the menu is not open, this is
 * the very first intro sequence. Start a single player game
 * right after it.
 **/
void CGUI::stopIntro()
{
	if (!intro)return;
	/* Remove the intro. */
	delete intro;
	intro=nullptr;
	startFirstGame();
}

void CGUI::startFirstGame()
{
	if (!startupParams.firststart)return;
	startupParams.firststart=false;

	if (startupParams.remotehost)
		joinMultiplayerGame(startupParams.remotehost,startupParams.remoteport,startupParams.humans, nullptr);
	else startSingleplayerGame(GAMEMODE_4_COLORS_4_PLAYERS,startupParams.humans,AI_MEDIUM,20,20, nullptr,startupParams.threads);
}


void CGUI::showIntro()
{
	if (intro)return;
	intro=new CIntro(this);
}

/**
 * Called by CWindow when a mouse event occurs.
 * Dispatch it here and handle it if appropriate.
 **/
void CGUI::processMouseEvent(TMouseEvent *e)
{
	/* While the intro runs, it gets the mouse exclusively */
	if (intro)
	{
		if (e->press && e->button1){
			stopIntro();
			return;
		}
		return;
	}
	/* If handleUI returns true, a dialog exists that prevents
	   handling by the GUI (selecting or placing stones) */
	bool dialog=widgets.handleUI();
	if (dialog || e->press)
	{
		/* If dialogs want to handle the mouse exclusively, first
		   remember the position */
		lastx=e->x;
		lasty=e->y;
		testSelection(e->x,e->y);
	}
	/* Always let the widgets handle the mouse. */
	int r=widgets.processMouseEvent(e);

	switch(r)
	{
	/* The menu button of the toolbar was pressed. */
	case 9000: 
	{
		if (menu){
			menu->close(true);
			menu=nullptr;
		}else{
			subchilds->addSubChild(menu=new CMainMenu(this,false));
		}
		break;
	}
	/* The "Quit" button was pressed; end the game by closing the window. */
	case 2: window->close();
		break;
	/* The main menu wants to show the intro. */
	case 5: showIntro();
		break;
	/* The "Hint" button was pressed; show a suggested move. */
	case 8: showHint();
		break;
	/* "Undo" button. If a local player is to move, try to undo the
	   turn. */
	case 9: if (game->is_local_player())game->request_undo();
		break;
	}

	/* If r!=0, the event was handled by some widget.
	   If dialog, a dialog demands exclusive handling, whether or not it just
	   handled the event. In both cases, bail out. */
	if (r!=0 || dialog)return;

	/* Otherwise the GUI may handle the event here, place stones, etc. */

	if (e->press)
	{
		if (e->button1 && !e->button2 && !e->button3 && !e->button4 && !e->button5)
		{
			if (hovered_stone!=-1)
			{
				current_stone=hovered_stone;
				return;
			}else if (selectedx!=-1 && selectedy!=-1 && current_stone!=-1)
			{
				/* If a stone is active and a square is selected */
				if (selected_allowed)
				{
					/* If this square is also marked as "allowed",
					   make this move. */
					/* First get the selected CStone */
					CStone& stone=game->get_current_player()->get_stone(current_stone);
					int x=selectedx-stone.get_stone_size()/2;
					int y=selectedy-stone.get_stone_size()/2;
					if (options.get(OPTION_ANIMATE_STONES))
						addEffect(new CStoneRollEffect(this,&stone,current_stone,game->current_player(),x,y,false));
					/* Let the game client place the stone. Sends a request to the game server. */
					if (game->request_set_stone(stone, y, x)==FIELD_ALLOWED)
						newCurrentPlayer(game->current_player());
					current_stone=-1;
				}
			}else if (selected_arrow>0 && current_stone!=-1)
			{
				/* If a stone is selected AND an arrow */
				/* Get the selected CStone*. */
				CStone& stone=game->get_current_player()->get_stone(current_stone);

				if (selected_arrow==1)stone.rotate_right();
				if (selected_arrow==2)stone.rotate_left();
				if (selected_arrow==3)stone.mirror_over_y();
				if (selected_arrow==4)stone.mirror_over_x();
			}
			/* Apparently the click hit empty space; deselect the stone. */
			else current_stone=-1;
		}
		/* If the right mouse button was pressed, remember the position. Needed for the release later */
		if (e->button2 && !e->button1 && !e->button3 && !e->button4 && !e->button5)
		{
			clickx=e->x;
			clicky=e->y;
		}
		/* The mouse wheel was turned. If no mouse button is held, rotate the current stone. */
		if (e->button4 && !e->button1 && !e->button2 && !e->button3 && !e->button5)
			if (current_stone!=-1)game->get_current_player()->get_stone(current_stone).rotate_left();
		if (e->button5 && !e->button1 && !e->button2 && !e->button3 && !e->button4)
			if (current_stone!=-1)game->get_current_player()->get_stone(current_stone).rotate_right();

		/* The mouse wheel was turned while the right mouse button is down:
		   scroll through all stones. */
		if (e->button4 && !e->button1 && e->button2 && !e->button3 && !e->button5)
		{
			/* Set the click to an impossible position so nothing is mirrored on release. */
			clickx=clicky=-1000;
			for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
			{
				current_stone--;
				/* Wrap around from the front to the back. */
				if (current_stone<0)current_stone=STONE_COUNT_ALL_SHAPES-1;
				/* And if the stone is even available, be happy and bail out. */
				if (game->get_current_player()->get_stone(current_stone).get_available())
					return;
			}
			/* Otherwise select no stone, since none is left. */
			current_stone=-1;
		}
		/* The same the other way round */
		if (e->button5 && !e->button1 && e->button2 && !e->button3 && !e->button4)
		{
			clickx=clicky=-1000;
			for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
			{
				current_stone++;
				if (current_stone>=STONE_COUNT_ALL_SHAPES)current_stone=0;
				if (game->get_current_player()->get_stone(current_stone).get_available())
					return;
			}
			current_stone=-1;
		}


		testSelection(e->x,e->y);

		lastx=e->x;
		lasty=e->y;
	}else if (e->release)
	{
		if (e->button2){
			/* And the mouse moved little enough since the click,
			   a stone is selected and the player is to move. */
			if (abs(e->x-clickx)<4 && abs(e->y-clicky)<4 && game->is_local_player() && current_stone!=-1)
			{
				/* Then mirror the current stone about the screen's Y axis. */
				CStone &s=game->get_current_player()->get_stone(current_stone);
				/* So this depends on the viewing angle, i.e. angy */
				if (angy<45.0 || angy>315.0 || ((angy<45+180) && (angy>-45+180)))
					s.mirror_over_y();
				else s.mirror_over_x();
			}
		}
		testSelection(e->x,e->y);
	}else if (e->move)
	{
		if (e->button2)
		{
			/* Apply delta x and delta y to angy and angx. */
			angy+=double(e->x-lastx)*0.35;
			angx+=double(e->y-lasty)*0.30;
			while (angy<0)angy+=360.0;
			while (angy>360.0)angy-=360.0;
			/* Clamp angx. */
			if (angx<15.0)angx=15.0;
			if (angx>90.0)angx=90.0;
		}else if (e->button3)
		{
			camera_distance+=double(e->y-lasty)*0.1;
			if (camera_distance<25.0)camera_distance=25.0;
			if (camera_distance>40.0)camera_distance=40.0;
		}else testSelection(e->x,e->y);
		/* Remember the last mouse position. */
		lastx=e->x;
		lasty=e->y;
	}
}

/**
 * Called by CWindow when a key press occurs.
 * Dispatch and handle it here.
 **/
void CGUI::processKeyEvent(unsigned short key)
{
	if (intro)
	{
		/* If the intro runs and Esc, Return or Space is pressed, end the intro. */
		if (key==VK_ESCAPE || key==VK_RETURN || key==VK_SPACE)stopIntro();
		return;
	}

	if (widgets.processKey(key))return;
	/* On Escape, show the menu if not already open. The menu closes itself on Escape. */
	if (key==VK_ESCAPE && !menu)
		subchilds->addSubChild(menu=new CMainMenu(this,false));
	else if (key==VK_F1)
		widgets.addSubChild(new CHelpBox());
	/* On Space with a stone selected */
	else if (key==VK_SPACE && game->is_local_player() && current_stone!=-1)
	{
		/* Mirror it about the screen's Y axis. */
		CStone &s=game->get_current_player()->get_stone(current_stone);
		if (angy<45.0 || angy>315.0 || ((angy<45+180) && (angy>-45+180)))
			s.mirror_over_y();
		else s.mirror_over_x();
	}else 
	/* On Return while a game is running, show the chat box. */
		if (key==VK_RETURN && !menu && game->isConnected())chatbox->show();
	else if (key=='h')showHint();
	else if (game->is_local_player()) {
		/* Up key: rotate the current stone to the left */
		if (key==VK_UP && current_stone!=-1)
			game->get_current_player()->get_stone(current_stone).rotate_left();
		/* Down key: rotate it to the right. */
		if (key==VK_DOWN && current_stone!=-1)
			game->get_current_player()->get_stone(current_stone).rotate_right();

		if (key==VK_LEFT)
		{
			for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
			{
				current_stone--;
				if (current_stone<0)current_stone=STONE_COUNT_ALL_SHAPES-1;
				if (game->get_current_player()->get_stone(current_stone).get_available())
					return;
			}
			current_stone=-1;
		}
		if (key==VK_RIGHT)
		{
			for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
			{
				current_stone++;
				if (current_stone>=STONE_COUNT_ALL_SHAPES)current_stone=0;
				if (game->get_current_player()->get_stone(current_stone).get_available())
					return;
			}
			current_stone=-1;
		}
		
	}
}

void CGUI::initGL()
{
	/* We want a bluish blue as the background colour. Only visible when textures are disabled anyway. */
	glClearColor( 0.05f, 0.10f, 0.25f, 1.0f );
	/* Set the stencil clear value */
	glClearStencil(0);
	glClearDepth(1);
	
	glEnable(GL_DEPTH_TEST);
	/* Cull back faces */
	glEnable(GL_CULL_FACE);
	
	/* Let there be light! */
	glEnable(GL_LIGHTING);
	/* OpenGL should renormalise normals, otherwise scaling causes odd effects. */
	glEnable(GL_NORMALIZE);
	
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0,GL_POSITION,light0_pos);
	glLightfv(GL_LIGHT0,GL_AMBIENT,light0_ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,light0_diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,light0_specular);

	displayLists=glGenLists(2);
}

/**
 * Creates a few OpenGL display lists that can be executed faster later
 **/
void CGUI::createDisplayLists()
{
	// Environment
	glNewList(displayLists,GL_COMPILE);
	{
		/* Radius of the table board */
		const double r=45.0;
		/* How far away the wall is. */
		const double wall_r=160.0;
		/* Y position of the board. Right below the playing field */
		const double y=border_bottom;
		/* How often the board texture repeats. */
		const double tile=5.0;
		/* How often the wall texture repeats. */
		const double wall_tile=7;
	
		/* Colour of the board */
		float d[4]={0.6f,0.45f,0.25f,1.0f};
		/* Specular highlight of the board */
		float s[4]={0.35f,0.3f,0.3f,1.0f};
		/* Shininess of the board */
		float sh[]={15.0f};
		/* Set the material */
		glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,d);
		glMaterialfv(GL_FRONT,GL_SPECULAR,s);
		glMaterialfv(GL_FRONT,GL_SHININESS,sh);
		
		glDepthFunc(GL_ALWAYS);
		
		/* The wall is rendered without lighting. */
		glDisable(GL_LIGHTING);
		/* Activate the wall texture. */
		texture_wall.activate();
		/* 4 sides */
		for (int i=0;i<4;i++)
		{
			/* The wall is rendered slightly above 0 vertically, since you cannot look that far below anyway */
			const double offs=+wall_r/2.0;
			/* 100% texture */
			glColor3d(1,1,1);
			glBegin(GL_QUADS);
			glNormal3d(0,0,+1);
			glTexCoord2d(0,0);			glVertex3d(-wall_r,offs-wall_r,-wall_r);
			glTexCoord2d(wall_tile,0);		glVertex3d(+wall_r,offs-wall_r,-wall_r);
			glTexCoord2d(wall_tile,wall_tile);	glVertex3d(+wall_r,offs+wall_r,-wall_r);
			glTexCoord2d(0,wall_tile);		glVertex3d(-wall_r,offs+wall_r,-wall_r);
			glEnd();
			/* Then rotate by 90 degrees so the next wall can be rendered. */
			glRotated(90.0,0,1,0);
		}
		texture_wall.deactivate();
		/* Switch the light on again. */
		glEnable(GL_LIGHTING);


		texture_ground.activate();

		glBegin(GL_QUADS);
		glNormal3d(0,1,0);
		glTexCoord2d(0,0);		glVertex3d(-r,y,r);
		glTexCoord2d(tile,0);		glVertex3d(r,y,r);
		glTexCoord2d(tile,tile);	glVertex3d(r,y,-r);
		glTexCoord2d(0,tile);		glVertex3d(-r,y,-r);
		glEnd();
		/* Deactivate the texture properly. */
		texture_ground.deactivate();
		
		glDepthFunc(GL_LESS);
	}
	glEndList();

	glNewList(displayLists+1,GL_COMPILE);
	{
		/* Outer radius and inner radius */
		const double r1=stone_size;
		const double r2=stone_size-bevel_size;
		/* Direction of the normals */
		static const double normal_y=0.7;
		static const double normal_x=sqrt(1.0-normal_y*normal_y);
		
		glBegin(GL_QUADS);

		/* Render the top side. */
		glNormal3d(0,1,0);
		glVertex3d(-r2,bevel_height,+r2);
		glVertex3d(+r2,bevel_height,+r2);
		glVertex3d(+r2,bevel_height,-r2);
		glVertex3d(-r2,bevel_height,-r2);
	
		/* then the bevel on top */
		glNormal3d(0,normal_y,-normal_x);
		glVertex3d(-r2,bevel_height,-r2);
		glVertex3d(+r2,bevel_height,-r2);
		glVertex3d(+r1,0,-r1);
		glVertex3d(-r1,0,-r1);

		/* Bevel at the bottom */
		glNormal3d(0,normal_y,normal_x);
		glVertex3d(+r2,bevel_height,+r2);
		glVertex3d(-r2,bevel_height,+r2);
		glVertex3d(-r1,0,+r1);
		glVertex3d(+r1,0,+r1);

		/* Bevel on the left */
		glNormal3d(-normal_x,normal_y,0);
		glVertex3d(-r2,bevel_height,+r2);
		glVertex3d(-r2,bevel_height,-r2);
		glVertex3d(-r1,0,-r1);
		glVertex3d(-r1,0,+r1);

		/* Bevel on the right */
		glNormal3d(normal_x,normal_y,0);
    		glVertex3d(+r2,bevel_height,-r2);
		glVertex3d(+r2,bevel_height,+r2);
		glVertex3d(+r1,0,+r1);
		glVertex3d(+r1,0,-r1);
		glEnd();
	}
	glEndList();
}

/**
 * Animate all objects by delta t=elapsed [s].
 **/
void CGUI::execute(double elapsed)
{
	/* Increase the global animation counter. Used e.g. by the arrows and other animations. */
	anim+=elapsed;
	/* Animate the effects. */
	if (effects)effects->execute(elapsed);
	widgets.execute(elapsed);
	/* If after animating the menu exists but wants to be removed, set it to 0. */
	/* Widgets that want to be removed still exist for exactly one more pass. */
	if (menu && menu->deleted())menu=0;
	/* If there is an intro and it says it has finished, stop it. */
	if (intro)
		if (intro->execute(elapsed))stopIntro();
}

/**
 * Renders the scene around window position x/y into the selection buffer and checks
 * which face is under the mouse pointer. Passes that on to the responsible parts of the program.
 **/
void CGUI::testSelection(int x,int y)
{
	/* The "names" of the faces under the mouse go here. 512 should be enough. */
	static GLuint buffer[512];
	GLint viewport[4];
	int hits;
	int type;
	int i,j;
	/* While the intro is running, none of this is needed. */
	if (intro)return;
	/* Initialise the select buffer. */
	glSelectBuffer(sizeof(buffer)/sizeof(GLuint), buffer);
	
	glGetIntegerv(GL_VIEWPORT,viewport);
	/* Switch on select mode and initialise the names. */
	glRenderMode(GL_SELECT);
	glInitNames();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	/* Important: the pick matrix */
	gluPickMatrix(x,viewport[3]-y,3.0,3.0,viewport);
	glPushMatrix();
	gluPerspective(fov, (float)viewport[2]/(float)viewport[3], 1.0, 300.0);

	/* Build the view matrix. */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0,4,0);
	gluLookAt(camera_distance*sin(-angy*M_PI/180.0)*cos(angx*M_PI/180.0),
		camera_distance*sin(angx*M_PI/180.0),
		camera_distance*cos(angy*M_PI/180.0)*cos(angx*M_PI/180.0),
		0,0,0,
		0,1,0);

	/* If a dialog handles the mouse exclusively, some rendering work can be saved here. */
	if (!widgets.handleUI())
	{
		glPushMatrix();
		/* If one of the player's stones is active, tile the board with squares
		   to find out which square the player selected. */
		if (current_stone!=-1)
		{
			glTranslated(	-(game->get_field_size_x()-1)*stone_size,
					0,
					-(game->get_field_size_y()-1)*stone_size);
			/* type=1, i.e. indicate that this is the board. */
			glPushName(1);
			for (i=0;i<game->get_field_size_x();i++)
			{
				/* The second name is the X coordinate. */
				glPushName(i);
				for (j=0;j<game->get_field_size_y();j++)
				{
					/* The third name is Y */
					glPushName(j);
					renderQuad(stone_size);
					glPopName();
					/* Move to the next stone in Z direction */
					glTranslated(0,0,stone_size*2.0);
				}
				glPopName();
				/* And start the next row. */
				glTranslated(stone_size*2.0,0,-(game->get_field_size_y())*stone_size*2.0);
			}
			glPopName();
		}
		glPopMatrix();
		/* For a local player, also render the player's stones into the selection buffer. */
		if (game->is_local_player())
			renderPlayerStones(game->current_player(),1.0,true);
	}

	widgets.render(true);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	/* Clean up the matrix stack again */
	
	/* End selection mode. hits is the number of faces hit */
	hits = glRenderMode(GL_RENDER);

	GLuint names, *ptr, minZ,*ptrNames=0, numberOfNames=0;
	selectedx=selectedy=-1;
	hovered_stone=-1;
	selected_arrow=0;

	if (hits==0)
	{
		widgets.processSelection(0);
		return;
	}
	/* Go through all faces and remember the one with the lowest Z value. */
	ptr = (GLuint *) buffer;
	minZ = 0xffffffff;
	for (i = 0; i < hits; i++) {
		names = *ptr;
		ptr++;
		type=*(ptr+2);
		if (*ptr < minZ || (names>=1 && type==3)) {
			numberOfNames = names;
			minZ = *ptr;
			ptrNames = ptr;
			if (*(ptr+2)==3)break;
		}
  
		ptr += names+2;
	}
	if (ptrNames==0)
	{
		widgets.processSelection(0);
		return;
	}
	/* The type is in the first name */
	type=*(ptrNames+2);
	ptr=ptrNames+3;
	/* type==1 is the board */
	if (type==1 && numberOfNames==3)
	{
		selectedx=*(ptr);
		selectedy=*(ptr+1);

		selected_allowed=true;
		if (current_stone!=-1)
		{
			/* Check whether the current stone would fit at this position. */
			const CStone &stone=game->get_current_player()->get_stone(current_stone);
			int vx=selectedx-stone.get_stone_size()/2,vy=selectedy-stone.get_stone_size()/2;
			/* And remember whether the stone would fit. */
			selected_allowed = game->is_valid_turn(stone, game->current_player(),vy,vx);
		}
	}else if (type==2 && numberOfNames==2)
	{
		/* The mouse is over one of the player's stones. Highlight that stone. */
		hovered_stone=*(ptr);
	}else if (type==3 && numberOfNames==2)
	{
		/* The mouse is over an arrow. */
		selected_arrow=*(ptr);
	}

	/* The mouse is over a widget. Inform all widgets about it */
	if (type==4 && numberOfNames==2)
		widgets.processSelection(*(ptr));
	/* Otherwise the mouse is over no widget; always inform them about that too */
	else widgets.processSelection(0);
}

void CGUI::render()
{
	int i,j;
	GLint viewport[4];

	/* Clear the buffer */
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	unsigned int c=0;
	if (!options.get(OPTION_ENVIRONMENT))c|=GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT;
	if (options.get(OPTION_SHADOW) && window->canShadow() && (intro || (effects && effects->hasShadow())))c|=GL_STENCIL_BUFFER_BIT;
	if (c)glClear(c);

	/* Get the size of the viewport and build a perspective matrix from it */
	glGetIntegerv(GL_VIEWPORT,viewport);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, (float)viewport[2]/(float)viewport[3], 1.0, 300.0);

	glMatrixMode(GL_MODELVIEW);
	if (!intro)
	{
		/* If the intro is _not_ running, build a modelview matrix here */
		glLoadIdentity();
		glTranslated(0,4,0);
		gluLookAt(camera_distance*sin(-angy*M_PI/180.0)*cos(angx*M_PI/180.0),
			camera_distance*sin(angx*M_PI/180.0),
			camera_distance*cos(angy*M_PI/180.0)*cos(angx*M_PI/180.0),
			0,0,0,
			0,1,0);

		/* And set the light again here so it matches the viewer's current position
		   again. Otherwise e.g. specular highlights are wrong. */
		glLightfv(GL_LIGHT0,GL_POSITION,light0_pos);
	}

	if (intro)
	{
		/* The intro takes care of everything itself, including the modelview matrix. */
 		intro->render();
	}else {
		/* First render the environment: floor, wall */
		renderGround();
		renderField();
		for (i=0;i<game->get_field_size_x();i++)
		for (j=0;j<game->get_field_size_y();j++)
			if (game->get_game_field(j,i)!=FIELD_FREE)
		{
			/* but only render if not handled by an effect. In that case the effect
			   renders this stone. */
			if (!effects || !effects->is_effected(i,j))
				renderStone(i,j,game->get_game_field(j,i),0.65f,false);
		}
		if (effects)effects->render();
		if (game) {
			for (int i=0;i<4;i++)
			{
				renderPlayerStones(i,(game->isConnected() && game->current_player()!=-1)?1.0:0.3,false);
			}
		}
	}

	/* If shadows are on and the intro or effects want shadows, render the shadows here! */
	if (options.get(OPTION_SHADOW) && window->canShadow() && (intro || (effects && effects->hasShadow())))
	{
		glPushAttrib( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_STENCIL_BUFFER_BIT );

		/* Do not write to the depth buffer, but still test against it */
		glDepthMask( GL_FALSE );
		glDepthFunc( GL_LEQUAL );
		/* Always draw into the stencil buffer */
		glEnable( GL_STENCIL_TEST );
		glStencilFunc( GL_ALWAYS, 1, 0xFFFFFFFFL );
		/* Do not draw into the colour buffer */
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		
		/* First draw only the front faces of the shadow volumes, incrementing the stencil value. */
		glFrontFace( GL_CCW );
		glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
		if (intro)intro->renderShadow(); else if (effects)effects->renderShadow();

		/* Then render only the back faces of the shadow volumes, decrementing the stencil. */
		glFrontFace( GL_CW );
		glStencilOp( GL_KEEP, GL_KEEP, GL_DECR );
		if (intro)intro->renderShadow(); else if (effects)effects->renderShadow();

		/* Reset again, i.e. render front faces, into the colour buffer, etc. */
		glFrontFace( GL_CCW );
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		
		/* Alpha blending on, lighting off; we draw the matte shadow plane */
		glEnable( GL_BLEND );
		glDisable(GL_LIGHTING);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		/* Render where the stencil buffer is not 0 */
		glStencilFunc( GL_NOTEQUAL, 0, 0xFFFFFFFFL );
		glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

		/* Build a plain modelview and orthographic projection matrix */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0,1,0,1,0,1);

		/* Render a black, highly transparent quad. */
		glColor4f( 0.0f, 0.0f, 0.0f, 0.25f );
		glBegin( GL_TRIANGLE_STRIP );
		glVertex3f(-1,1,0);
		glVertex3f(-1,-1,0);
		glVertex3f(1,1,0);
		glVertex3f(1,-1,0);
		glEnd();

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopAttrib();
	}

	/* If no intro is running, render the widgets. */
	if (!intro)
	{
		widgets.render(false);
	}

#ifndef WIN32
	GLenum error=glGetError();
	if (error!=GL_NO_ERROR)printf("GL Error %d\n",error);
#endif


	window->swapBuffers();
}

void CGUI::renderPlayerStones(int player,double alpha,bool selectionbuffer)const
{
	for (int num=0;num<STONE_COUNT_ALL_SHAPES;num++)
	{
		CStone &stone=game->get_player(player)->get_stone(num);
		/* If the stone is still available but not handled by an effect */
		if (stone.get_available()>1 || (stone.get_available()==1 && (!effects || !effects->handle_player_stone(player,num))))
		{
			glPushName(2);
			glPushName(num);
			/* If the stone belongs to the current local player, take current_stone and hovered_stone
			   into account. */
			if (player==game->current_player() && game->is_local_player())
				renderPlayerStone(player,&stone,num,(num==current_stone),(num==hovered_stone),alpha,selectionbuffer);
			/* Otherwise render statically, and only when not in the selection buffer. The
			   computer's stones have no business being there. */
			else if (!selectionbuffer)renderPlayerStone(player,&stone,num,false,false,alpha,false);
			glPopName();
			glPopName();
		}
	}
	/* If a local player is to move and has selected a stone */
	if (current_stone!=-1 && player==game->current_player() && game->is_local_player())
	{
		glPushName(3);
		renderPlayerStoneArrows(player,&game->get_player(player)->get_stone(current_stone),current_stone,selectionbuffer);
		glPopName();
	}
}

void CGUI::renderPlayerStoneArrows(int player,CStone* stone,int stone_number,bool selectionbuffer)const
{
	/* Assume the stone is 0.61 of stone_size */
	const double scale=0.61;
	/* Where is the edge of the stone? */
	const double rand=stone_size*(stone->get_stone_size()+2);
	/* Animation */
	const double anim2=sin(anim*6.0)*10.0;
	const double arrow_scale=0.9;

	/* The arrow is always on top of everything else, whatever the depth buffer says */
	glDepthFunc(GL_ALWAYS);

	glPushMatrix();
	double px,py,pz;
	getPlayerStonePos(player,stone_number,&px,&py,&pz);
	glTranslated(px,py,pz);
	glRotated(-angy,0,1,0);
	glScaled(scale,scale,scale);
	glScaled(1.2,1.2,1.2);

	/* If the stone has no rotational symmetry */
	if (stone->get_rotateable()!=ROTATEABLE_NOT)
	{
		// Rotate right arrow
		glPushMatrix();
		glTranslated(rand,0,-rand);
		glRotated(-90.0-45.0,0,1,0);
		if (options.get(OPTION_ANIMATE_ARROWS))
			glRotated(-anim2,0,1,0);
		glPushName(1);
		glScaled(arrow_scale,arrow_scale,arrow_scale);
		if (!selectionbuffer)renderArrow(false,selected_arrow==1); else 
			renderQuad(1.0);
		glPopName();
		glPopMatrix();

		// Rotate left arrow
		glPushMatrix();
		glTranslated(-rand,0,-rand);
		glRotated(90+45,0,1,0);
		if (options.get(OPTION_ANIMATE_ARROWS))
			glRotated(anim2,0,1,0);
		glScaled(arrow_scale,arrow_scale,arrow_scale);
		glPushName(2);
		if (!selectionbuffer)renderArrow(false,selected_arrow==2);
		else renderQuad(1.0);
		glPopName();
		glPopMatrix();
	}

	/* If the stone has no mirror symmetry */
	if (stone->get_mirrorable()!=MIRRORABLE_NOT)
	{
		// An Y Spiegeln
		glPushMatrix();
		glRotated(angy,0,1,0);
		glTranslated(0,0,-rand);
		if (options.get(OPTION_ANIMATE_ARROWS))
			glRotated(anim2,0,0,1);
		glRotated(90.0,0,1,0);
		glScaled(arrow_scale,arrow_scale,arrow_scale);
		glPushName(3);
		if (!selectionbuffer)renderArrow(true,selected_arrow==3);
		else renderQuad(1.0);
		glPopName();
		glPopMatrix();

		// An X Spiegeln
		glPushMatrix();
		glRotated(angy,0,1,0);
		glTranslated(-rand,0,0);
		if (options.get(OPTION_ANIMATE_ARROWS))
			glRotated(anim2,1,0,0);
		glPushName(4);
		glScaled(arrow_scale,arrow_scale,arrow_scale);
		if (!selectionbuffer)renderArrow(true,selected_arrow==4);
		else renderQuad(1.0);
		glPopName();
		glPopMatrix();
	}

	/* Reset the depth function again. */
	glDepthFunc(GL_LEQUAL);
	glPopMatrix();
}

/**
 * Renders an x/z-planar quad of size size at height bevel_height
 **/
void CGUI::renderQuad(double size)const
{
	glBegin(GL_QUADS);
	glVertex3d(-size,bevel_height,+size);
	glVertex3d(+size,bevel_height,+size);
	glVertex3d(+size,bevel_height,-size);
	glVertex3d(-size,bevel_height,-size);
	glEnd();
}

/**
 * Renders an arrow at the origin in the x/z plane. doublearrow: does the arrow have 2 heads?
 * selected: is the arrow selected? Only affects the colour.
 **/
void CGUI::renderArrow(bool doublearrow,bool selected)const
{
	/* Y position of the arrow */
	const double vpos=bevel_height;
	/* Coordinates of the arrow's corners, distances, etc. Who knows */
	const double x1=0.6;
	const double x2=0.20;
	const double y1=-1.0;
	const double y2=-0.3;
	const double y3=0.3;
	/* Colour of an unselected arrow: yellow */
	const float diffuse1[]={1.0f,0.7f,0.1f,0.65f};
	/* Colour of a selected arrow: red */
	const float diffuse2[]={1.0f,0.1f,0.0f,0.85f};
	/* Set the corresponding material */
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,selected?diffuse2:diffuse1);
	/* Switch on alpha blending */
	glEnable(GL_BLEND);


	/* Draw the upper polygon, one arrow head with its shaft. */
	glBegin(GL_POLYGON);
	glNormal3d(0,1,0);
	glVertex3d(0,vpos,y1);
	glVertex3d(-x1,vpos,y2);
	glVertex3d(-x2,vpos,y2);
	glVertex3d(-x2,vpos,y3);
	glVertex3d(x2,vpos,y3);
	glVertex3d(x2,vpos,y2);
	glVertex3d(x1,vpos,y2);
	glEnd();

	if (doublearrow)
	{
		glBegin(GL_POLYGON);
 		glVertex3d(-x2,vpos,y3);
		glVertex3d(-x1,vpos,y3);
 		glVertex3d(0,vpos,y3-(y1-y2));
		glVertex3d(+x1,vpos,y3);
		glVertex3d(+x2,vpos,y3);
		glEnd();
	}
	/* Deactivate alpha blending again. */
	glDisable(GL_BLEND);
}

/**
 * Returns the position of one of a player's stones in world coordinates.
 * The result is stored in *dx, *dy and *dz.
 **/
void CGUI::getPlayerStonePos(int player,int stone,double *dx,double *dy,double *dz)const
{
	/* The radius at which the stones are arranged. This is the larger of
	   field_size_x and field_size_y, but at least 20. */
	int s=game->get_field_size_x();
	if (game->get_field_size_y()>s)s=game->get_field_size_y();
	if (s<20)s=20;

	/* Compute x, *dy and z for the first player. */
	const double x=stone_size*(5*(stone%7)-16)*1.4;
	*dy=border_bottom+bevel_height;
	const double z=stone_size*(s+5+5*(stone/7)*1.1);

	switch (player)
	{
	default:
	case 0:
		*dx=x;
		*dz=z;
		break;
	case 1:
		/* The second player swaps x and -z */
		*dx=-z;
		*dz=x;
		break;
	case 2:
		/* The third player is rotated by 180 degrees relative to the 1st. */
		*dx=-x;
		*dz=-z;
		break;
	case 3:
		/* And the 4th player is rotated by 180 degrees relative to the 2nd. */
		*dx=z;
		*dz=-x;
		break;
	}
}

void CGUI::renderPlayerStone(int player,CStone* stone,int stone_number,bool current,bool hovered,double alpha,bool selectionbuffer)const
{
	const double scale=0.6;

	int i,j;
	double px,py,pz;
	glPushMatrix();
	getPlayerStonePos(player,stone_number,&px,&py,&pz);

	/* Move to the position and scale the stones down a bit. */
	glTranslated(px,py,pz);
	glScaled(scale,scale,scale);
	if (current)glScaled(1.2,1.2,1.2);

	if (selectionbuffer)
	{
		/* When rendering into the selection buffer, treat the whole stone as one big quad.
		   It renders faster and nothing is lost. */
		const double x1=-(double)stone->get_stone_size()/2.0*stone_size*2.0;
// 		const double x2=+(double)stone->get_stone_size()/2.0*stone_size*2.0;
		const double x2=-x1;

		glBegin(GL_QUADS);
		glVertex3d(x2,0,x1);
		glVertex3d(x1,0,x1);
		glVertex3d(x1,0,x2);
		glVertex3d(x2,0,x2);
		glEnd();
	}else{
		for (i=0;i<stone->get_stone_size();i++)
		for (j=0;j<stone->get_stone_size();j++)
		if (stone->get_stone_field(j,i)!=STONE_FIELD_FREE)
		{
			glPushMatrix();
			glTranslated(+stone_size+((double)i-(double)stone->get_stone_size()/2.0)*stone_size*2.0,
				0,
			     	+stone_size+((double)j-(double)stone->get_stone_size()/2.0)*stone_size*2.0);
			if (current)
			/* If the stone is selected, animate it and render it white. */
				renderStone(-1,0.4f+((float)sin((float)anim*7.0f)*0.5f+0.5f)*0.3f,false);
			/* Otherwise it may be hovered: then do not animate (but white), otherwise render normally. */
			else renderStone(hovered?-1:player,0.6f*(float)alpha,false);
			glPopMatrix();
		}
		if (stone->get_available()>1)
		{
			char c[5];
			sprintf(c,"%dx",stone->get_available());
			glTranslated(0,1.5,+stone_size+((double)stone->get_stone_size()/2.0)*stone_size*2.0);
			/* But always let the digit face the camera. */
			glRotated(-angy,0,1,0);
			glRotated(180-angx,1,0,0);

			CGLFont f(0.5,0.8);
			f.drawText(-0.5*(strlen(c)/2),0,c);
		}
	}
	glPopMatrix();
}

void CGUI::renderGround()const
{
	if (!options.get(OPTION_ENVIRONMENT))return;
	glCallList(displayLists);
}

/**
 * Renders the board without stones, but some squares may be highlighted in colour (controlled by withgame).
 * The board is rendered as w x h.
 **/
void CGUI::renderField(bool withgame,int w,int h)const
{
	/* If withgame, the width and height should match those of the board. */
	if (withgame)w=game->get_field_size_x();
	if (withgame)h=game->get_field_size_y();
	float diffuse[]={0.43f,0.43f,0.30f,1.0f};
	float specular[]={0.27f,0.25f,0.25f,1.0f};
	float shininess[]={35.0};

	/* Outer radius */
	const double r1=stone_size;
	/* Inner radius */
	const double r2=stone_size-bevel_size;
	/* How far the frame of the board sticks out at the top */
	const double border_height=0.5;
	int i,j;
	
	glPushMatrix();
	/* Set the materials */
	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,diffuse);
	glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
	glMaterialfv(GL_FRONT,GL_SHININESS,shininess);
	
	/* Move to the top left corner of the board. */
	glTranslated(-(w-1)*r1,0,-(h-1)*r1);

	if (options.get(OPTION_ENVIRONMENT))
	{
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		/* The texture is applied in world coordinates, i.e. with a factor of 0.25
		   it repeats every 4 units. */
		glScaled(0.25,0.25,1);
		/* Rotate a little, to make it more exciting. */
		glRotated(88,0,0,1);
		glMatrixMode(GL_MODELVIEW);
		texture_field.activate();
	}
	
	for (i=0;i<w;i++)
	for (j=0;j<h;j++)
	{
		const double x=(double)i*r1*2.0;
		const double y=(double)j*r1*2.0;
		const double y1=0.0;
		const double y2=bevel_height;
		/* Orientation of the normals in space */
		static const double normal_y=0.8;
		static const double normal_x=sqrt(1.0-normal_y*normal_y);
		bool inside;

		/* If the player is local and rendering should include game data */
		if (game->is_local_player() && withgame)
		{
			/* Check whether the square belongs to the current stone's selection on the board. */
			inside=isInSelection(i,j);
			/* If the square is not part of the selection but is an allowed corner */
			if (!inside && game->get_game_field(game->current_player(),j,i)==FIELD_ALLOWED && options.get(OPTION_SHOW_HINTS))
			{
				/* Colour this square light green */
				float allowed[]={0.5f,0.65f,0.5f,1.0f};
				glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,allowed);
			}
			/* If the square is part of the selection and the selection is valid */
			if (inside && selected_allowed)
			{
				/* Colour the square in animated light green */
				const float c=(float)((sin(anim*7.0)+1.0f)/2.0f);
				float allowed[]={0.5f+c*0.2f,0.7f+c*0.25f,0.5f+c*0.2f,1.0f};
				glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,allowed);
			}
			/* If the square is part of the selection but not allowed */
			if (inside && !selected_allowed)
			{
				/* Colour the square in animated red */
				const float c=(float)((sin(anim*10.0)+1.0f)/2.0f);
				float denied[]={0.65f+c*0.3f,0.55f,0.55f,1.0f};
				glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,denied);
			}
		}
	
		glBegin(GL_QUADS);
		/* First render the underside of the square. */
		glNormal3d(0,1,0);
		glTexCoord2d(x-r2,y+r2);
		glVertex3d(x-r2,y1,y+r2);
		glTexCoord2d(x+r2,y+r2);
		glVertex3d(x+r2,y1,y+r2);
		glTexCoord2d(x+r2,y-r2);
		glVertex3d(x+r2,y1,y-r2);
		glTexCoord2d(x-r2,y-r2);
		glVertex3d(x-r2,y1,y-r2);
	
		/* Then the bevel on top */
		glNormal3d(0,normal_y,normal_x);
		glTexCoord2d(x-r2,y-r2);
		glVertex3d(x-r2,y1,y-r2);
		glTexCoord2d(x+r2,y-r2);
		glVertex3d(x+r2,y1,y-r2);
		glTexCoord2d(x+r1,y-r1);
		glVertex3d(x+r1,y2,y-r1);
		glTexCoord2d(x-r1,y-r1);
		glVertex3d(x-r1,y2,y-r1);

		/* Render the bevel at the bottom. */
		glNormal3d(0,normal_y,-normal_x);
		glTexCoord2d(x+r2,y+r2);
		glVertex3d(x+r2,y1,y+r2);
		glTexCoord2d(x-r2,y+r2);
		glVertex3d(x-r2,y1,y+r2);
		glTexCoord2d(x-r1,y+r1);
		glVertex3d(x-r1,y2,y+r1);
		glTexCoord2d(x+r1,y+r1);
		glVertex3d(x+r1,y2,y+r1);

		/* Bevel on the left */
		glNormal3d(normal_x,normal_y,0);
		glTexCoord2d(x-r2,y+r2);
		glVertex3d(x-r2,y1,y+r2);
		glTexCoord2d(x-r2,y-r2);
		glVertex3d(x-r2,y1,y-r2);
		glTexCoord2d(x-r1,y-r1);
		glVertex3d(x-r1,y2,y-r1);
		glTexCoord2d(x-r1,y+r1);
		glVertex3d(x-r1,y2,y+r1);

		/* Bevel on the right */
		glNormal3d(-normal_x,normal_y,0);
		glTexCoord2d(x+r2,y-r2);
		glVertex3d(x+r2,y1,y-r2);
		glTexCoord2d(x+r2,y+r2);
		glVertex3d(x+r2,y1,y+r2);
		glTexCoord2d(x+r1,y+r1);
		glVertex3d(x+r1,y2,y+r1);
		glTexCoord2d(x+r1,y-r1);
		glVertex3d(x+r1,y2,y-r1);
		glEnd();
		/* Restore the normal material in case we changed it before */
		glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,diffuse);
	}
	/* Now the 4 side walls. */
	/* First the top */
	renderCube(-r1*2.0,-r1*2.0,w*r1*2.0,-r1,border_bottom,border_height);
	/* Then bottom */
	renderCube(-r1*2.0,h*r1*2.0-r1,w*r1*2.0,h*r1*2.0,border_bottom,border_height);
	/* Then left */
	renderCube(-r1*2.0,-r1,-r1,(h-1)*r1*2.0+r1,border_bottom,border_height);
	/* Then right */
	renderCube(w*r1*2.0-r1,-r1,w*r1*2.0,(h-1)*r1*2.0+r1,border_bottom,border_height);

	if (options.get(OPTION_ENVIRONMENT))
	{
		texture_field.deactivate();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

void CGUI::renderField(bool withgame)const
{
	renderField(withgame,game->get_field_size_x(),game->get_field_size_y());
}

/**
 * Overloaded function. Renders a stone of the given colour at position
 * x/y (board coordinates).
 **/
void CGUI::renderStone(int x,int y,int color,float alpha,bool selectionbuffer)const
{
	glPushMatrix();
	glTranslated(-(game->get_field_size_x()-1)*stone_size+(double)x*stone_size*2.0,
			bevel_height,
		     -(game->get_field_size_y()-1)*stone_size+(double)y*stone_size*2.0);
	
	renderStone(color,alpha,selectionbuffer);
	glPopMatrix();
}

/**
 * Renders a single stone at the origin of the coordinate system
 **/
void CGUI::renderStone(int color,float alpha,bool selectionbuffer)const
{
	const float red[]={0.75f,0.0f,0.0f,alpha};
	const float blue[]={0.0f,0.05f,0.8f,alpha};
	const float green[]={0.0f,0.75f,0.0f,alpha};
	const float yellow[]={0.75f,0.75f,0.0f,alpha};
	const float white[]={0.7f,0.7f,0.7f,alpha};
	const float specular[]={0.4f,0.4f,0.4f,1.0f};
	const float shininess[]={40.0f};

	/* When rendering to the colour buffer, choose the colour by player. */
	if (!selectionbuffer) switch(color)
	{
	    case -1:	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,white);
			glMaterialfv(GL_FRONT,GL_SPECULAR,white);
			break;
	    case 0:	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,blue);
			glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
			break;
	    case 1:	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,yellow);
			glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
			break;
	    case 2:	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,red);
			glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
			break;
	    case 3:	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,green);
			glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
			break;
	    default:	return;
	}
	glMaterialfv(GL_FRONT,GL_SHININESS,shininess);

	/* If transparent enough, enable alpha blending */
	if (alpha<0.95)
	{
    		glEnable(GL_BLEND);
    		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}

	/* For the upper and lower half */
	for (int i=0;i<2;i++)
	{
		glCallList(displayLists+1);
		/* When rendering into the selection buffer we only need the top side */
		if (selectionbuffer)
			return;
		/* And rotate by 180 degrees and render the underside. */
		glRotated(180.0,1,0,0);
	}
	/* Deactivate alpha blending */
	if (alpha<0.95)glDisable(GL_BLEND);
}

/**
 * Renders a single stone, but extrudes it in direction dir.
 * If this is the direction of the light ray, we get a neat
 * shadow volume for volumetric shadows.
 **/
void CGUI::renderStoneShadow(float dir[3])const
{
	const double r1=stone_size;
	/* Extrude the shadow this far. More makes no sense. */
	const double infinity=50.0;
	/* Where to start the shadow: the middle of the stone. */
	const double yoffs=0;
	
	/* Remember the winding in which faces are rendered. */
	int ffc;
	glGetIntegerv(GL_FRONT_FACE,&ffc);
	/* If our stone is upside down, i.e. light comes from below. */
	if (dir[1]>0.0)
	{
		/* Flip the faces, since our shadow volume is "turned inside out". */
		if (ffc==GL_CCW)glFrontFace(GL_CW); else glFrontFace(GL_CCW);
	}
	/* The colour does not really matter, since we are probably rendering into the stencil buffer. */
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);

	/* Render the top. */
	glVertex3d(-r1,yoffs,+r1);
	glVertex3d(+r1,yoffs,r1);
	glVertex3d(+r1,yoffs,-r1);
	glVertex3d(-r1,yoffs,-r1);
	
	/* Render the 4 side walls. */
	glVertex3d(-r1,yoffs,-r1);
	glVertex3d(+r1,yoffs,-r1);
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);

	glVertex3d(+r1,yoffs,+r1);
	glVertex3d(-r1,yoffs,+r1);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);

	glVertex3d(-r1,yoffs,+r1);
	glVertex3d(-r1,yoffs,-r1);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);

	glVertex3d(+r1,yoffs,-r1);
	glVertex3d(+r1,yoffs,+r1);
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);

	/* And render the bottom. */
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);
	glEnd();
	/* Restore the original face order. */
	glFrontFace(ffc);
}

/**
 * Renders a (textured) cube with the given coordinates.
 * Texture coordinates are world coordinates.
 */
void CGUI::renderCube(double x1,double z1,double x2,double z2,double y1,double y2)const
{
	glBegin(GL_QUADS);

	/* Top */
	glNormal3d(0,1.0,0);
	glTexCoord2d(x2,z1);
	glVertex3d(x2,y2,z1);
	glTexCoord2d(x1,z1);
	glVertex3d(x1,y2,z1);
	glTexCoord2d(x1,z2);
	glVertex3d(x1,y2,z2);
	glTexCoord2d(x2,z2);
	glVertex3d(x2,y2,z2);

	/* Left */
	glNormal3d(-1,0,0);
	glTexCoord2d(x2-(y1-y2),z2);
	glVertex3d(x2,y1,z2);
	glTexCoord2d(x2-(y1-y2),z1);
	glVertex3d(x2,y1,z1);
	glTexCoord2d(x2,z1);
	glVertex3d(x2,y2,z1);
	glTexCoord2d(x2,z2);
	glVertex3d(x2,y2,z2);

	/* Right */
	glNormal3d(1,0,0);
	glTexCoord2d(x1+(y2-y1),z1);
	glVertex3d(x1,y1,z1);
	glTexCoord2d(x1+(y2-y1),z2);
	glVertex3d(x1,y1,z2);
	glTexCoord2d(x1,z2);
	glVertex3d(x1,y2,z2);
	glTexCoord2d(x1,z1);
	glVertex3d(x1,y2,z1);

	/* Front */
	glNormal3d(0,0,1);
	glTexCoord2d(x1,z2-(y1-y2));
	glVertex3d(x1,y1,z2);
	glTexCoord2d(x2,z2-(y1-y2));
	glVertex3d(x2,y1,z2);
	glTexCoord2d(x2,z2);
	glVertex3d(x2,y2,z2);
	glTexCoord2d(x1,z2);
	glVertex3d(x1,y2,z2);

	/* Back */
	glNormal3d(0,0,-1);
	glTexCoord2d(x2,z1-(y1-y2));
	glVertex3d(x2,y1,z1);
	glTexCoord2d(x1,z1-(y1-y2));
	glVertex3d(x1,y1,z1);
	glTexCoord2d(x1,z1);
	glVertex3d(x1,y2,z1);
	glTexCoord2d(x2,z1);
	glVertex3d(x2,y2,z1);
	glEnd();
}

/**
 * true if square x/y on the board belongs to the current stone's selection on the
 * board. Otherwise false.
 **/
bool CGUI::isInSelection(int x,int y)const
{
	/* If no stone or no square on the board is selected, false */
	if (current_stone==-1)return false;
	if (selectedx==-1 || selectedy==-1)return false;

	CStone &stone=game->get_current_player()->get_stone(current_stone);
	/* Convert x/y into coordinates relative to the stone. */
	y=y-selectedy+stone.get_stone_size()/2;
	x=x-selectedx+stone.get_stone_size()/2;
	/* If outside the stone, it is outside in any case. */
	if (x<0 || y<0 || x>=stone.get_stone_size() || y>=stone.get_stone_size())return false;

	/* Otherwise true if the stone actually has a square there */
	if (stone.get_stone_field(y,x)!=STONE_FIELD_FREE)return true;
	return false;
}

/**
 * Add a message from [client] to the chat box (-1 for a server message)
 **/
void CGUI::addChatMessage(int client,const char* text)
{
	chatbox->addChatMessage(client,text);
}

/**
 * Make the chat box sticky. If true, it does not hide automatically after 7 s.
 **/
void CGUI::setChatBoxSticky(bool sticky)
{
	chatbox->setSticky(sticky);
}

/**
 * The game client signals here that a new player is to move. Please update the
 * GUI accordingly.
 **/
void CGUI::newCurrentPlayer(int player)
{
	if (player==-1)
	{
		/* No player is active, e.g. game over, a pause or whatever. */
		playerframe->setAlpha(0.0);
		playertext->setText("");
		/* Disable buttons that should not be used now. */
		hint->setEnabled(false);
		undo->setEnabled(false);
		/* Without a current player there is nothing to select; the rendering and event code
		   dereference game->get_current_player() whenever current_stone is set. */
		current_stone=-1;
		selectedx=selectedy=-1;
	}else{
		char text[500];
		/* Set the frame's transparency depending on whether the player is local
		   or not. */
		playerframe->setAlpha(game->is_local_player()?0.75:0.20);
		switch (player)
		{
		default:
		case 0:playerframe->setColor(0,0.3,1.0);
			break;
		case 1:playerframe->setColor(1.0,1.0,0);
			break;
		case 2:playerframe->setColor(1.0,0.1,0.1);
			break;
		case 3:playerframe->setColor(0,1.0,0);
			break;
		}
		if (game->is_local_player())
		{
			sprintf(text,"%s, it's your turn!",COLOR_NAME[player]);
			/* Hint should only be enabled if only one client, or only one
			   local player, is playing. */
			bool b=game->getServerStatus()->clients<=1;
			b=b||(game->getServerStatus()->player<=1);
			/* Enable the buttons accordingly. */
			hint->setEnabled(true);
			undo->setEnabled(b);
			testSelection(lastx,lasty);
		}else{
			sprintf(text,"Waiting for %s...",COLOR_NAME[player]);
			hint->setEnabled(false);
			undo->setEnabled(false);
		}
		playertext->setText(text);
	}
}

void CGUI::showHint()
{
	CPlayer *p=game->get_current_player();
	/* Only show it if a local player is actually to move right now. */
	if (p && game->is_local_player() && hint->isEnabled())
	{
		/* Let the AI suggest a move. */
#if (1)
		NET_REQUEST_HINT data;
		data.player=game->current_player();
		game->send_message((NET_HEADER*)&data,sizeof(data),MSG_REQUEST_HINT);
		hint->setEnabled(false);
#else
		CTurn *t=board->get_ai_turn(board->current_player(),AI_HARD);
		/* Do not let the animation advance, otherwise there are jumps. */
		timer.reset(); 
		if (t)
		{
			/* Show an effect for the move, blinking gently. */
			effects->add(new CStoneFadeEffect(this,t,board->current_player()));
			/* Even select the suggested move. */
			current_stone=t->get_stone_number();
			board->get_current_player()->get_stone(t->get_stone_number())->mirror_rotate_to(t->get_mirror_count(),t->get_rotate_count());
		}
#endif
	}
}

void CGUI::setHintEnable(bool b)
{
	hint->setEnabled(b);
	testSelection(lastx,lasty);
}
