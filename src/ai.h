#ifndef ________AI_H_
#define ________AI_H_

#include <stdlib.h>
#include "player.h"
#include "turn.h"
#include "time.h"


class CBoard;

class CPlayer;

class CAI {
	private:
		int num_threads;

		void calculate_possible_turns(const CBoard& board, const Shape& shape, const int playernumber);
		void calculate_possible_turns_in_position(const CBoard& board, const Shape& shape, const int player, const int field_y, const int field_x);

		const CTurn* get_ultimate_turn(CBoard& board, const int playernumber, const int ai_error);
		void build_up_turnpool_biggest_x_stones(CBoard& board, const int playernumber, const int max_stored_stones);

	public:
		CTurnPool m_turnpool;

		CAI(): num_threads(1) { }
		CAI(int threads): num_threads(threads) { }

		void set_num_threads(int threads) {
		    num_threads = threads;
		}

		const CTurn* get_ai_turn(CBoard& board, int player, int ai_error);
};


#endif

