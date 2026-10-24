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
	if (turns.empty()) return;

	/* The pool owns its turns, see delete_all_turns() */
	delete turns.back();
	turns.pop_back();
}
