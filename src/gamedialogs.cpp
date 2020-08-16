/**
 * gamedialogs.cpp
 * Klassen fuer Spiel-Start und Spiel-Beendet Dialoge
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "gamedialogs.h"
#include "widgets.h"
#include "gui.h"
#include "spielclient.h"


/**
 * Dialog beim Start eines Multiplayerspiels. Es werden Daten ueber das zu startende Spiel angezeigt,
 * sowie ein Chat angeboten. Dies ist quasi eine Lobby, ueber die das Spiel endgueltig gestartet wird.
 * Es muessen der aktuelle SpielClient, die GUI und der Servername als Zeichenkette uebergeben werden.
 **/
CStartGameDialog::CStartGameDialog(CSpielClient *vclient,CGUI* vgui,const char* servername)
:CDialog(300,260,"Waiting for players")
{
	gui=vgui;
	/* Die ChatBox soll nicht automatisch verschwinden, damit Nachrichten laenger gelesen werden koennen. */
	gui->setChatBoxSticky(true);
	client=vclient;
	setColor(0.9,0.3,0);
	statustext=NULL;
	/* Zeit auf 0 setzen. Jeweils nach 0.5 sek wird der StatusText aktualisiert. */
	time=0.0;

	/* Servername soll oben im Titel stehen. */
	addChild(new CStaticText(10,25,280,servername,this));
	/* Beschriftung des statustext. */
	addChild(new CStaticText(10,60,"Connected clients:\nHuman players:\nComputer players:\n\nYou will play:",this));

	/* Chat-Feld, mit Fokus */
	addChild(chat=new CTextEdit(10,190,230,20,97,"",false,this));
	chat->setFocus();

	/* Buttons */
	addChild(new CButton(245,188,50,24,96,this,"Chat"));
	addChild(new CButton(30,h-35,110,25,99,this,"Start Now"));
	addChild(new CButton(150,h-35,110,25,98,this,"Cancel"));
	/* Einen leeren statustext. */
	addChild(statustext=new CStaticText(190,60,0,"",this));
}

/**
 * Mausereignis verarbeiten (Druck auf Button).
 **/
int CStartGameDialog::processMouseEvent(TMouseEvent *event)
{
	/* Dialog das Event verarbeiten lassen. */
	int r=CDialog::processMouseEvent(event);
	/* Spielstart wurde betaetigt. */
	if (r==99)
	{
		/* Spiel starten. Der Dialog schliesst sich von allein, sobald das Spiel laeuft. */
		if (client)client->request_start();
		return 0;
	}else if (r==98)
	{
		/* Close-Button wurde betaetigt. Verbindung zum Server kappen und Dialog schliessen. */
		if (client)client->Disconnect();
		gui->setChatBoxSticky(false);
		close();
		return 0;
	}else if (r==96)
	{
		/* Chat-Nachricht abschicken. */
		sendChat();
		return 0;
	}else return r;
}

/**
 * statustext mit neuen Daten aus NET_SERVER_STATUS fuettern.
 **/
void CStartGameDialog::updateStatus(NET_SERVER_STATUS *status)
{
	char c[64];
	int i;
	/* Neuen Text bauen */
	sprintf(c,"%d\n%d\n%d\n\n",status->clients,status->player,status->computer);
	for (i=0;i<PLAYER_MAX;i++)if (client && client->is_local_player(i))
	{
		strcat(c,COLOR_NAME[i]);
		strcat(c,"\n");
	}
	/* Und statustext neu zuweisen. */
	statustext->setText(c,true);
}

/**
 * Verbindung ueberpruefen und so
 **/
void CStartGameDialog::execute(double elapsed)
{
	CDialog::execute(elapsed);
	/* Wenn das Spiel entweder laeuft, oder der Client nicht mehr verbunden ist (Server down),
	   Dialog schliessen. */
	if (client && (client->current_player()!=-1 || !client->isConnected()))
	{
		/* Chat-Box darf wieder automatisch geschlossen werden. */
		gui->setChatBoxSticky(false);
		/* Dialog schliessen, aber nicht rekursiv, es koennten Fehlermeldungen oben liegen. */
		close(false);
		client=0;
	}
	/* Nach 0.5 sek soll der Status aktualisiert werden. */
	time+=elapsed;
	if (time>0.5 && client)
	{
		time=0.0;
		updateStatus(client->getServerStatus());
	}
}

/**
 * Sendet die Chat-Nachricht, die aktuell im EditFeld eingetragen ist.
 **/
void CStartGameDialog::sendChat()
{
	/* Chat-Nachricht abschicken. */
	client->chat(chat->getText());
	/* Und Eingabefeld leeren */
	chat->setText("");
	/* Sowie ihm wieder den Fokus geben. */
	chat->setFocus();
}

/**
 * Tastendruck verarbeiten.
 **/
bool CStartGameDialog::processKey(unsigned short key)
{
	/* Wird hier Escape gedrueckt. */
	if (key==VK_ESCAPE)
	{
		/* Verbindung zum Server kappen. */
		if (client)client->Disconnect();
		client=0;
		/* Chatbox darf wieder automatisch geschlosen werden. */
		gui->setChatBoxSticky(false);
		/* Dialog schliessen. */
		close();
		return true;
	}
	/* Sonst Rest des Dialogs die Nachricht verarbeiten lassen. */
	bool b=CDialog::processKey(key);
	/* Und wenn ueber dem Chat EditFeld Enter gedrueckt wurde, die Nachricht abschicken. */
	if (chat->committed())sendChat();
	return b;
}





/**
 * Der Dialog, der ueber Ausgang einer Partie berichtet, mit Chat-Feld.
 **/
CGameFinishDialog::CGameFinishDialog(CGUI *vgui,CSpielClient *client)
:CDialog(390,220,"Game finished")
{
	/* Welcher Spieler auf dem entsprechenden Platz ist. Wird noch sortiert. */
	int place[PLAYER_MAX]={0,1,2,3};
	/* Farbnamen der Spieler. */
	int i;
	double y,w;

	if (client->getServerStatus()->clients==1)this->h-=30;

	gui=vgui;
	/* Chat-Box wird nicht automatisch geschlossen. */
	gui->setChatBoxSticky(true);

	/* Hier die Plaetze absteigend mit Bubble-Sort sortieren. */
	i=0;
	while (i<PLAYER_MAX-1)
	{
		if (client->get_player(place[i])->get_stone_points_left()>client->get_stone_points_left(place[i+1]))
		{
			int bla=place[i];
			place[i]=place[i+1];
			place[i+1]=bla;
			i=0;
		}else i++;
	}

	y=40;
	w=320;
	int p=1;	/* Aktueller Platz */
	/* Ein grosses Frame in den Dialog pflanschen */
	CFrame *frame;
	frame=new CFrame(x+15,this->y+30,this->w-40,115);
	frame->setColor(0,0,0);
	frame->setAlpha(0.70);
	addChild(frame);
	/* Alle Plaetze durchgehen, und sofern der Platz ueberhaupt existiert... */
	for (i=0;i<PLAYER_MAX;i++)if (
		   (client->get_game_mode() == GAMEMODE_2_COLORS_2_PLAYERS && (place[i] == 0 || place[i] == 2))
		|| (client->get_game_mode() == GAMEMODE_DUO && (place[i] == 0 || place[i] == 2))
		|| (client->get_game_mode() == GAMEMODE_JUNIOR && (place[i] == 0 || place[i] == 2))
		|| (client->get_game_mode() == GAMEMODE_4_COLORS_4_PLAYERS)
		|| (client->get_game_mode() == GAMEMODE_4_COLORS_2_PLAYERS && (place[i] == 0 || place[i] == 1))
		)
	{
		/* Wenn lokaler Spieler, waehle Farben intensiver und mal einen * */
		bool local=client->is_local_player(place[i]);
		char t[256];
		CFrame *frame;

		/* In diesem Spielmodi addieren wir die Punkte und malen zwei Frames in beiden Farben. */
		if (client->get_game_mode() == GAMEMODE_4_COLORS_2_PLAYERS)
		{
			/* Linken Frame erstellen und Farbe setzen */
			frame=new CFrame(x+20+(this->w-w)/2.0,this->y+y,(w-20)/2.0,20);
			frame->setAlpha(local?0.7:0.25);
			switch (place[i])
			{
			default:
			case 0:frame->setColor(0,0.3,1.0);
				break;
			case 1:frame->setColor(1.0,1.0,0);
				break;
			case 2:frame->setColor(1.0,0.1,0.1);
				break;
			case 3:frame->setColor(0,1.0,0);
				break;
			}
			addChild(frame);

			/* Rechten Frame erstellen und Farbe setzen. */
			frame=new CFrame(x+10+(this->w-w)/2.0+w/2.0,this->y+y,(w-20)/2.0,20);
			frame->setAlpha(local?0.7:0.25);
			switch (client->get_teammate(place[i]))
			{
			default:
			case 0:frame->setColor(0,0.3,1.0);
				break;
			case 1:frame->setColor(1.0,1.0,0);
				break;
			case 2:frame->setColor(1.0,0.1,0.1);
				break;
			case 3:frame->setColor(0,1.0,0);
				break;
			}
			addChild(frame);
			/* Punkte und Steine der beiden Farben addieren */
			int points=-client->get_stone_points_left(place[i])
			    -client->get_stone_points_left(client->get_teammate(place[i]));
			int stones=client->get_stone_count(place[i])
			    +client->get_stone_count(client->get_teammate(place[i]));
			/* Text setzen. */
			sprintf(t,"%s/%s: %d points (%d stones)",COLOR_NAME[place[i]],COLOR_NAME[client->get_teammate(place[i])],
				points,stones);
		}else{
			/* Wir erstellen nur ein Frame mit der Farbe des Spielers. */
			frame=new CFrame(x+20+(this->w-w)/2.0,this->y+y,w-20,20);
			frame->setAlpha(local?0.7:0.25);
			switch (place[i])
			{
			default:
			case 0:frame->setColor(0,0.3,1.0);
				break;
			case 1:frame->setColor(1.0,1.0,0);
				break;
			case 2:frame->setColor(1.0,0.1,0.1);
				break;
			case 3:frame->setColor(0,1.0,0);
				break;
			}
			addChild(frame);
			/* Text setzen. */
			sprintf(t,"%s: %d points (%d stones)",COLOR_NAME[place[i]],-client->get_stone_points_left(place[i]),client->get_stone_count(place[i]));
		}
		/* Den Text als Statictext in das Frame setzen. */
		addChild(new CStaticText(20,y+2,this->w-20,t,this));
		/* Platz Nummer links daneben schreiben, mit *, falls Spieler lokal ist. */
		sprintf(t,"%c %d.",local?'*':' ',p);
		CStaticText *text=new CStaticText(17,y+2,t,this);
		if (!local)text->setAlpha(0.5);
		addChild(text);
		/* Neue Posistion und Platz waehlen. */
		y+=25;
		w+=5;
		p++;
	}

	/* Nur Chat anbieten, wenn mehr als ein Client verbunden */
	if (client->getServerStatus()->clients>1)
	{
		/* Chat TextEdit Feld und Button erstellen. */
		addChild(chat=new CTextEdit(30,155,270,20,97,"",false,this));
		chat->setFocus();
		addChild(new CButton(310,153,50,24,96,this,"Chat"));
	}else chat=NULL;

// 	setColor(0.0,0.10,0.30);
	setColor(0.9,0.3,0);
	setAlpha(0.5);
	/* Buttons hinzufuegen. */
	addChild(new CButton(50,this->h-35,120,25,17,this,"New Round"));
	addChild(new CButton((this->w-120.0-50.0),this->h-35,120,25,18,this,(client->getServerStatus()->clients>1)?"Disconnect":"Close"));
}

/**
 * Mausereignis verarbeiten (Druck auf Button)
 **/
int CGameFinishDialog::processMouseEvent(TMouseEvent *event)
{
	int r=CDialog::processMouseEvent(event);
	/* Chat-Nachricht senden. */
	if (r==96)sendChat();
	else if (r==17)
	/* Neue Runde starten und Dialog schliessen. */
	{
		gui->spiel->request_start();
		close();
	}else if (r==18)
	{
		/* Verbindung zum Server trennen und Dialog schliessen. */
		close();
		gui->spiel->Disconnect();
	}else return r;
	return 0;
}

/**
 * Nachricht aus Texteingabefeld versenden
 **/
void CGameFinishDialog::sendChat()
{
	/* Nachricht senden. */
	gui->spiel->chat(chat->getText());
	/* Eingabefeld leeren. */
	chat->setText("");
	/* Und Fokus geben. */
	chat->setFocus();
}

/**
 * Tastendruck verarbeiten
 **/
bool CGameFinishDialog::processKey(unsigned short key)
{
	/* Wird hier Escape gedrueckt. */
	if (key==VK_ESCAPE)
	{
		/* Verbindung zum Server kappen. */
		gui->spiel->Disconnect();
		/* Dialog schliessen. */
		close();
		return true;
	}
	/* Dialog die Taste verarbeiten lassen. */
	bool b=CDialog::processKey(key);
	/* Und wenn das Text-Feld mit Enter verlassen wurde, Nachricht verschicken */
	if (chat && chat->committed())sendChat();
	return b;
}

/**
 * Dialog schliessen ueberschreiben. Das laesst das automatische Schliessen der ChatBox wieder zu.
 **/
void CGameFinishDialog::close()
{
	gui->setChatBoxSticky(false);
	CDialog::close();
}
