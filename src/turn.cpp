#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "turn.h"

#include "spiel.h"



CTurn::~CTurn(){
	if (m_follow_situation){
		delete m_follow_situation;
	}
	if (m_next){
		delete m_next;
	}
}


CTurn::CTurn(const int turn_number, const int playernumber, const CStone* stone, const int y, const int x){
	init_CTurn(turn_number, playernumber, stone, y, x);
}



CTurn::CTurn(const int playernumber, const CStone* stone, const int y, const int x){
	init_CTurn(-1, playernumber, stone, y, x);
}


CTurn::CTurn(const CTurn* turn){
	CTurn::m_playernumber = turn->get_playernumber();
	CTurn::m_turn_number = turn->get_turn_number();
	CTurn::m_stone_number = turn->get_stone_number();
	CTurn::m_mirror_count = turn->get_mirror_count();
	CTurn::m_rotate_count = turn->get_rotate_count();
	CTurn::m_y = turn->get_y();
	CTurn::m_x = turn->get_x();
	CTurn::m_next = 0;
	
	CTurn::m_follow_situation = 0;
}


void CTurn::init_CTurn(const int turn_number, const int playernumber, const CStone* stone, const int y, const int x){
	CTurn::m_playernumber = playernumber;
	CTurn::m_turn_number = turn_number;
	CTurn::m_stone_number = stone->get_number();
	CTurn::m_mirror_count = stone->get_mirror_counter();
	CTurn::m_rotate_count = stone->get_rotate_counter();
	CTurn::m_y = y;
	CTurn::m_x = x;
	CTurn::m_next = 0;
	
	CTurn::m_follow_situation = 0;
}




CSpiel* CTurn::get_follow_situation(const CSpiel* spiel_vor, const int playernumber){
	if (!CTurn::m_follow_situation) {
		CTurn::m_follow_situation = new CSpiel(playernumber, spiel_vor, this);
	}
	return CTurn::m_follow_situation;
}






