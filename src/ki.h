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

		void calculate_possible_turns(const CBoard& spiel, CStone& stone, const char playernumber);
		void calculate_possible_turns_in_position(const CBoard& spiel, CStone& stone, const char playernumber, const int field_y, const int field_x);

		const CTurn* get_ultimate_turn(CBoard& spiel, const char playernumber, const int ki_fehler);
		void build_up_turnpool_biggest_x_stones(CBoard& spiel, const char playernumber, const int max_stored_stones);

	public:
		CTurnPool m_turnpool;

		CKi(): num_threads(1) { }
		CKi(int threads): num_threads(threads) { }

		void set_num_threads(int threads) {
		    num_threads = threads;
		}

		static int get_ultimate_points(CBoard& spiel, const char playernumber, const int ai_error, const CTurn& turn);
		static int get_distance_points(CBoard& follow_situation, const char playernumber, const CTurn& turn);
		const CTurn* get_ki_turn(CBoard& spiel, char playernumber, int ki_fehler);
};


#endif

