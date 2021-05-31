#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <cstddef>
#include "turnpool.h"

void CTurnPool::add_turn(const int player, const int stone, const int y, const int x, const int mirrored, const int rotation) {
	CTurn* new_element = new CTurn(player, stone, mirrored, rotation, y, x);
	turns.push_back(new_element);
}

void CTurnPool::delete_last(){
	#ifdef _DEBUG
		if (CTurnPool::get_number_of_stored_turns() == 0) error_exit("Turnpool ist leer!!... delete_last unsinnig", 31);
	#endif

	turns.pop_back();
}
