/**
 * spielleiter.cpp
 * Autor: Sascha Hlusiak
 *
 * Erweiterung von CBoard um Grundlegende Funktionen eines Spielleiters
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "game.h"

CGame::CGame():
	m_current_player(-1),
	m_game_mode(GAMEMODE_4_COLORS_4_PLAYERS)
{
	for (int i=0;i<PLAYER_MAX;i++)
		player[i] = PLAYER_COMPUTER;
}

/**
 * Appends a turn to the history.
 **/
void CGame::addHistory(const CTurn *turn)
{
	history.add_turn(turn);
}

/**
 * Appends a turn to the history.
 **/
void CGame::addHistory(int player, const CStone &stone, int y, int x)
{
	history.add_turn(player, stone, y, x);
}

/**
 * @return number of non-computer players
 **/
int CGame::num_players()const
{
	int n;
	n=0;
	for (int i=0;i<PLAYER_MAX;i++)if (player[i] != PLAYER_COMPUTER)n++;
	return n;
}


