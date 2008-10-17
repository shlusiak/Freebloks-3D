#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "player.h"

#include "spiel.h"




void CPlayer::init(const CSpiel* spiel, const int playernumber){
	CPlayer::m_number = playernumber;
	for (int i = 0; i < STONE_COUNT_ALL_SHAPES; i++){
		CPlayer::m_stone[i].init(i);
	}
	CPlayer::refresh_data(spiel);
}




//*f�r followsituations CTurn
void CPlayer::init_recycle_player(const CPlayer* player_to_copy){
	CPlayer::m_number = player_to_copy->get_number();
	CPlayer::m_nemesis = player_to_copy->get_nemesis();
	CPlayer::m_teammate = player_to_copy->get_teammate();
	memcpy(m_stone,player_to_copy->m_stone,sizeof(m_stone));
// 	for (int i = 0; i < STONE_COUNT_ALL_SHAPES; i++){
// 		CPlayer::m_stone[i].init(player_to_copy->get_stone(i));
// 	}
}



 
void CPlayer::refresh_data(const CSpiel* spiel){
	
	CPlayer::m_stone_points_left = 0;
	CPlayer::m_number_of_possible_turns = 0;
	CPlayer::m_position_points = 0;
	CPlayer::m_stone_count = 0;
	CPlayer::m_number_of_allowed_fields = 0;	

	for (int n = 0; n < STONE_COUNT_ALL_SHAPES; n++){
		CStone* stone = &CPlayer::m_stone[n];
		if (stone->get_available()){
			int pos_turns = 0;
			CPlayer::m_stone_count += stone->get_available();
			CPlayer::m_stone_points_left += stone->get_stone_points() * stone->get_available();

			for (int x = 0; x < spiel->get_field_size_x(); x++){
				for (int y = 0; y < spiel->get_field_size_y(); y++){
					if (spiel->get_game_field(CPlayer::m_number, y, x) == FIELD_ALLOWED){
						pos_turns += stone->calculate_possible_turns_in_position(spiel, CPlayer::m_number, y, x);
					}
				}	
			}	
			CPlayer::m_number_of_possible_turns += pos_turns;
			CPlayer::m_position_points += pos_turns * stone->get_stone_position_points() * stone->get_stone_points(); //ist ein guter wert!!!!
		}
	}
}



