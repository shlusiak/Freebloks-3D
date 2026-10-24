#ifndef ____BOARD___H__
#define ____BOARD___H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stone.h"
#include "player.h"
#include "turn.h"
#include "constants.h"
#include "ai.h"
#include "turnpool.h"
#include "network.h"

/**
 * The board state includes all fields and players.
 *
 * Does not include current player or player types.
 */
class CBoard {
private:
    int m_field_size_y;
    int m_field_size_x;

    CPlayer m_player[PLAYER_MAX];
    TSingleField *m_game_field;

    bool is_position_inside_field(const int y, const int x) const;

    void set_single_stone_for_player(const int player_number, const int y, const int x);

    void free_game_field(int y, int x);

public:

    // Recomputes the possible turns of all players, needed after changing stone availability
    void refresh_player_data();

    CBoard();

    CBoard(const int width, const int height);

    CBoard(const int player_team1_1, const int player_team1_2, const int player_team2_1,
           const int player_team2_2);

    virtual ~CBoard();

    /* Owns m_game_field, a copy would free it twice */
    CBoard(const CBoard &) = delete;
    CBoard &operator=(const CBoard &) = delete;

    void follow_situation(const CBoard &from_board, const CTurn &turn);

    void init_field();

    void set_seeds(enum GAMEMODE game_mode);

    void set_game_field(const int y, const int x, const TSingleField value);

    int get_player_start_x(const int player_number) const;

    int get_player_start_y(const int player_number) const;


    int get_number_of_possible_turns(const int player_number) const;

    int get_stone_points_left(const int player_number) const;

    int get_position_points(const int player_number) const;

    int get_stone_count(const int player_number) const;

    int get_teammate(const int player_number) const;

    int get_nemesis(const int player_number) const;

    void start_new_game(GAMEMODE game_mode);

    void set_field_size(int x, int y) {
        m_field_size_x = x;
        m_field_size_y = y;
    }

    int get_field_size_x() const;

    int get_field_size_y() const;

    int get_player_max() const;

    void set_teams(int player_team1_1, int player_team1_2, int player_team2_1, int player_team2_2);

    virtual void set_stone_numbers(int8 stone_numbers[]);

    void set_stone_numbers(int8 monominoes, int8 dominoes, int8 trominoes, int8 tetrominoes, int8 pentominoes);

    CPlayer *get_player(const int player_number);

    bool is_valid_turn(const Shape &shape, int player, int y, int x, int mirrored, int rotation) const;

    bool is_valid_turn(const CStone &stone, int player, int y, int x) const {
        return is_valid_turn(stone.get_shape(), player, y, x, stone.get_mirror_counter(), stone.get_rotate_counter());
    }

    bool is_valid_turn(const CTurn &turn);

    TSingleField get_game_field(const int playernumber, const int y, const int x) const;

    TSingleField get_game_field(const int y, const int x) const;

    char get_game_field_value(const int y, const int x) const;

    TSingleField *get_field_pointer() const;

    bool set_stone(CStone &stone, int playernumber, int y, int x);
    bool set_stone(const CTurn &turn);

    void undo_turn(CTurnPool &turn_pool, GAMEMODE game_mode);
};

inline
char CBoard::get_game_field_value(const int y, const int x) const {
    return m_game_field[y * m_field_size_x + x];
}

inline
TSingleField *CBoard::get_field_pointer() const {
    return m_game_field;
}

inline
TSingleField CBoard::get_game_field(const int playernumber, const int y, const int x) const {
    TSingleField value = get_game_field_value(y, x);
    if (value >= PLAYER_BIT_HAVE_MIN) return FIELD_DENIED;
    value &= PLAYER_BIT_ADDR[playernumber];
    if (value == 0) return FIELD_FREE;
    if (value > PLAYER_BIT_ALLOWED[playernumber]) return FIELD_DENIED;
    return FIELD_ALLOWED;
}


inline
TSingleField CBoard::get_game_field(const int y, const int x) const {
    const TSingleField value = CBoard::get_game_field_value(y, x);
    if (value < PLAYER_BIT_HAVE_MIN) return FIELD_FREE;
    return value & 3;
}

inline
int CBoard::get_field_size_x() const {
    return CBoard::m_field_size_x;
}

inline
int CBoard::get_field_size_y() const {
    return CBoard::m_field_size_y;
}

inline
int CBoard::get_player_max() const {
    return PLAYER_MAX;
}

inline
int CBoard::get_stone_count(const int player_number) const {
    return CBoard::m_player[player_number].get_stone_count();
}

inline
int CBoard::get_nemesis(const int player_number) const {
    return CBoard::m_player[player_number].get_nemesis();
}

inline
int CBoard::get_teammate(const int player_number) const {
    return CBoard::m_player[player_number].get_teammate();
}

inline
void CBoard::set_game_field(const int y, const int x, const TSingleField value) {
    m_game_field[y * CBoard::m_field_size_x + x] = value;
}

inline
CPlayer *CBoard::get_player(const int player_number) {
    return &m_player[player_number];
}

inline
bool CBoard::is_position_inside_field(const int y, const int x) const {
    return (y >= 0 && y < CBoard::m_field_size_y && x >= 0 && x < CBoard::m_field_size_x);
}

inline
int CBoard::get_number_of_possible_turns(const int player_number) const {
    return CBoard::m_player[player_number].get_number_of_possible_turns();
}

inline
int CBoard::get_stone_points_left(const int player_number) const {
    return CBoard::m_player[player_number].get_stone_points_left();
}

inline
int CBoard::get_position_points(const int player_number) const {
    return CBoard::m_player[player_number].get_position_points();
}

#endif
