#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "player.h"

#include "board.h"




void CPlayer::init(const CBoard& spiel, const int playernumber){
	m_number = playernumber;
	for (int i = 0; i < STONE_COUNT_ALL_SHAPES; i++){
		m_stone[i].init(i);
	}
	refresh_data(spiel);
}

void CPlayer::refresh_data(const CBoard& spiel){
	m_stone_points_left = 0;
	m_number_of_possible_turns = 0;
	m_position_points = 0;
	m_stone_count = 0;

	for (int n = 0; n < STONE_COUNT_ALL_SHAPES; n++){
		const CStone& stone = m_stone[n];
		m_stone_count += stone.get_available();
		m_stone_points_left += stone.get_stone_points() * stone.get_available();
	}

	for (int x = 0; x < spiel.get_field_size_x(); x++){
		for (int y = 0; y < spiel.get_field_size_y(); y++){
			if (spiel.get_game_field(m_number, y, x) == FIELD_ALLOWED){
				for (int n = 0; n < STONE_COUNT_ALL_SHAPES; n++){
					CStone& stone = m_stone[n];
					if (stone.get_available()){
						int pos_turns = stone.calculate_possible_turns_in_position(spiel, m_number, y, x);

						m_number_of_possible_turns += pos_turns;
						m_position_points += pos_turns * stone.get_stone_position_points() * stone.get_stone_points();
					}
				}
			}
		}
	}
}



