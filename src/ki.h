#ifndef ________KI_H_
#define ________KI_H_

#include <stdlib.h>
#include "player.h"
#include "turn.h"
#include "time.h"


class CBoard;

class CPlayer;

class CKi {
	private:
		int num_threads;

		void calculate_possible_turns(const CBoard& board, const CStone& stone, const int playerplayernumber);
		void calculate_possible_turns_in_position(const CBoard& board, const CStone& stone, const int player, const int field_y, const int field_x);

		const CTurn* get_ultimate_turn(CBoard& board, const int playernumber, const int ai_error);
		void build_up_turnpool_biggest_x_stones(CBoard& spiel, const int playernumber, const int max_stored_stones);

	public:
		CTurnPool m_turnpool;

		CKi(): num_threads(1) { }
		CKi(int threads): num_threads(threads) { }

		void set_num_threads(int threads) {
		    num_threads = threads;
		}

		const CTurn* get_ki_turn(CBoard& spiel, int player, int ai_error);
};


#endif

