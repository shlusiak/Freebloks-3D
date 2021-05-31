#ifndef __GAME_H_INCLUDED_
#define __GAME_H_INCLUDED_

#include "board.h"
#include "constants.h"
#include "turnpool.h"
#include "turn.h"


// player is a computer
#define PLAYER_COMPUTER (-2)

// player is a local player
#define PLAYER_LOCAL (-1)

/**
 * CGame is a CBoard with current status about a game, such as game mode, current player and player types.
 **/
class CGame : public CBoard {
protected:
    // Number of current player or -1, if no current player
    int m_current_player;

    // One of PLAYER_COMPUTER or PLAYER_LOCAL or a remote file handle, if remote player.
    int player[PLAYER_MAX];

    GAMEMODE m_game_mode;

    CTurnPool history;

    void addHistory(const CTurn *turn);

    void addHistory(int player, const CStone *stone, int y, int x);

public:
    CGame();

    void setSpieler(int i, int s) { player[i] = s; }

    void setCurrentPlayer(int c) { m_current_player = c; }

    void set_no_player() { m_current_player = -1; }

    const int current_player() const { return m_current_player; }

    CPlayer *get_current_player() {
        if (m_current_player == -1)
            return NULL;
        else
            return get_player(m_current_player);
    }

    // Return number of non-computer players
    int num_players() const;

    const GAMEMODE get_game_mode() const { return m_game_mode; }
};

#endif
