#ifndef ___TURNPOOL_H____
#define ___TURNPOOL_H____

#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "turn.h"

class CTurnPool {
	private:
        std::vector<CTurn*> turns;

	public:
		CTurnPool() {};

		~CTurnPool() {
		    delete_all_turns();
		}

		/* Owns its turns, a copy would delete them twice */
		CTurnPool(const CTurnPool &) = delete;
		CTurnPool &operator=(const CTurnPool &) = delete;

		void add_turn(const int player, const int stone, const int y, const int x, const int mirrored, const int rotation);

		void delete_all_turns() {
			for (auto turn: turns) {
				delete turn;
			}
			turns.clear();
		}

		int size() const {
		    return turns.size();
		}

		const CTurn* get_turn(int number) const {
			return turns[number - 1];
		}

		const CTurn* get_last_turn() const {
			if (turns.empty()) return nullptr;
			return turns.back();
		}

		void delete_last();
};

#endif

