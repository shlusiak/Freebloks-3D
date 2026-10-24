/**
 * gamedialogs.cpp
 * Classes for the game start and game finished dialogs
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "gamedialogs.h"
#include "widgets.h"
#include "gui.h"
#include "gameclient.h"


/**
 * Dialog at the start of a multiplayer game. Shows data about the game to be started
 * and offers a chat. It is effectively a lobby from which the game is finally started.
 * Takes the current game client, the GUI and the server name as a string.
 **/
CStartGameDialog::CStartGameDialog(CGameClient *vclient,CGUI* vgui,const char* servername)
:CDialog(300,260,"Waiting for players")
{
	gui=vgui;
	/* The chat box should not hide automatically, so messages can be read for longer. */
	gui->setChatBoxSticky(true);
	client=vclient;
	setColor(0.9,0.3,0);
	statustext=nullptr;
	/* Set the time to 0. The status text is refreshed every 0.5 s. */
	time=0.0;

	/* The server name goes at the top of the title. */
	addChild(new CStaticText(10,25,280,servername,this));
	/* Labels of the status text. */
	addChild(new CStaticText(10,60,"Connected clients:\nHuman players:\nComputer players:\n\nYou will play:",this));

	/* Chat field, with focus */
	addChild(chat=new CTextEdit(10,190,230,20,97,"",false,this));
	chat->setFocus();

	/* Buttons */
	addChild(new CButton(245,188,50,24,96,this,"Chat"));
	addChild(new CButton(30,h-35,110,25,99,this,"Start Now"));
	addChild(new CButton(150,h-35,110,25,98,this,"Cancel"));
	addChild(statustext=new CStaticText(190,60,0,"",this));
}

int CStartGameDialog::processMouseEvent(TMouseEvent *event)
{
	int r=CDialog::processMouseEvent(event);
	/* Start game was pressed. */
	if (r==99)
	{
		/* Start the game. The dialog closes itself as soon as the game is running. */
		if (client)client->request_start();
		return 0;
	}else if (r==98)
	{
		/* The close button was pressed. Cut the connection to the server and close the dialog. */
		if (client)client->Disconnect();
		gui->setChatBoxSticky(false);
		close();
		return 0;
	}else if (r==96)
	{
		sendChat();
		return 0;
	}else return r;
}

/**
 * Feed the status text with new data from NET_SERVER_STATUS.
 **/
void CStartGameDialog::updateStatus(NET_SERVER_STATUS *status)
{
	char c[64];
	int i;
	snprintf(c, sizeof(c), "%d\n%d\n%d\n\n", status->clients, status->player, status->computer);
	for (i=0;i<PLAYER_MAX;i++)if (client && client->is_local_player(i))
	{
		strcat(c,COLOR_NAME[i]);
		strcat(c,"\n");
	}
	statustext->setText(c,true);
}

void CStartGameDialog::execute(double elapsed)
{
	CDialog::execute(elapsed);
	/* If the game is either running or the client is no longer connected (server down),
	   close the dialog. */
	if (client && (client->current_player()!=-1 || !client->isConnected()))
	{
		/* The chat box may close automatically again. */
		gui->setChatBoxSticky(false);
		/* Close the dialog, but not recursively; error messages may be on top. */
		close(false);
		client=0;
	}
	/* Refresh the status after 0.5 s. */
	time+=elapsed;
	if (time>0.5 && client)
	{
		time=0.0;
		updateStatus(client->getServerStatus());
	}
}

/**
 * Sends the chat message currently entered in the edit field.
 **/
void CStartGameDialog::sendChat()
{
	/* The dialog stays alive during its close animation, but client is already gone then */
	if (client==nullptr)return;
	client->chat(chat->getText());
	chat->setText("");
	chat->setFocus();
}

/**
 * Handle a key press.
 **/
bool CStartGameDialog::processKey(unsigned short key)
{
	if (key==VK_ESCAPE)
	{
		if (client)client->Disconnect();
		client=0;
		/* The chat box may close automatically again. */
		gui->setChatBoxSticky(false);
		/* Close the dialog. */
		close();
		return true;
	}
	bool b=CDialog::processKey(key);
	/* And if Enter was pressed in the chat edit field, send the message. */
	if (chat->committed())sendChat();
	return b;
}





/**
 * The dialog that reports the outcome of a match, with a chat field.
 **/
CGameFinishDialog::CGameFinishDialog(CGUI *vgui,CGameClient *client)
:CDialog(390,220,"Game finished")
{
	/* Which player is in which place. Sorted below. */
	int place[PLAYER_MAX]={0,1,2,3};
	int i;
	double y,w;

	if (client->getServerStatus()->clients==1)this->h-=30;

	gui=vgui;
	/* The chat box does not close automatically. */
	gui->setChatBoxSticky(true);

	/* Sort the places in descending order with bubble sort. */
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
	int p=1;	/* Current place */
	/* Put a large frame into the dialog */
	CFrame *frame;
	frame=new CFrame(x+15,this->y+30,this->w-40,115);
	frame->setColor(0,0,0);
	frame->setAlpha(0.70);
	addChild(frame);
	/* Go through all places, and if the place exists at all... */
	for (i=0;i<PLAYER_MAX;i++)if (
		   (client->get_game_mode() == GAMEMODE_2_COLORS_2_PLAYERS && (place[i] == 0 || place[i] == 2))
		|| (client->get_game_mode() == GAMEMODE_DUO && (place[i] == 0 || place[i] == 2))
		|| (client->get_game_mode() == GAMEMODE_JUNIOR && (place[i] == 0 || place[i] == 2))
		|| (client->get_game_mode() == GAMEMODE_4_COLORS_4_PLAYERS)
		|| (client->get_game_mode() == GAMEMODE_4_COLORS_2_PLAYERS && (place[i] == 0 || place[i] == 1))
		)
	{
		/* For a local player, use more intense colours and add a * */
		bool local=client->is_local_player(place[i]);
		char t[256];
		CFrame *frame;

		/* In this game mode we add up the points and draw two frames in both colours. */
		if (client->get_game_mode() == GAMEMODE_4_COLORS_2_PLAYERS)
		{
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
			/* Add up the points and stones of both colours */
			int points=-client->get_stone_points_left(place[i])
			    -client->get_stone_points_left(client->get_teammate(place[i]));
			int stones=client->get_stone_count(place[i])
			    +client->get_stone_count(client->get_teammate(place[i]));
			/* Set the text. */
			snprintf(t, sizeof(t), "%s/%s: %d points (%d stones)", COLOR_NAME[place[i]], COLOR_NAME[client->get_teammate(place[i])],
				points,stones);
		}else{
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
			/* Set the text. */
			snprintf(t, sizeof(t), "%s: %d points (%d stones)", COLOR_NAME[place[i]], -client->get_stone_points_left(place[i]), client->get_stone_count(place[i]));
		}
		addChild(new CStaticText(20,y+2,this->w-20,t,this));
		/* Write the place number to the left, with * if the player is local. */
		snprintf(t, sizeof(t), "%c %d.", local ? '*' :  ' ',p);
		CStaticText *text=new CStaticText(17,y+2,t,this);
		if (!local)text->setAlpha(0.5);
		addChild(text);
		y+=25;
		w+=5;
		p++;
	}

	/* Only offer chat if more than one client is connected */
	if (client->getServerStatus()->clients>1)
	{
		addChild(chat=new CTextEdit(30,155,270,20,97,"",false,this));
		chat->setFocus();
		addChild(new CButton(310,153,50,24,96,this,"Chat"));
	}else chat=nullptr;

// 	setColor(0.0,0.10,0.30);
	setColor(0.9,0.3,0);
	setAlpha(0.5);
	/* Add buttons. */
	addChild(new CButton(50,this->h-35,120,25,17,this,"New Round"));
	addChild(new CButton((this->w-120.0-50.0),this->h-35,120,25,18,this,(client->getServerStatus()->clients>1)?"Disconnect":"Close"));
}

int CGameFinishDialog::processMouseEvent(TMouseEvent *event)
{
	int r=CDialog::processMouseEvent(event);
	if (r==96)sendChat();
	else if (r==17)
	/* Start a new round and close the dialog. */
	{
		gui->game->request_start();
		close();
	}else if (r==18)
	{
		/* Disconnect from the server and close the dialog. */
		close();
		gui->game->Disconnect();
	}else return r;
	return 0;
}

void CGameFinishDialog::sendChat()
{
	gui->game->chat(chat->getText());
	/* Clear the input field. */
	chat->setText("");
	chat->setFocus();
}

/**
 * Handle a key press
 **/
bool CGameFinishDialog::processKey(unsigned short key)
{
	if (key==VK_ESCAPE)
	{
		gui->game->Disconnect();
		/* Close the dialog. */
		close();
		return true;
	}
	bool b=CDialog::processKey(key);
	/* And if the text field was left with Enter, send the message */
	if (chat && chat->committed())sendChat();
	return b;
}

/**
 * Override closing the dialog. This allows the chat box to close automatically again.
 **/
void CGameFinishDialog::close()
{
	gui->setChatBoxSticky(false);
	CDialog::close();
}
