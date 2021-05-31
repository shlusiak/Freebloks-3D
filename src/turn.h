#ifndef ___TURN____H_
#define ___TURN____H_

#include <cstddef>
#include "stone.h"

class CBoard;
class CPlayer;

struct CTurn {
	const int player;
	const int stone_number;
	const int mirror_count;
	const int rotate_count;
	const int y;
	const int x;

	CTurn(int player, int stone_number, int mirror_count, int rotate_count, int y, int x):
			player(player),
			stone_number(stone_number),
			mirror_count(mirror_count),
			rotate_count(rotate_count),
			y(y),
			x(x) { }

	CTurn(const CTurn* turn):
			player(turn->player),
			stone_number(turn->stone_number),
			mirror_count(turn->mirror_count),
			rotate_count(turn->rotate_count),
			y(turn->y),
			x(turn->x) { }

	CTurn(const int player, const CStone* stone, const int y, const int x):
			player(player),
			stone_number(stone->get_number()),
			mirror_count(stone->get_mirror_counter()),
			rotate_count(stone->get_rotate_counter()),
			y(y),
			x(x) { }
};

#endif


