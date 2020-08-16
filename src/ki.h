#ifndef ________KI_H_
#define ________KI_H_

#include <stdlib.h>
#include "player.h"
#include "turn.h"
#include "time.h"


class CBoard;

class CPlayer;

class CKi{

	private:
		int num_threads;

		void calculate_possible_turns(const CBoard* spiel, CStone* stone, const char playernumber);
		void calculate_possible_turns_in_position(const CBoard* spiel, CStone* stone, const char playernumber, const int field_y, const int field_x);


		CTurn* get_ultimate_turn(CBoard* spiel, const char playernumber, const int ki_fehler);
		void build_up_turnpool_biggest_x_stones(CBoard* spiel, const char playernumber, const int max_stored_stones);

	public:
		CKi() { num_threads=1; }

		void set_num_threads(int threads) { num_threads=threads; }

		CTurnpool m_turnpool;
		static int get_ultimate_points(CBoard* spiel, const char playernumber, const int ki_fehler, const CTurn* turn);
		static int get_distance_points(CBoard* follow_situation, const char playernumber, const CTurn* turn);
		CTurn* get_ki_turn(CBoard* spiel, char playernumber, int ki_fehler); //je h�her ki_fehler, desto schlechter die ki... werte zwischen 0 und 1 empfohlen
};


#endif

