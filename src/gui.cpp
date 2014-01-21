/**
 * gui.cpp
 * Autor: Sascha Hlusiak
 * Implementation der GUI Klasse, zustaendig fuer die Grafik und Herz des
 * Spiels
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
#include "spielserver.h"
#include "guispielclient.h"
#include "player.h"
#include "dialogs.h"
#include "intro.h"
#include "chatbox.h"
#include "gamedialogs.h"
#include "helpbox.h"



// Breite der Erhohung auf den Spielfeld
const double bevel_size=0.18;
// Der Rand des Spielfelds hat diese Hoehe
const double border_bottom=-0.6;
// Farbwerte des Richtungslichts
static float light0_ambient[] = {0.35f, 0.35f, 0.35f, 1.0f};
static float light0_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
static float light0_specular[] = {1.0,1.0,1.0, 1.0f};


/**
 * Konstruktor, alles sinnvoll initialisieren, meistens mit NULL
 * Wird in der run() Methode richtig initialisiert.
 **/
CGUI::CGUI()
{
#ifdef WIN32
	/* Winsock initialisieren, damit wir Sockets unter Win verwenden koennen */
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2,0),&wsadata);
#endif
	/* Fenster erstellen */
	window=new CWindow(this);

	/* Variablen auf Default initialisieren */
	spiel=NULL;
	selectedx=selectedy=-1;
	selected_allowed=false;
	current_stone=-1;
	hovered_stone=-1;
	selected_arrow=0;
	anim=0.0;
	effects=NULL;
	menu=NULL;
	chatbox=NULL;
	font=NULL;
	intro=NULL;

	startupParams.remotehost=NULL;
	startupParams.remoteport=TCP_PORT;
	startupParams.firststart=true;
	startupParams.intro=true;
	startupParams.humans=1;
	startupParams.threads=1;


	/* Kamera initialisieren */
	angx=60.0;
	angy=0.0;
	camera_distance=30.0;

	// Optionen mit sinnvollen Voreinstellungen belegen, Voreinstellung ist 1!!
	options.set(OPTION_SHOW_FPS,0);
}

/**
 * Destruktor, alles aufraeumen, was dynamisch erzeugt wurde
 **/
CGUI::~CGUI()
{
	/* Dynamische Variablen sauber entfernen */
	if (intro)delete intro;
	if (window)delete window;
	if (font)delete font;
	if (effects)delete effects;
	if (spiel)delete spiel;
#ifdef WIN32
	/* Winsock sauber entladen */
	WSACleanup();
#endif
}

/**
 * Hilfetext ausgeben
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
 * Kommandozeile parsen
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
 * Startet ein Singleplayerspiel, mit angegebenen
 * gamemode (Spielmodus), players (lokale Spieler), diff (Schwierigkeitsgrad),
 * width/height (breite/hoehe) und Anzahl Einer, Zweier, etc.
 * Rueckgabewert: Erfolg
 **/
bool CGUI::startSingleplayerGame(GAMEMODE gamemode,int players,int diff,int width,int height,int einer,int zweier,int dreier,int vierer,int fuenfer,int ki_threads)
{
	int r;
	const char *err;
	char path[256];
	int port;

	/* Alle vorhandenen StoneEffects loeschen (bis auf Kopf der Liste) */
	if (effects)effects->clear(); else effects=new CStoneEffect();
	/* Wir haben keinen aktuellen Spieler mehr */
	newCurrentPlayer(-1);
	/* und keinen aktuellen Stein mehr */
	current_stone=-1;
	/* Momentan existierenden Spielclient entfernen */
	if (spiel)delete spiel;

	/* Einen lokalen Server in einem sekundaerem Thread
	   mit den uebergebenen Parametern starten. Der Server laeuft an
	   Port (TCP_PORT+1) */
#ifdef WIN32
	strcpy(path, "127.0.0.1");
	port = TCP_PORT+1;
#else
	sprintf(path, P_tmpdir"/freebloks-%d", rand()%131072);
	port = 0;
#endif

	r=CSpielServer::run_server(path,port,PLAYER_MAX,diff,width,height,gamemode,einer,zweier,dreier,vierer,fuenfer,ki_threads);
	if (r!=0)
	{
		/* Bei Miserfolg Fehlerdialog ausgeben. */
		char t[512];
		sprintf(t,"Could not run local server:\n%s.",strerror(errno));
		widgets.addSubChild(new CMessageBox(270,150,"Error",t));
		/* Aber nicht abbrechen, evtl. laeuft ein Server trotzdem. */
	}

	/* Einen jungfraeulichen SpielClient erstellen */
	spiel=new CGUISpielClient(this);

	/* SpielClient zu lokalen Server connecten */
	err=spiel->Connect(path, port);
	if (port == 0)
		unlink(path);
		
	/* Timer auf 0 setzen, damit das Connect keinen Sprung in der Animation
	   bewirkt. */
	timer.reset();
	if (err!=NULL)
	{
		/* wenn Connect fehlgeschlagen ist, Fehlermeldung ausgeben. */
		char t[300];
		sprintf(t,"Could not connect to local server:\n%s.",err);
		widgets.addSubChild(new CMessageBox(300,150,"Error",t));
		return false;
	}
	/* Jetzt [players] lokale Spieler anfordern. */
	for (int i=0;i<players;i++)spiel->request_player(-1, NULL);
	/* Da das Spiel lokal ist, kann direkt losgelegt werden. Spielstart anfordern */
	spiel->request_start();
	/* Erfolg */
	return true;
}

/**
 * Ein Multiplayerspiel erstellen (hosten).
 * Parameter wie oben,
 * maxhumans gibt maximale Anzahl menschlicher Spieler an, der Rest wird von
 * Computergegnern belegt
 **/
bool CGUI::startMultiplayerGame(GAMEMODE gamemode,int maxhumans,int localplayers,int diff,int width,int height,int einer,int zweier,int dreier,int vierer,int fuenfer,int ki_threads)
{
	/* Einen Server auf dem Rechner starten. */
	int r=CSpielServer::run_server(NULL,TCP_PORT,maxhumans,diff,width,height,gamemode,einer,zweier,dreier,vierer,fuenfer,ki_threads);
	if (r!=0)
	{
		/* Wenn Fehler, dann Dialogbox ausgeben */
		char t[300];
		sprintf(t,"Could not run server:\n%s.",strerror(errno));
		widgets.addSubChild(new CMessageBox(270,150,"Error",t));
		/* Und Fehler zurueck. */
		return false;
	}

	/* Spiel beitreten, und [localplayers] lokale Spieler anfordern */
	return joinMultiplayerGame("localhost",TCP_PORT,localplayers);
}

/**
 * einem Netzwerkspiel beitreten. 
 * _host ist der Name des Servers (in Form "rechner[:port]")
 * Es werden players lokale Spieler angefordert
 **/
bool CGUI::joinMultiplayerGame(const char *host,int port,int players)
{
	const char *err;

	/* Port ist erstmal Default */
	/* Sicherstellen, dass Effekte eine leere Liste ist */
	if (effects)effects->clear();else effects=new CStoneEffect();
	/* kein aktueller Spieler, und kein aktueller Stein ausgewaehlt */
	newCurrentPlayer(-1);
	current_stone=-1;
	/* SpielClient entfernen */
	if (spiel)delete spiel;
	/* Und neuen SpielClient erstellen */
	spiel=new CGUISpielClient(this);

	/* Versuchen, zu Server zu verbinden */
	err=spiel->Connect(host,port);
	/* Timer auf 0 setzen, damit das Connect keinen Sprung in der Animation
	   verursacht */
	timer.reset();
	if (err!=NULL)
	{
		/* Bei Fehler, Fehlerdialog anzeigen */
		char t[300];
		sprintf(t,"Could not connect to remote server:\n%s.",err);
		widgets.addSubChild(new CMessageBox(300,150,"Error",t));
		/* Fehler aufgetreten */
		return false;
	}else{
		/* [players] lokale Spieler anfordern. */
		for (int i=0;i<players;i++)spiel->request_player(-1, NULL);
		/* Dann modal den StartGameDialog als Lobby anzeigen. */
		widgets.addSubChild(new CStartGameDialog(spiel,this,host));
		/* Erfolg, Verbindung steht. */
		return true;
	}
}

/**
 * Hauptmethode der GUI, wird von der main() aufgerufen
 * Kuemmert sich um die richtige Initialisierung vor dem Start der
 * Hauptschleife, und durchlaeuft diese
 **/
void CGUI::run()
{
	/* Hier das Fenster erstellen, und bei Fehler raus. */
	if (!window->createWindow())return;

	/* Einen (unverbundenen) Spielclient erstellen */
	spiel=new CGUISpielClient(this);

	/* Fenster sichtbar machen. */
	window->show();
	/* OpenGL-Status mit angenehmen Werten initialisieren. */
	initGL();

	/* Hier globales Font-Objekt erstellen. Auch wenn wirs nicht brauchen,
	   es haelt die globale Textur aktiv. */
	font=new CGLFont(1.0,2.0);
	/* Drei Texturen fuer das Feld laden. */
	texture_ground.load("ground.bmp");
	texture_wall.load("wall.bmp");
	texture_field.load("field.bmp");

	createDisplayLists();

	/* Das Menue am oberen Rand konstruieren. */
	{
		/* Einen transparenten Frame als Leiste */
		CFrame *frame=new CFrame(0,0,640,25);
		frame->setAlpha(0.70);
		widgets.add(frame);

		/* Dem frame subchilds hinzufuegen, die in gleicher Ebene
		   wie die Leiste sind. Unterfenster von subchilds sind
		   untereinander modal, aber die Leiste reagiert noch */
		frame->add(subchilds=new CWidget());

		/* Knoepfe der Leiste hinzfuegen. */
		frame->addChild(new CButton(5,3,60,19,9000,frame,"Menu"));
		frame->addChild(hint=new CButton(70,3,65,19,8,frame,"Hint"));
		frame->addChild(undo=new CButton(140,3,65,19,9,frame,"Undo"));
		frame->addChild(new CButton(575,3,60,19,2,frame,"Quit"));

		/* Frame fuer aktiven Spieler hinzufuegen und unsichtbar machen */
		frame->add(playerframe=new CFrame(310,0,250,25));
		playerframe->setAlpha(0.0);
		playerframe->add(playertext=new CStaticText(0,5,250,"",playerframe));
	}
	/* ChatBox als SubChild erstellen. Das Hauptmenu waere modal dazu. */
	subchilds->addSubChild(chatbox=new CChatBox(0,25,this));
	/* FPS zaehler an unteren Bildschirmrand hinzufuegen. Rendert sich selber
	   nur, wenn die Option dafuer aktiviert ist. */
	widgets.add(new CFPS(2,465,this));

	/* Die Introsequenz instantiieren, die zu Beginn gezeigt wird. */
	if (startupParams.intro)showIntro();
		else startFirstGame();

	/* Einen einzelnen Frame vorab mal rendern. */
	render();

	/* Dann Timer auf 0 setzen, damits fluessig losgeht */
	timer.reset();
	/* Hauptschleife. Durchlaufe so lange, wie das Hauptfenster nicht geschlossen wurde */
	while (!window->isClosed())
	{
		/* Abwechselnd immer Fensternachrichten verarbeiten. */
		window->processEvents();
		/* Spielclient Netzwerknachrichten verarbeiten lassen. */
		const char *err=spiel->poll();
		if (err!=NULL)
		{
			/* Und bei Fehler entsprechende Fehlermeldung ausgeben,
			   wahrscheinlich "Connection Lost" oder sowas. */
			char c[1000];
			sprintf(c,"Connection to server lost:\n%s.",err);
			/* absolut modalen Dialog anzeigen */
			widgets.addSubChild(new CMessageBox(300,200,"Error",c));
		}
		/* checkPoint prueft, ob Zeit fuer einen Frame vorrueber ist,
		   dann wird animiert und gerendert, ansonsten mal ~7ms geschlafen,
		   um die CPU zu entlasten. */
		if (checkPoint(options.get(OPTION_LIMITFPS)?(1.0/(double)options.get(OPTION_FPS)):0))timer.sleep(5);
	}
}

/**
 * wenn mindestens mintime sek vergangen sind, Frame rendern und animieren.
 * gib true zurueck, wenn die Zeit noch nicht reif ist, dann wird geschlafen.
 * bei false wurde gerendert, dann nicht schlafen.
 **/
bool CGUI::checkPoint(double mintime)
{
	/* Vergangene Zeit seit letztem Frame. */
	const double elapsed=timer.elapsed();
	/* Sind wir ueber mintime? */
	if (elapsed>=mintime)
	{
		/* Dann timer zuruecksetzen. */
		timer.reset();
		/* Dann den Frame rendern. */
		render();
		/* Und dann alle Objekte entsprechend Delta-t animieren */
		execute(elapsed);
		/* false zurueck: wir haben gerendert, nicht schlafen */
		return false;
	}
	/* Sonst haben wir nix gemacht, koennen in Ruhe schlafen. */
	return true;
}

/**
 * Sauber das Intro beenden. Wenn das Menu nicht auf ist, handelt es sich
 * um die allererste Intro-Sequenz. Danach direkt ein Singleplayerspiel
 * starten.
 **/
void CGUI::stopIntro()
{
	/* intro sollte schon existieren, sonst raus. */
	if (!intro)return;
	/* Intro entfernen. */
	delete intro;
	intro=NULL;
	startFirstGame();
}

/** 
 * Das erste Spiel starten
 **/
void CGUI::startFirstGame()
{
	if (!startupParams.firststart)return;
	startupParams.firststart=false;

	if (startupParams.remotehost)
		joinMultiplayerGame(startupParams.remotehost,startupParams.remoteport,startupParams.humans);
	else startSingleplayerGame(GAMEMODE_4_COLORS_4_PLAYERS,startupParams.humans,KI_MEDIUM,20,20, 1,1,1,1,1,startupParams.threads);
}


/**
 * Das Intro beginnen.
 **/
void CGUI::showIntro()
{
	/* Nur ein Intro gleichzeitig erlaubt, natuerlich */
	if (intro)return;
	/* Sonst Intro erstellen. */
	intro=new CIntro(this);
}

/**
 * Wird von CWindow aufgerufen, wenn ein Mausereignis vorliegt.
 * Hier sortieren und ggf. bearbeiten.
 **/
void CGUI::processMouseEvent(TMouseEvent *e)
{
	/* Wenn das Intro laeuft, kriegt das exklusiv die Maus */
	if (intro)
	{
		/* Bei linker Maustaste */
		if (e->press && e->button1){
			/* Das Intro beenden. */
			stopIntro();
			return;
		}
		return;
	}
	/* Wenn handleUI true zurueck gibt, existiert ein Dialog, der das
	   Behandeln durch die GUI (Stein auswaehlen, ablegen) verhindert */
	bool dialog=widgets.handleUI();
	if (dialog || e->press)
	{
		/* Wenn Dialoge die Maus exklusiv behandeln wollen, erstmal
		   Position merken */
		lastx=e->x;
		lasty=e->y;
		/* Und pruefen, wo die Maus gerade ist. */
		testSelection(e->x,e->y);
	}
	/* widgets die Maus auf jeden Fall behandeln lassen. */
	int r=widgets.processMouseEvent(e);

	switch(r)
	{
	/* Es wurde der Menu-Knopf der Leiste gedrueckt. */
	case 9000: 
	{
		/* Wenn das Menu offen ist, hier schliessen. */
		if (menu){
			menu->close(true);
			menu=NULL;
		}else{
			/* Ansonsten das MainMenu oeffnen. */
			subchilds->addSubChild(menu=new CMainMenu(this,false));
		}
		break;
	}
	/* Der "Quit" Knopf wurde betaetigt, das Spiel durch Schliessen des Fensters beenden. */
	case 2: window->close();
		break;
	/* Das MainMenu moechte das Intro zeigen. */
	case 5: showIntro();
		break;
	/* Der "Hint" Knopf wurde gedrueckt, Spielvorschlag anzeigen. */
	case 8: showHint();
		break;
	/* "Undo" Knopf. Wenn lokaler Spieler dran ist, versuchen, Spielzug rueckgaengig
	   zu machen. */
	case 9: if (spiel->is_local_player())spiel->request_undo();
		break;
	}

	/* Wenn r!=0, so wurde das Ereignis irgendwo von einem widget bearbeiten.
	   wenn dialog, so fordert ein Dialog exklusive Behandlung, egal ob es gerade
	   bearbeitet wurde, oder nicht. in beiden Faellen raus. */
	if (r!=0 || dialog)return;

	/* Sonst darf die GUI hier das Event behandeln, und Steine ablegen, etc. */

	if (e->press)
	{
		if (e->button1 && !e->button2 && !e->button3 && !e->button4 && !e->button5)
		{
			/* Wenn nur linke Maustaste gedrueckt! */
			if (hovered_stone!=-1)
			{
				/* Wenn Maus ueber einem Stein, dann diesen auswaehlen. */
				current_stone=hovered_stone;
				return;
			}else if (selectedx!=-1 && selectedy!=-1 && current_stone!=-1)
			{
				/* Wenn ein Stein aktiv ist, und ein Feld ausgewaehlt ist */
				if (selected_allowed)
				{
					/* Wenn auch noch dieses Feld als "erlaubt" gekennzeichnet wurde,
					   diesen Zug ausfuehren. */
					/* Zuerst ausgewaehlten CStone erhalten */
					CStone* stone=spiel->get_current_player()->get_stone(current_stone);
					/* Stein an selectedx/selectedy zentrieren. */
					int x=selectedx-stone->get_stone_size()/2;
					int y=selectedy-stone->get_stone_size()/2;
					/* Schonmal ne Animation starten, wenn aktiviert. */
					if (options.get(OPTION_ANIMATE_STONES))
						addEffect(new CStoneRollEffect(this,stone,current_stone,spiel->current_player(),x,y,false));
					/* Den SpielClient den Stein setzen lassen. Schickt ne Anfrage an den SpielServer. */
					if (spiel->set_stone(stone, current_stone,y,x)==FIELD_ALLOWED)
						newCurrentPlayer(spiel->current_player());
					/* Keinen Stein auswaehlen. */
					current_stone=-1;
				}
			}else if (selected_arrow>0 && current_stone!=-1)
			{
				/* Wenn ein Stein ausgewaehlt ist, UND ein Pfeil */
				/* Ausgewaehlten CStone* holen. */
				CStone* stone=spiel->get_current_player()->get_stone(current_stone);

				/* Und je nach Pfeil, den Stein drehen, rotieren, etc. */
				if (selected_arrow==1)stone->rotate_right();
				if (selected_arrow==2)stone->rotate_left();
				if (selected_arrow==3)stone->mirror_over_y();
				if (selected_arrow==4)stone->mirror_over_x();
			}
			/* Es wurde anscheinend ins Leere geklickt, keinen Stein auswaehlen. */
			else current_stone=-1;
		}
		/* Wenn rechte Maustaste gedrueckt wurde, Position merken. Wichtig fuers Loslassen spaeter */
		if (e->button2 && !e->button1 && !e->button3 && !e->button4 && !e->button5)
		{
			clickx=e->x;
			clicky=e->y;
		}
		/* Mausrad wurde gedreht. Wenn keine Maustaste dabei gedruckt ist, aktuellen Stein drehen. */
		if (e->button4 && !e->button1 && !e->button2 && !e->button3 && !e->button5)
			if (current_stone!=-1)spiel->get_current_player()->get_stone(current_stone)->rotate_left();
		if (e->button5 && !e->button1 && !e->button2 && !e->button3 && !e->button4)
			if (current_stone!=-1)spiel->get_current_player()->get_stone(current_stone)->rotate_right();

		/* Mausrad wurde gedreht, aber rechte Maustaste ist down:
		   Durch alle Steine durchscrollen. */
		if (e->button4 && !e->button1 && e->button2 && !e->button3 && !e->button5)
		{
			/* Klick auf unmoegliche Pos. setzen, damit beim Loslassen nix gespiegelt wird. */
			clickx=clicky=-1000;
			for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
			{
				/* vorherigen Stein auswaehlen */
				current_stone--;
				/* Vorne wieder hinten anfangen. */
				if (current_stone<0)current_stone=STONE_COUNT_ALL_SHAPES-1;
				/* Und wenn der Stein sogar verfuegbar ist, gluecklich sein und raus. */
				if (spiel->get_current_player()->get_stone(current_stone)->get_available())
					return;
			}
			/* Sonst keinen Stein auswaehlen, da keiner mehr da. */
			current_stone=-1;
		}
		/* Dasselbe andersrum */
		if (e->button5 && !e->button1 && e->button2 && !e->button3 && !e->button4)
		{
			clickx=clicky=-1000;
			for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
			{
				current_stone++;
				if (current_stone>=STONE_COUNT_ALL_SHAPES)current_stone=0;
				if (spiel->get_current_player()->get_stone(current_stone)->get_available())
					return;
			}
			current_stone=-1;
		}


		/* Mal gucken, wo ich mit der Maus ueberhaupt bin. */
		testSelection(e->x,e->y);

		/* Und letzte Mausposition mal merken. Man weiss ja nie. */
		lastx=e->x;
		lasty=e->y;
	}else if (e->release)
	{
		/* Wenn die rechte Maustaste losgelassen wurde. */
		if (e->button2){
			/* Und die Maus seit dem Klick hinreichend wenig bewegt wurde,
			   und ein Stein ausgewaehlt ist und der Spieler auch noch dran ist. */
			if (abs(e->x-clickx)<4 && abs(e->y-clicky)<4 && spiel->is_local_player() && current_stone!=-1)
			{
				/* Dann spiegle den aktuellen Stein um die Y-Achse des Screens. */
				CStone *s=spiel->get_current_player()->get_stone(current_stone);
				/* Das ist also abhaenig vom Blickwinkel, also angy */
				if (angy<45.0 || angy>315.0 || ((angy<45+180) && (angy>-45+180)))
					s->mirror_over_y();
				else s->mirror_over_x();
			}
		}
		/* Guckwn, wo ich mit der Maus bin */
		testSelection(e->x,e->y);
	}else if (e->move)
	{
		if (e->button2)
		{
			/* Wenn die Maus bei gedrueckter Rechter Maustaste bewegt wurde. */
			/* delta x und delta y mit angy und angx verrechnen. */
			angy+=double(e->x-lastx)*0.35;
			angx+=double(e->y-lasty)*0.30;
			while (angy<0)angy+=360.0;
			while (angy>360.0)angy-=360.0;
			/* angx beschraenken. */
			if (angx<15.0)angx=15.0;
			if (angx>90.0)angx=90.0;
		}else if (e->button3)
		{
			/* Wenns die mittlere Maustaste ist, zoomen. */
			camera_distance+=double(e->y-lasty)*0.1;
			if (camera_distance<25.0)camera_distance=25.0;
			if (camera_distance>40.0)camera_distance=40.0;
		}else testSelection(e->x,e->y);
		/* Letzte Mausposition merken. */
		lastx=e->x;
		lasty=e->y;
	}
}

/**
 * Wird von CWindow aufgerufen, wenn ein Tastendruck anliegt.
 * Hier sortieren und verarbeiten.
 **/
void CGUI::processKeyEvent(unsigned short key)
{
	if (intro)
	{
		/* Wenn Intro laeuft und Esc, Ret, oder Space gedrueckt ist, Intro beenden. */
		if (key==VK_ESCAPE || key==VK_RETURN || key==VK_SPACE)stopIntro();
		/* Sonst NIX mit der Taste machen. Raus! */
		return;
	}

	/* Wenn ein Widget die Taste bearbeiten moechte, bitte. Kuemmert uns hier nicht. */
	if (widgets.processKey(key))return;
	/* Bei Escape, das Menu erscheinen lassen, wenn noch nicht geschehen. Das Menu schliesst sich selber bei Escape. */
	if (key==VK_ESCAPE && !menu)
		subchilds->addSubChild(menu=new CMainMenu(this,false));
	else if (key==VK_F1)
		widgets.addSubChild(new CHelpBox());
	/* Bei Leertaste und ausgewaehlten Stein */
	else if (key==VK_SPACE && spiel->is_local_player() && current_stone!=-1)
	{
		/* Diesen an der Y-Achse des Screens spiegeln. */
		CStone *s=spiel->get_current_player()->get_stone(current_stone);
		if (angy<45.0 || angy>315.0 || ((angy<45+180) && (angy>-45+180)))
			s->mirror_over_y();
		else s->mirror_over_x();
	}else 
	/* Bei Return, wenn ein Spiel laeuft, die ChatBox sichtbar machen. */
		if (key==VK_RETURN && !menu && spiel->isConnected())chatbox->show();
	/* Bei 'h' einen Zugvorschlag servieren. */
	else if (key=='h')showHint();
	/* Wenn lokaler Spieler dran ist... */
	else if (spiel->is_local_player()) {
		/* Taste rauf, dann aktuelle Stein nach links drehen */
		if (key==VK_UP && current_stone!=-1)
			spiel->get_current_player()->get_stone(current_stone)->rotate_left();
		/* Taste runter, den Rummel nach rechts. */
		if (key==VK_DOWN && current_stone!=-1)
			spiel->get_current_player()->get_stone(current_stone)->rotate_right();

		/* Links und rechts -> vorherigen / naechsten Stein auswaehlen. */
		if (key==VK_LEFT)
		{
			for (int i=0;i<STONE_COUNT_ALL_SHAPES;i++)
			{
				current_stone--;
				if (current_stone<0)current_stone=STONE_COUNT_ALL_SHAPES-1;
				if (spiel->get_current_player()->get_stone(current_stone)->get_available())
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
				if (spiel->get_current_player()->get_stone(current_stone)->get_available())
					return;
			}
			/* Keinen Stein auswaehlen, da anscheinend keiner mehr verfuegbar. */
			current_stone=-1;
		}
		
	}
}

/**
 * Den OpenGL Status mit brauchbaren Voreinstellungen belegen
 **/
void CGUI::initGL()
{
	/* Wir wollen ein blauaehnliches blau als Hintergrundfarbe. Sieht man eh nur, wenn Texturen deaktiviert sind. */
	glClearColor( 0.05f, 0.10f, 0.25f, 1.0f );
	/* Stencilfarbe setzen */
	glClearStencil(0);
	glClearDepth(1);
	
	/* Z-Buffer ein. */
	glEnable(GL_DEPTH_TEST);
	/* Rueckseiten entfernen */
	glEnable(GL_CULL_FACE);
	
	/* Es werde Licht! */
	glEnable(GL_LIGHTING);
	/* OpenGL soll Normalen neu berechnen, da sonst komische Effekte beim Skalieren auftreten. */
	glEnable(GL_NORMALIZE);
	
	/* Eine direktionale Lichtquelle aktivieren. */
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0,GL_POSITION,light0_pos);
	glLightfv(GL_LIGHT0,GL_AMBIENT,light0_ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,light0_diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,light0_specular);

	displayLists=glGenLists(2);
}

/**
 * Erstellt ein paar OpenGL Kommandolisten, die spter schneller ausgefhrt werden knnen
 **/
void CGUI::createDisplayLists()
{
	// Umgebung
	glNewList(displayLists,GL_COMPILE);
	{
		/* Radius des Tischbretts */
		const double r=45.0;
		/* Wie weit die Wand entfernt ist. */
		const double wall_r=160.0;
		/* Y-Position des Bretts. Genau unterhalb des Spielfelds */
		const double y=border_bottom;
		/* Wie oft soll die Textur des Bretts wiederholt werden. */
		const double tile=5.0;
		/* Wie oft soll die Textur der Wand wiederholt werden. */
		const double wall_tile=7;
	
		/* Farbe des Bretts */
		float d[4]={0.6f,0.45f,0.25f,1.0f};
		/* Specular Highlight des Bretts */
		float s[4]={0.35f,0.3f,0.3f,1.0f};
		/* Shininess des Bretts */
		float sh[]={15.0f};
		/* Material setzen */
		glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,d);
		glMaterialfv(GL_FRONT,GL_SPECULAR,s);
		glMaterialfv(GL_FRONT,GL_SHININESS,sh);
		
		glDepthFunc(GL_ALWAYS);
		
		/* Wand wird ohne Beleuchtung gerendert. */
		glDisable(GL_LIGHTING);
		/* Wand-Textur aktivieren. */
		texture_wall.activate();
		/* 4 Seiten */
		for (int i=0;i<4;i++)
		{
			/* Die Wand ist vertikal etwas ueber 0 gerendert, da man eh nicht so weit drunter gucken kann */
			const double offs=+wall_r/2.0;
			/* 100% Textur */
			glColor3d(1,1,1);
			/* Und ein Quad an die Wand rendern. */
			glBegin(GL_QUADS);
			glNormal3d(0,0,+1);
			glTexCoord2d(0,0);			glVertex3d(-wall_r,offs-wall_r,-wall_r);
			glTexCoord2d(wall_tile,0);		glVertex3d(+wall_r,offs-wall_r,-wall_r);
			glTexCoord2d(wall_tile,wall_tile);	glVertex3d(+wall_r,offs+wall_r,-wall_r);
			glTexCoord2d(0,wall_tile);		glVertex3d(-wall_r,offs+wall_r,-wall_r);
			glEnd();
			/* Dann um 90 Grad drehen, dann kann naechste Wand gerendert werden. */
			glRotated(90.0,0,1,0);
		}
		/* Und Wand-Textur sauber deaktivieren. */
		texture_wall.deactivate();
		/* Light wieder anschalten. */
		glEnable(GL_LIGHTING);


		/* Textur des Bodens aktivieren. */
		texture_ground.activate();

		/* Boden ist ein stinknormales texturiertes Quad. Hier rendern. */
		glBegin(GL_QUADS);
		glNormal3d(0,1,0);
		glTexCoord2d(0,0);		glVertex3d(-r,y,r);
		glTexCoord2d(tile,0);		glVertex3d(r,y,r);
		glTexCoord2d(tile,tile);	glVertex3d(r,y,-r);
		glTexCoord2d(0,tile);		glVertex3d(-r,y,-r);
		glEnd();
		/* Textur brav deaktivieren. */
		texture_ground.deactivate();
		
		glDepthFunc(GL_LESS);
	}
	glEndList();

	glNewList(displayLists+1,GL_COMPILE);
	{
		/* Aeusserer Radius, und innerer Radius */
		const double r1=stone_size;
		const double r2=stone_size-bevel_size;
		/* Richtung der Normalen */
		static const double normal_y=0.7;
		static const double normal_x=sqrt(1.0-normal_y*normal_y);
		
		glBegin(GL_QUADS);

		/* Oberseite rendern. */
		glNormal3d(0,1,0);
		glVertex3d(-r2,bevel_height,+r2);
		glVertex3d(+r2,bevel_height,+r2);
		glVertex3d(+r2,bevel_height,-r2);
		glVertex3d(-r2,bevel_height,-r2);
	
		/* dann Schraege oben */
		glNormal3d(0,normal_y,-normal_x);
		glVertex3d(-r2,bevel_height,-r2);
		glVertex3d(+r2,bevel_height,-r2);
		glVertex3d(+r1,0,-r1);
		glVertex3d(-r1,0,-r1);

		/* Schraege unten */
		glNormal3d(0,normal_y,normal_x);
		glVertex3d(+r2,bevel_height,+r2);
		glVertex3d(-r2,bevel_height,+r2);
		glVertex3d(-r1,0,+r1);
		glVertex3d(+r1,0,+r1);

		/* Schraege links */
		glNormal3d(-normal_x,normal_y,0);
		glVertex3d(-r2,bevel_height,+r2);
		glVertex3d(-r2,bevel_height,-r2);
		glVertex3d(-r1,0,-r1);
		glVertex3d(-r1,0,+r1);

		/* Schraege rechts */
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
 * Alle Objekte, sonstwas, um delta t=elapsed [sek] animieren.
 **/
void CGUI::execute(double elapsed)
{
	/* Globalen Animationszaehler erhoehen. Greift z.B. bei den Pfeilen, und sonstigen Animationen. */
	anim+=elapsed;
	/* Effekte animieren. */
	if (effects)effects->execute(elapsed);
	/* Alle widgets animieren (Menue, etc. ) */
	widgets.execute(elapsed);
	/* Wenn nach dem Animieren, das menu existiert, aber entfernt werden moechte, dann auf 0 setzen. */
	/* Die Widgets existieren noch exakt einen Durchlauf, wenn sie entfernt werden wollen. */
	if (menu && menu->deleted())menu=0;
	/* Wenn Intro, und Intro sagt, dass es abgelaufen ist, dann stoppen. */
	if (intro)
		if (intro->execute(elapsed))stopIntro();
}

/**
 * Rendert die Szene um die Fensterposition x/y in den Selectionbuffer, und guckt an,
 * Ueber welchem Face der Mauszeiger gerade steht. Uebergibt das dann an die zustaendigen Programmteile.
 **/
void CGUI::testSelection(int x,int y)
{
	/* Hier kommen die "names" der Faces unter der Maus rein. 512 sollten reichen. */
	static GLuint buffer[512];
	/* Ich brauche noch die Masse des Viewports. */
	GLint viewport[4];
	/* Anzahl der getroffenen Faces */
	int hits;
	int type;
	int i,j;
	/* Bei laufendem Intro eruebrigt sich das alles. */
	if (intro)return;
	/* SelectBuffer initialisieren. */
	glSelectBuffer(sizeof(buffer)/sizeof(GLuint), buffer);
	
	/* Masse des Viewports holen. */
	glGetIntegerv(GL_VIEWPORT,viewport);
	/* Select Modus anschalten, und Namen initialisieren. */
	glRenderMode(GL_SELECT);
	glInitNames();
	
	/* Eine entsprechende Projektionmatrix aufbauen. */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	/* Wichtig: Die PickMatrix */
	gluPickMatrix(x,viewport[3]-y,3.0,3.0,viewport);
	glPushMatrix();
	gluPerspective(fov, (float)viewport[2]/(float)viewport[3], 1.0, 300.0);

	/* Ansichtsmatrix basteln. */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0,4,0);
	gluLookAt(camera_distance*sin(-angy*M_PI/180.0)*cos(angx*M_PI/180.0),
		camera_distance*sin(angx*M_PI/180.0),
		camera_distance*cos(angy*M_PI/180.0)*cos(angx*M_PI/180.0),
		0,0,0,
		0,1,0);

	/* Wenn Dialog exklusiv Maus behandeln wuerde, kann man hier manches an Renderarbeit sparen. */
	if (!widgets.handleUI())
	{
		glPushMatrix();
		/* Wenn ein Stein des Spielers aktiv ist, das Spielfeld mit Quadraten pflastern,
		   damit man rauskriegt, welches Feld der Spieler ausgewaehlt hat. */
		if (current_stone!=-1)
		{
			glTranslated(	-(spiel->get_field_size_x()-1)*stone_size,
					0,
					-(spiel->get_field_size_y()-1)*stone_size);
			/* type=1, also zeigen, dass es das Spielfeld ist. */
			glPushName(1);
			for (i=0;i<spiel->get_field_size_x();i++)
			{
				/* Zweiter Name ist X-Koordinate. */
				glPushName(i);
				for (j=0;j<spiel->get_field_size_y();j++)
				{
					/* Dritter Name ist Y */
					glPushName(j);
					renderQuad(stone_size);
					glPopName();
					/* Zu naechsten Stein in Z-Richtung gehen */
					glTranslated(0,0,stone_size*2.0);
				}
				glPopName();
				/* Und naechste Reihe beginnen. */
				glTranslated(stone_size*2.0,0,-(spiel->get_field_size_y())*stone_size*2.0);
			}
			glPopName();
		}
		glPopMatrix();
		/* Wenn lokaler Spieler, dann in SelectionBuffer auch die Steine des Spielers rendern. */
		if (spiel->is_local_player())
			renderPlayerStones(spiel->current_player(),1.0,true);
	}

	/* Widgets in jedem Fall in den SelectionBuffer rendern. */
	widgets.render(true);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	/* Matrix-Stack wieder saeubern */
	
	/* Selection-Mode beenden. hits sind die Anzahl der zutreffenden Faces */
	hits = glRenderMode(GL_RENDER);

	GLuint names, *ptr, minZ,*ptrNames=0, numberOfNames=0;
	selectedx=selectedy=-1;
	hovered_stone=-1;
	selected_arrow=0;

	/* Wenn keine Facs getroffen */
	if (hits==0)
	{
		/* Widgets mitteilen, dass die Maus ueber KEINEM Widget ist. */
		widgets.processSelection(0);
		/* Und schnell raus */
		return;
	}
	/* Durch alle Faces gehen und dasjenige, mit dem geringstem Z-Value merken. */
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
	/* Wenn keins gefunden wurde */
	if (ptrNames==0)
	{
		/* Widgets mitteilen, dass die Maus ueber KEINEM Widget ist */
		widgets.processSelection(0);
		/* Und raus. */
		return;
	}
	/* Der Typ steht im ersten Name */
	type=*(ptrNames+2);
	ptr=ptrNames+3;
	/* type==1 ist Spielfeld */
	if (type==1 && numberOfNames==3)
	{
		/* Koordinate merken, auf dem der Mauszeiger steht */
		selectedx=*(ptr);
		selectedy=*(ptr+1);

		selected_allowed=true;
		if (current_stone!=-1)
		{
			/* Gucken, ob aktueller Stein so an die Position passen wuerde. */
			CStone *stone=spiel->get_current_player()->get_stone(current_stone);
			int vx=selectedx-stone->get_stone_size()/2,vy=selectedy-stone->get_stone_size()/2;
			/* Und merken, ob der Stein passen taete. */
			selected_allowed=(spiel->is_valid_turn(stone,spiel->current_player(),vy,vx)==FIELD_ALLOWED);
		}
	}else if (type==2 && numberOfNames==2)
	{
		/* Maus ist ueber Stein des Spielers. Diesen Stein hervorheben. */
		hovered_stone=*(ptr);
	}else if (type==3 && numberOfNames==2)
	{
		/* Maus befindet sich ueber einem Pfeil. */
		selected_arrow=*(ptr);
	}

	/* Maus ist ueber einem Widget. Alle Widgets darueber in Kenntnis setzen */
	if (type==4 && numberOfNames==2)
		widgets.processSelection(*(ptr));
	/* Sonst ist die Maus ueber keinem Widget, auch stets davon in Kenntnis setzen */
	else widgets.processSelection(0);
}

/**
 * Rendert den aktuellen Zustand als Frame in das Fenster.
 **/
void CGUI::render()
{
	int i,j;
	GLint viewport[4];

	/* Buffer leeren */
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	unsigned int c=0;
	if (!options.get(OPTION_ENVIRONMENT))c|=GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT;
	if (options.get(OPTION_SHADOW) && window->canShadow() && (intro || (effects && effects->hasShadow())))c|=GL_STENCIL_BUFFER_BIT;
	if (c)glClear(c);

	/* Groesse des Viewports erhalten, und daraus eine Perspective-Matrix basteln */
	glGetIntegerv(GL_VIEWPORT,viewport);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, (float)viewport[2]/(float)viewport[3], 1.0, 300.0);

	glMatrixMode(GL_MODELVIEW);
	if (!intro)
	{
		/* Wenn das Intro _nicht_ laeuft, bauen wir uns hier eine Modelview-Matrix */
		glLoadIdentity();
		glTranslated(0,4,0);
		gluLookAt(camera_distance*sin(-angy*M_PI/180.0)*cos(angx*M_PI/180.0),
			camera_distance*sin(angx*M_PI/180.0),
			camera_distance*cos(angy*M_PI/180.0)*cos(angx*M_PI/180.0),
			0,0,0,
			0,1,0);

		/* Und setzen hier das Licht neu, damit es mit der aktuellen Position des Betrachters wieder
		   stimmt. Sonst sind z.B. Specular Hightlights falsch. */
		glLightfv(GL_LIGHT0,GL_POSITION,light0_pos);
	}

	if (intro)
	{
		/* Das Intro sorgt komplett um sich selbst, auch um die ModelView-Matrix. */
 		intro->render();
	}else {
		/* Erst Umgebung rendern, Boden, Wand */
		renderGround();
		/* Dann Spielfeld hinklatschen */
		renderField();
		/* Dann der Reihe nach alle Steine des Spielfelds rendern */
		for (i=0;i<spiel->get_field_size_x();i++)
		for (j=0;j<spiel->get_field_size_y();j++)
			if (spiel->get_game_field(j,i)!=FIELD_FREE)
		{
			/* aber nur rendern, wenn nicht durch einen Effekt behandelt. In dem Falle rendert
			   der Effekt diesen Stein. */
			if (!effects || !effects->is_effected(i,j))
				renderStone(i,j,spiel->get_game_field(j,i),0.65f,false);
		}
		/* Dann Effekte rendern */
		if (effects)effects->render();
		/* Und die Steine aller 4 Spieler. */
		if (spiel) {
			for (int i=0;i<4;i++)
			{
				renderPlayerStones(i,(spiel->isConnected() && spiel->current_player()!=-1)?1.0:0.3,false);
			}
		}
	}

	/* Wenn Schatten an sind, und das Intro oder Effekte Schatten wollen, dann hier Schatten rendern! */
	if (options.get(OPTION_SHADOW) && window->canShadow() && (intro || (effects && effects->hasShadow())))
	{
		glPushAttrib( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_STENCIL_BUFFER_BIT );

		/* Nicht in Depth-Buffer malen, aber ihn immer noch abfragen */
		glDepthMask( GL_FALSE );
		glDepthFunc( GL_LEQUAL );
		/* Immer in Stencil-Buffer zeichnen */
		glEnable( GL_STENCIL_TEST );
		glStencilFunc( GL_ALWAYS, 1, 0xFFFFFFFFL );
		/* Bloss nicht in Color-Buffer zeichnen */
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		
		/* Zuerst nur Vorderseiten der Shadow-Volumes zeichnen, dabei Stencil-Wert erhoehen. */
		glFrontFace( GL_CCW );
		glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
		if (intro)intro->renderShadow(); else if (effects)effects->renderShadow();

		/* Dann nur Rueckseiten der Shadow-Volumes rendern, dabei Stencil dekrementieren. */
		glFrontFace( GL_CW );
		glStencilOp( GL_KEEP, GL_KEEP, GL_DECR );
		if (intro)intro->renderShadow(); else if (effects)effects->renderShadow();

		/* Wieder zuruecksetzen, also Vorderseiten rendern, in Colorbuffer, etc. */
		glFrontFace( GL_CCW );
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		
		/* Alpha-Blending an, Licht aus, wir malen die matt-shadow-plane */
		glEnable( GL_BLEND );
		glDisable(GL_LIGHTING);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		/* Rendern, wenn im Stencil-Buffer keine 0 drin steht */
		glStencilFunc( GL_NOTEQUAL, 0, 0xFFFFFFFFL );
		glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

		/* Stinknormale Modelview und Ortho-Projection Matrix bauen */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0,1,0,1,0,1);

		/* Schwarzes, stark transparentes Quad rendern. */
		glColor4f( 0.0f, 0.0f, 0.0f, 0.25f );
		glBegin( GL_TRIANGLE_STRIP );
		glVertex3f(-1,1,0);
		glVertex3f(-1,-1,0);
		glVertex3f(1,1,0);
		glVertex3f(1,-1,0);
		glEnd();

		/* Matrix-Stack und Attribute wiederherstellen. */
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopAttrib();
	}

	/* Wenn kein Intro laeuft, rendern wir mal die Widgets. */
	if (!intro)
	{
		widgets.render(false);
	}

#ifndef WIN32
	GLenum error=glGetError();
	if (error!=GL_NO_ERROR)printf("GL Error %d\n",error);
#endif


	/* Fertig mit Rendern. Jetzt tauschen wir Front und Back-Buffer, um das Werk anzuzeigen */
	window->swapBuffers();
}

/**
 * rendert alle Steine des angegebenen Spielers mit den angegebenen alpha Wert.
 **/
void CGUI::renderPlayerStones(int player,double alpha,bool selectionbuffer)const
{
	/* Durch alle Steinformen iterieren */
	for (int num=0;num<STONE_COUNT_ALL_SHAPES;num++)
	{
		CStone *stone=spiel->get_player(player)->get_stone(num);
		/* Wenn der Stein noch vorhanden ist, aber nicht von einem Effekt behandelt */
		if (stone->get_available()>1 || (stone->get_available()==1 && (!effects || !effects->handle_player_stone(player,num))))
		{
			glPushName(2);
			glPushName(num);
			/* Wenn der Stein zum aktuellem lokalen Spieler gehoert, current_stone und hovered_stone
			   mit einbeziehen. */
			if (player==spiel->current_player() && spiel->is_local_player())
				renderPlayerStone(player,stone,num,(num==current_stone),(num==hovered_stone),alpha,selectionbuffer);
			/* Sonst statisch rendern, und auch nur, wenn nicht in selectionBuffer. Da haben
			   Steine vom Computer nix verloren. */
			else if (!selectionbuffer)renderPlayerStone(player,stone,num,false,false,alpha,false);
			glPopName();
			glPopName();
		}
	}
	/* Wenn ein Spieler lokal dran ist und einen Stein ausgewaehlt hat */
	if (current_stone!=-1 && player==spiel->current_player() && spiel->is_local_player())
	{
		/* Dann rendere die Pfeile um diesen Stein. */
		glPushName(3);
		renderPlayerStoneArrows(player,spiel->get_player(player)->get_stone(current_stone),current_stone,selectionbuffer);
		glPopName();
	}
}

/**
 * Rendert die Pfeile um einen bestimmten Stein, des angegebenen Spielers.
 **/
void CGUI::renderPlayerStoneArrows(int player,CStone* stone,int stone_number,bool selectionbuffer)const
{
	/* Nehme an, dass Stein 0.61 von stone_size gross ist */
	const double scale=0.61;
	/* Wo ist der Rand des Steins? */
	const double rand=stone_size*(stone->get_stone_size()+2);
	/* Animation */
	const double anim2=sin(anim*6.0)*10.0;
	/* Und Groesse des Pfeils einfach angeben */
	const double arrow_scale=0.9;

	/* Der Pfeil liegt immer ueber allen anderen, egal was der DepthBuffer sagt */
	glDepthFunc(GL_ALWAYS);

	glPushMatrix();
	double px,py,pz;
	/* An Position des Steins bewegen */
	getPlayerStonePos(player,stone_number,&px,&py,&pz);
	glTranslated(px,py,pz);
	/* Mit rotieren lassen */
	glRotated(-angy,0,1,0);
	/* Und Groesse anpassen. */
	glScaled(scale,scale,scale);
	glScaled(1.2,1.2,1.2);

	/* Wenn Stein keine Rotationssymmetrie hat */
	if (stone->get_rotateable()!=ROTATEABLE_NOT)
	{
		// Nach rechts-drehen-Pfeil
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

		// Nach links-drehen-Pfeil
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

	/* Wenn Stein keine Spiegel-symmetrie hat */
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

	/* DepthFunc wieder zuruecksetzen. */
	glDepthFunc(GL_LEQUAL);
	glPopMatrix();
}

/** 
 * Rendert ein x/z-planares Viereck der Groesse size an Hoehe bevel_height
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
 * Rendert einen Pfeil an den Ursprung in die x/z-Ebene. doublearrow: Hat der Pfeil 2 Spitzen?
 * selected: Ist der Pfeil ausgewaehlt? Beeinflusst nur Farbe.
 **/
void CGUI::renderArrow(bool doublearrow,bool selected)const
{
	/* Y-Position des Pfeils */
	const double vpos=bevel_height;
	/* Koordinaten der Ecken des Pfeils, Abstaende, etc. Was weiss ich */
	const double x1=0.6;
	const double x2=0.20;
	const double y1=-1.0;
	const double y2=-0.3;
	const double y3=0.3;
	/* Farbe fuer nicht ausgewaehlten Pfeil: Gelb */
	const float diffuse1[]={1.0f,0.7f,0.1f,0.65f};
	/* Fare fuer ausgewaehlten Pfeil: Rot */
	const float diffuse2[]={1.0f,0.1f,0.0f,0.85f};
	/* Entsprechendes Material setzen */
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,selected?diffuse2:diffuse1);
	/* Alpha-Blending anschalten */
	glEnable(GL_BLEND);


	/* Das obere Polygon zeichnen, eine Spitze des Pfeils mit Rumpf. */
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

	/* Andere Spitze auch noch hinterher rendern, wenn erwuenscht */
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
	/* Alpha-Blending wieder deaktivieren. */
	glDisable(GL_BLEND);
}

/**
 * Gibt die Position eines Steins eines Spielers in Weltkoordinaten zurueck.
 * Rueckgabewert wird in *dx, *dy und *dz gespeichert.
 **/
void CGUI::getPlayerStonePos(int player,int stone,double *dx,double *dy,double *dz)const
{
	/* Der Radius, in dem die Steine angeordnet werden. Das ist der groessere Wert von
	   field_size_x und field_size_y, mindestens aber 20. */
	int s=spiel->get_field_size_x();
	if (spiel->get_field_size_y()>s)s=spiel->get_field_size_y();
	if (s<20)s=20;

	/* x, *dy und z fuer den ersten Spieler errechnen. */
	const double x=stone_size*(5*(stone%7)-16)*1.4;
	*dy=border_bottom+bevel_height;
	const double z=stone_size*(s+5+5*(stone/7)*1.1);

	switch (player)
	{
	default:
	case 0:
		/* Fuer den ersten Spieler haben wir das errechnet. */
		*dx=x;
		*dz=z;
		break;
	case 1:
		/* Der zweite Spieler vertauscht x und -z */
		*dx=-z;
		*dz=x;
		break;
	case 2:
		/* Dritte Spieler ist um 180 Grad gedreht zum 1. */
		*dx=-x;
		*dz=-z;
		break;
	case 3:
		/* Und der 4. Spieler ist um 180 Grad gedreht zum 2. */
		*dx=z;
		*dz=-x;
		break;
	}
}

/**
 * Rendert einen Stein eines Spielers an die errechnete Position des Steins.
 **/
void CGUI::renderPlayerStone(int player,CStone* stone,int stone_number,bool current,bool hovered,double alpha,bool selectionbuffer)const
{
	const double scale=0.6;

	int i,j;
	double px,py,pz;
	glPushMatrix();
	/* Position zu dem Stein in Weltkoordinaten holen */
	getPlayerStonePos(player,stone_number,&px,&py,&pz);

	/* An Position begeben, und Steine was kleiner skalieren. */
	glTranslated(px,py,pz);
	glScaled(scale,scale,scale);
	/* den aktuellen Stein aber ein wenig groesser rendern. */
	if (current)glScaled(1.2,1.2,1.2);

	if (selectionbuffer)
	{
		/* Beim Rendern in den SelectionBuffer, den kompletten Stein als grosses Quad auffassen.
		   geht schneller zu Rendern und man verliert nix dabei. */
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
		/* Sonst durch alle kleinen Steinchen des Felds iterieren */
		for (i=0;i<stone->get_stone_size();i++)
		for (j=0;j<stone->get_stone_size();j++)
		if (stone->get_stone_field(j,i)!=STONE_FIELD_FREE)
		{
			/* An Position springen und Stein rendern. */
			glPushMatrix();
			glTranslated(+stone_size+((double)i-(double)stone->get_stone_size()/2.0)*stone_size*2.0,
				0,
			     	+stone_size+((double)j-(double)stone->get_stone_size()/2.0)*stone_size*2.0);
			if (current)
			/* Wenn Stein ausgewaehlt, animieren und in weiss rendern. */
				renderStone(-1,0.4f+((float)sin((float)anim*7.0f)*0.5f+0.5f)*0.3f,false);
			/* Sonst ist er evtl. hovered, dann nicht animieren (aber weiss) ansonsten ganz normal rendern. */
			else renderStone(hovered?-1:player,0.6f*(float)alpha,false);
			glPopMatrix();
		}
		/* Wenn Stein mehrfach verfuegbar ist. */
		if (stone->get_available()>1)
		{
			/* Kleine Ziffer mit Anzahl dazu rendern */
			char c[5];
			sprintf(c,"%dx",stone->get_available());
			glTranslated(0,1.5,+stone_size+((double)stone->get_stone_size()/2.0)*stone_size*2.0);
			/* Ziffer aber immer in Kamera blicken lassen. */
			glRotated(-angy,0,1,0);
			glRotated(180-angx,1,0,0);

			/* Und string nun rendern. */
			CGLFont f(0.5,0.8);
			f.drawText(-0.5*(strlen(c)/2),0,c);
		}
	}
	glPopMatrix();
}

/**
 * Den texturierten Untergrund und die Wand rendern.
 **/
void CGUI::renderGround()const
{
	/* Aber nur, wenn die Option entsprechend gesetzt ist. */
	if (!options.get(OPTION_ENVIRONMENT))return;
	glCallList(displayLists);
}

/**
 * Rendert das Spielfeld, ohne Steine, aber ggf. werden manche Quadrate farblich markiert (steuert withgame).
 * Das Feld wird in w x h gerendert.
 **/
void CGUI::renderField(bool withgame,int w,int h)const
{
	/* Wenn withgame, dann sollte auch Breite und Hoehe mit der des Felds uebereinstimmen. */
	if (withgame)w=spiel->get_field_size_x();
	if (withgame)h=spiel->get_field_size_y();
	/* Farbwerte des Felds. */
	float diffuse[]={0.43f,0.43f,0.30f,1.0f};
	float specular[]={0.27f,0.25f,0.25f,1.0f};
	float shininess[]={35.0};

	/* Aeusserer Radius */
	const double r1=stone_size;
	/* Innerer Radius */
	const double r2=stone_size-bevel_size;
	/* Wie weit der Rahmen des Felds nach oben rausweicht */
	const double border_height=0.5;
	int i,j;
	
	glPushMatrix();
	/* Materialien setzen */
	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,diffuse);
	glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
	glMaterialfv(GL_FRONT,GL_SHININESS,shininess);
	
	/* An linke obere Ecke des Felds wandern. */
	glTranslated(-(w-1)*r1,0,-(h-1)*r1);

	if (options.get(OPTION_ENVIRONMENT))
	{
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		/* Textur wird in Weltkoordinaten aufgetragen, d.h. bei 0.25 als Faktor 
		   wiederholt sie sich alle 4 Einheiten. */
		glScaled(0.25,0.25,1);
		/* Bissel drehen, ums spannender zu machen. */
		glRotated(88,0,0,1);
		glMatrixMode(GL_MODELVIEW);
		/* Textur fuer das Feld aktivieren und Texture-Matrix richtig setzen. */
		texture_field.activate();
	}
	
	/* Jetzt ueber alle Feldsteine iterieren. */
	for (i=0;i<w;i++)
	for (j=0;j<h;j++)
	{
		/* Position des Steins errechnen. */
		const double x=(double)i*r1*2.0;
		const double y=(double)j*r1*2.0;
		const double y1=0.0;
		const double y2=bevel_height;
		/* Lage der Normalen im Raum */
		static const double normal_y=0.8;
		static const double normal_x=sqrt(1.0-normal_y*normal_y);
		bool inside;

		/* Wenn Spieler lokal ist, und mit Spieldaten gerendert werden soll */
		if (spiel->is_local_player() && withgame)
		{
			/* Gucke, ob das Feld zur Auswahl des aktuellen Steins auf dem Feld gehoert. */
			inside=isInSelection(i,j);
			/* Wenn das Feld nicht zur Auswahl gehoert, aber es eine erlaubte Ecke ist */
			if (!inside && spiel->get_game_field(spiel->current_player(),j,i)==FIELD_ALLOWED && options.get(OPTION_SHOW_HINTS))
			{
				/* So faerbe dieses Feld in hellem Gruen */
				float allowed[]={0.5f,0.65f,0.5f,1.0f};
				glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,allowed);
			}
			/* Wenn das Feld zur Auswahl gehoert, und die Auswahl gueltig ist */
			if (inside && selected_allowed)
			{
				/* Faerbe das Feld in animiertes helles Gruen */
				const float c=(float)((sin(anim*7.0)+1.0f)/2.0f);
				float allowed[]={0.5f+c*0.2f,0.7f+c*0.25f,0.5f+c*0.2f,1.0f};
				glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,allowed);
			}
			/* Wenn das Feld zur Auswahl gehoert, aber nicht erlaubt ist */
			if (inside && !selected_allowed)
			{
				/* So faerbe das Feld in animiertes Rot */
				const float c=(float)((sin(anim*10.0)+1.0f)/2.0f);
				float denied[]={0.65f+c*0.3f,0.55f,0.55f,1.0f};
				glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,denied);
			}
		}
	
		glBegin(GL_QUADS);
		/* Zuerst Unterseite des Felds rendern. */
		glNormal3d(0,1,0);
		glTexCoord2d(x-r2,y+r2);
		glVertex3d(x-r2,y1,y+r2);
		glTexCoord2d(x+r2,y+r2);
		glVertex3d(x+r2,y1,y+r2);
		glTexCoord2d(x+r2,y-r2);
		glVertex3d(x+r2,y1,y-r2);
		glTexCoord2d(x-r2,y-r2);
		glVertex3d(x-r2,y1,y-r2);
	
		/* Dann Schraege oben */
		glNormal3d(0,normal_y,normal_x);
		glTexCoord2d(x-r2,y-r2);
		glVertex3d(x-r2,y1,y-r2);
		glTexCoord2d(x+r2,y-r2);
		glVertex3d(x+r2,y1,y-r2);
		glTexCoord2d(x+r1,y-r1);
		glVertex3d(x+r1,y2,y-r1);
		glTexCoord2d(x-r1,y-r1);
		glVertex3d(x-r1,y2,y-r1);

		/* Schraege unten rendern. */
		glNormal3d(0,normal_y,-normal_x);
		glTexCoord2d(x+r2,y+r2);
		glVertex3d(x+r2,y1,y+r2);
		glTexCoord2d(x-r2,y+r2);
		glVertex3d(x-r2,y1,y+r2);
		glTexCoord2d(x-r1,y+r1);
		glVertex3d(x-r1,y2,y+r1);
		glTexCoord2d(x+r1,y+r1);
		glVertex3d(x+r1,y2,y+r1);

		/* Schraege links */
		glNormal3d(normal_x,normal_y,0);
		glTexCoord2d(x-r2,y+r2);
		glVertex3d(x-r2,y1,y+r2);
		glTexCoord2d(x-r2,y-r2);
		glVertex3d(x-r2,y1,y-r2);
		glTexCoord2d(x-r1,y-r1);
		glVertex3d(x-r1,y2,y-r1);
		glTexCoord2d(x-r1,y+r1);
		glVertex3d(x-r1,y2,y+r1);

		/* Schraege rechts */
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
		/* Wieder normales Material setzen, falls wir es vorher umgebogen haben */
		glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,diffuse);
	}
	/* Jetzt sind die 4 Seitenwaende dran. */
	/* Zuerst oben */
	renderCube(-r1*2.0,-r1*2.0,w*r1*2.0,-r1,border_bottom,border_height);
	/* Dann unten */
	renderCube(-r1*2.0,h*r1*2.0-r1,w*r1*2.0,h*r1*2.0,border_bottom,border_height);
	/* Dann links */
	renderCube(-r1*2.0,-r1,-r1,(h-1)*r1*2.0+r1,border_bottom,border_height);
	/* Dann rechts */
	renderCube(w*r1*2.0-r1,-r1,w*r1*2.0,(h-1)*r1*2.0+r1,border_bottom,border_height);

	/* Und ggf. Textur wieder entladen und zuruecksetzen */
	if (options.get(OPTION_ENVIRONMENT))
	{
		texture_field.deactivate();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
	glPopMatrix();
}

/**
 * Ueberladene Funktion zum Rendern des Spielfelds ohne Angabe von w/h
 **/
void CGUI::renderField(bool withgame)const
{
	renderField(withgame,spiel->get_field_size_x(),spiel->get_field_size_y());
}

/**
 * Ueberladene Funktion. Rendert einen Stein der angegebenen Farbe an Position
 * x/y (Spielfeldkoordinaten).
 **/
void CGUI::renderStone(int x,int y,int color,float alpha,bool selectionbuffer)const
{
	glPushMatrix();
	/* An Position auf dem Spielfeld bewegen */
	glTranslated(-(spiel->get_field_size_x()-1)*stone_size+(double)x*stone_size*2.0,
			bevel_height,
		     -(spiel->get_field_size_y()-1)*stone_size+(double)y*stone_size*2.0);
	
	/* Stein dort rendern. */
	renderStone(color,alpha,selectionbuffer);
	glPopMatrix();
}

/**
 * Rendert einen Single-Stone an den Ursprung des Koordinatensystems
 **/
void CGUI::renderStone(int color,float alpha,bool selectionbuffer)const
{
	/* Ein paar Farben vordefinieren */
	const float red[]={0.75f,0.0f,0.0f,alpha};
	const float blue[]={0.0f,0.05f,0.8f,alpha};
	const float green[]={0.0f,0.75f,0.0f,alpha};
	const float yellow[]={0.75f,0.75f,0.0f,alpha};
	const float white[]={0.7f,0.7f,0.7f,alpha};
	/* Farbe des Glanzlichts */
	const float specular[]={0.4f,0.4f,0.4f,1.0f};
	/* Und shininess */
	const float shininess[]={40.0f};

	/* Wenn in Color-Buffer, dann waehle Farbe anhand des Spielers aus. */
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

	/* Wenn hinreichend transparent, Alpha-Blending aktivieren */
	if (alpha<0.95)
	{
    		glEnable(GL_BLEND);
    		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}

	/* Fuer die obere und untere Haelfte */
	for (int i=0;i<2;i++)
	{
		glCallList(displayLists+1);
		/* Wenn wir in SelectionBuffer rendern, brauchen wir nur Oberseite */
		if (selectionbuffer)
			return;
		/* Und um 180 Grad drehen und Unterseite rendern. */
		glRotated(180.0,1,0,0);
	}
	/* Alpha-Blending deaktivieren */
	if (alpha<0.95)glDisable(GL_BLEND);
}

/**
 * rendert einen Singlestone, aber extrudiert ihn in Richtung von dir.
 * Wenn dies die Roichtung des Lichtstrahls ist, erhalten wir ein feines
 * Shadow-Volume, fuer volumetrischen Schatten.
 **/
void CGUI::renderStoneShadow(float dir[3])const
{
	const double r1=stone_size;
	/* Den Schatten soo weit extrudieren. Mehr macht keinen Sinn. */
	const double infinity=50.0;
	/* Wo mit dem Schatten anfangen: Mitte des Steins. */
	const double yoffs=0;
	
	/* Merken, wie rum Faces gerendert werden. */
	int ffc;
	glGetIntegerv(GL_FRONT_FACE,&ffc);
	/* Wenn unser Stein auf dem Kopf steht, also Licht quasi von unten. */
	if (dir[1]>0.0)
	{
		/* Drehen wir die Faces um, da unser Shadow-Volume "umgekrempelt" ist. */
		if (ffc==GL_CCW)glFrontFace(GL_CW); else glFrontFace(GL_CCW);
	}
	/* Farbe ist eigentlich wurscht, da wir wahrscheinlich in den Stencil-Buffer rendern. */
	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);

	/* Die Decke rendern. */
	glVertex3d(-r1,yoffs,+r1);
	glVertex3d(+r1,yoffs,r1);
	glVertex3d(+r1,yoffs,-r1);
	glVertex3d(-r1,yoffs,-r1);
	
	/* Die 4 Seitenwaende rendern. */
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

	/* Und Boden rendern. */
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,+r1+dir[2]*infinity);
	glVertex3d(-r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);
	glVertex3d(+r1+dir[0]*infinity,dir[1]*infinity,-r1+dir[2]*infinity);
	glEnd();
	/* Original Face-Order wiederherstellen. */
	glFrontFace(ffc);
}

/**
 * Rendert einen (texturierten) Wuerfel, mit den angegebenen Koordinaten.
 * Texturkoordinaten sind Weltkoordinaten.
 */
void CGUI::renderCube(double x1,double z1,double x2,double z2,double y1,double y2)const
{
	glBegin(GL_QUADS);

	/* Oben */
	glNormal3d(0,1.0,0);
	glTexCoord2d(x2,z1);
	glVertex3d(x2,y2,z1);
	glTexCoord2d(x1,z1);
	glVertex3d(x1,y2,z1);
	glTexCoord2d(x1,z2);
	glVertex3d(x1,y2,z2);
	glTexCoord2d(x2,z2);
	glVertex3d(x2,y2,z2);

	/* Links */
	glNormal3d(-1,0,0);
	glTexCoord2d(x2-(y1-y2),z2);
	glVertex3d(x2,y1,z2);
	glTexCoord2d(x2-(y1-y2),z1);
	glVertex3d(x2,y1,z1);
	glTexCoord2d(x2,z1);
	glVertex3d(x2,y2,z1);
	glTexCoord2d(x2,z2);
	glVertex3d(x2,y2,z2);

	/* Rechts */
	glNormal3d(1,0,0);
	glTexCoord2d(x1+(y2-y1),z1);
	glVertex3d(x1,y1,z1);
	glTexCoord2d(x1+(y2-y1),z2);
	glVertex3d(x1,y1,z2);
	glTexCoord2d(x1,z2);
	glVertex3d(x1,y2,z2);
	glTexCoord2d(x1,z1);
	glVertex3d(x1,y2,z1);

	/* Vorne */
	glNormal3d(0,0,1);
	glTexCoord2d(x1,z2-(y1-y2));
	glVertex3d(x1,y1,z2);
	glTexCoord2d(x2,z2-(y1-y2));
	glVertex3d(x2,y1,z2);
	glTexCoord2d(x2,z2);
	glVertex3d(x2,y2,z2);
	glTexCoord2d(x1,z2);
	glVertex3d(x1,y2,z2);

	/* Hinten */
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
 * true, wenn das Feld x/y auf dem Spielfeld zur Auswahl des aktuellen Steins auf dem
 * Feld gehoert. Sonst false.
 **/
bool CGUI::isInSelection(int x,int y)const
{
	/* Wenn kein Stein oder kein Quadrat auf dem Spielfeld ausgewaehlt ist, false */
	if (current_stone==-1)return false;
	if (selectedx==-1 || selectedy==-1)return false;

	/* Stein holen. */
	CStone *stone=spiel->get_current_player()->get_stone(current_stone);
	/* x/y in Koordinaten relativ zum Stein umrechnen. */
	y=y-selectedy+stone->get_stone_size()/2;
	x=x-selectedx+stone->get_stone_size()/2;
	/* Wenn ausserhalb des Steins, ists auf jeden Fall ausserhalb. */
	if (x<0 || y<0 || x>=stone->get_stone_size() || y>=stone->get_stone_size())return false;

	/* Sonst true, wenn der Stein dort tatsaechlich ein Feld besitzt */
	if (stone->get_stone_field(y,x)!=STONE_FIELD_FREE)return true;
	/* Sonst wohl nicht. */
	return false;
}

/**
 * Der Chat-Box eine Nachricht hinzufuegen, von [client] (-1 fuer Servernachricht)
 **/
void CGUI::addChatMessage(int client,const char* text)
{
	chatbox->addChatMessage(client,text);
}

/**
 * Die Chat-Box sticky setzen. Wenn true, dann verschwindet sie nicht nach 7 sek automatisch.
 **/
void CGUI::setChatBoxSticky(bool sticky)
{
	chatbox->setSticky(sticky);
}

/**
 * Der SpielClient signalisiert hier, dass ein neuer Spieler dran ist. Bitte GUI dafuer
 * updaten.
 **/
void CGUI::newCurrentPlayer(int player)
{
	if (player==-1)
	{
		/* Es ist kein Spieler aktiv. z.B. Spielende, oder Pause oder sonstwas. */
		/* Frame in der Menuleiste verstecken. */
		playerframe->setAlpha(0.0);
		playertext->setText("");
		/* Knoepfe deaktivieren, die man nun nicht benutzen sollte. */
		hint->setEnabled(false);
		undo->setEnabled(false);
	}else{
		char text[500];
		/* Frame entsprechend transparent setzen, abhaengig, ob der Spieler lokal
		   ist, oder nicht. */
		playerframe->setAlpha(spiel->is_local_player()?0.75:0.20);
		/* Farbe des Frames setzen. */
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
		if (spiel->is_local_player())
		{
			/* lokaler Spieler hat anderen Text. */
			sprintf(text,"%s, it's your turn!",COLOR_NAME[player]);
			/* Hint soll nur aktiv sein, wenn nur ein Client, oder nur ein
			   lokaler Spieler mitspielen. */
			bool b=spiel->getServerStatus()->clients<=1;
			b=b||(spiel->getServerStatus()->player<=1);
			/* Knoepfe entsprechend aktivieren. */
			hint->setEnabled(true);
			undo->setEnabled(b);
			testSelection(lastx,lasty);
		}else{
			/* Spieler muss warten, entsprechend darstellen. */
			sprintf(text,"Waiting for %s...",COLOR_NAME[player]);
			hint->setEnabled(false);
			undo->setEnabled(false);
		}
		/* Den Text setzen. */
		playertext->setText(text);
	}
}

/**
 * Zeigt einen Zugvorschlag an.
 **/
void CGUI::showHint()
{
	CPlayer *p=spiel->get_current_player();
	/* Nur anzeigen, wenn tatsaechlich gerade ein lokaler Spieler dran ist. */
	if (p && spiel->is_local_player() && hint->isEnabled())
	{
		/* Die KI soll mal einen Zug vorschlagen. */
#if (1)
		NET_REQUEST_HINT data;
		data.player=spiel->current_player();
		spiel->send_message((NET_HEADER*)&data,sizeof(data),MSG_REQUEST_HINT);
		hint->setEnabled(false);
#else
		CTurn *t=spiel->get_ki_turn(spiel->current_player(),KI_HARD); 
		/* Animation nicht weiterlaufen lassen, da sonst Spruenge auftreten. */
		timer.reset(); 
		/* Wenn es einen Zug gibt */
		if (t)
		{
			/* Effekt zu dem Zug anzeigen, seicht blinken lassen. */
			effects->add(new CStoneFadeEffect(this,t,spiel->current_player()));
			/* Den vorgeschlagenen Zug sogar auswaehlen. */
			current_stone=t->get_stone_number();
			spiel->get_current_player()->get_stone(t->get_stone_number())->mirror_rotate_to(t->get_mirror_count(),t->get_rotate_count());
		}
#endif
	}
}

void CGUI::setHintEnable(bool b)
{
	hint->setEnabled(b);
	testSelection(lastx,lasty);
}
