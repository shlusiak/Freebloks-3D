/**
 * spielclient.cpp
 * Autor: Sascha Hlusiak
 *
 * Klasse fuer einen Netzwerk-Client. Das ist die Klasse mit direkter Verbindung zur GUI
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>

#include "gui.h"
#include "stoneeffect.h"
#include "spielclient.h"
#include "gamedialogs.h"



/**
 * Leeren SpielClient initialisieren
 **/
CGUISpielClient::CGUISpielClient(CGUI* vgui)
:CSpielClient()
{
	gui=vgui;
}

void CGUISpielClient::newCurrentPlayer(const int player)
{
	if (gui)gui->newCurrentPlayer(player);
}

void CGUISpielClient::stoneWasSet(NET_SET_STONE *s)
{
	/* Wenn kein lokaler Spieler, zeige eine Blink-Animation an */
	if (gui && !is_local_player(s->player) && gui->getOptions()->get(OPTION_ANIMATE_STONES))
		gui->addEffect(new CStoneRollEffect(
			gui,get_player(s->player)->get_stone(s->stone),s->stone,s->player,s->x,s->y,false));
}

void CGUISpielClient::hintReceived(NET_SET_STONE *s)
{
	/* Entsprechenden Stein des Spielers holen */
	CStone *stone=get_player(s->player)->get_stone(s->stone);
	if (stone && (stone->get_available()>0) && (s->player==current_player()))
	{
		/* Stein in richtige Position drehen */
		stone->mirror_rotate_to(s->mirror_count,s->rotate_count);
		/* Stein aufs echte Spielfeld setzen */
		if (gui)gui->addEffect(new CStoneFadeEffect(gui,stone,s->player,s->x,s->y));
		/* Den vorgeschlagenen Zug sogar auswaehlen. */
		if (gui)gui->setCurrentStone(s->stone);
		if (gui)gui->setHintEnable(true);
	}
}

void CGUISpielClient::gameFinished()
{
	gui->addSubChild(new CGameFinishDialog(gui,this),false);
}

void CGUISpielClient::chatReceived(NET_CHAT* chat)
{
	if (gui)gui->addChatMessage(chat->client,(char*)&chat->text[0]);
}

void CGUISpielClient::gameStarted()
{
	// Den ersten lokalen Spieler ermitteln, und Ansicht der GUI genau
	// dorthin rotieren
	for (int i=0;i<PLAYER_MAX;i++)if (player[i] == PLAYER_LOCAL)
	{
		gui->setAngy(i*90.0);
		break;
	}
	/* Entfernung der Kamera zum Spielfeld angemessen setzen, dass die 
	Steine immer alle drauf passen. */
	int s=get_field_size_x();
	if (get_field_size_y()>s)s=get_field_size_y();
	if (s>20)gui->setZoom(30.0+(double)s-20.0);
		else gui->setZoom(30.0);
}

void CGUISpielClient::stoneUndone(CStone *stone, CTurn *t)
{
	if (gui && gui->getOptions()->get(OPTION_ANIMATE_STONES))
		gui->addEffect(new CStoneRollEffect(
			gui,stone,t->get_stone_number(),t->get_playernumber(),t->get_x(),t->get_y(),true));
}
