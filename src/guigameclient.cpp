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
#include "gameclient.h"
#include "gamedialogs.h"



CGUIGameClient::CGUIGameClient(CGUI* vgui)
:CGameClient()
{
	gui=vgui;
}

void CGUIGameClient::newCurrentPlayer(const int player)
{
	if (gui)gui->newCurrentPlayer(player);
}

void CGUIGameClient::stoneWasSet(NET_SET_STONE *s)
{
	/* Wenn kein lokaler Spieler, zeige eine Blink-Animation an */
	if (gui && !is_local_player(s->player) && gui->getOptions()->get(OPTION_ANIMATE_STONES))
		gui->addEffect(new CStoneRollEffect(
			gui,&get_player(s->player)->get_stone(s->stone),s->stone,s->player,s->x,s->y,false));
}

void CGUIGameClient::hintReceived(NET_SET_STONE *s)
{
	CStone &stone=get_player(s->player)->get_stone(s->stone);
	if ((stone.get_available() > 0) && (s->player==current_player()))
	{
		stone.mirror_rotate_to(s->mirror_count,s->rotate_count);
		if (gui)gui->addEffect(new CStoneFadeEffect(gui,&stone,s->player,s->x,s->y));
		if (gui)gui->setCurrentStone(s->stone);
		if (gui)gui->setHintEnable(true);
	}
}

void CGUIGameClient::gameFinished()
{
	gui->addSubChild(new CGameFinishDialog(gui,this),false);
}

void CGUIGameClient::chatReceived(NET_CHAT* chat)
{
	if (gui)gui->addChatMessage(chat->client,(char*)&chat->text[0]);
}

void CGUIGameClient::gameStarted()
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

void CGUIGameClient::stoneUndone(const CStone &stone, const CTurn *t)
{
	if (gui && gui->getOptions()->get(OPTION_ANIMATE_STONES))
		gui->addEffect(new CStoneRollEffect(
				gui, &stone, t->stone_number, t->player, t->x, t->y, true));
}
