#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <cstddef>
#include "turnpool.h"

void CTurnPool::add_turn(const CTurn* turn) {
	CTurn* new_element = new CTurn(turn);
	turns.push_back(new_element);
}

void CTurnPool::add_turn(const int player, const CStone& stone, const int y, const int x) {
	CTurn* new_element = new CTurn(player, stone.get_number(), stone.get_mirror_counter(), stone.get_rotate_counter(), y, x);
	turns.push_back(new_element);
}

void CTurnPool::delete_last(){
	#ifdef _DEBUG
		if (CTurnPool::get_number_of_stored_turns() == 0) error_exit("Turnpool ist leer!!... delete_last unsinnig", 31);
	#endif

	turns.pop_back();
}
