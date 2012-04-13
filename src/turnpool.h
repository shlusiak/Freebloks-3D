#ifndef ___TURNPOOL_H____
#define ___TURNPOOL_H____

#include <stdlib.h>
#include <stdio.h>
#include "turn.h"



class CTurnpool{
	private:
		
		CTurn *m_tail;
		CTurn *m_head;
		CTurn *m_current;

	public: 

		CTurnpool();
		virtual ~CTurnpool();
		void add_turn(const int playernumber, const CStone* stone, const int y, const int x);
		void add_turn(const CTurn* turn);
		void delete_all_turns();
		

		const int get_number_of_stored_turns()const;
		CTurn* get_turn(int i);
		CTurn* get_last_turn();
		void delete_last();

};










#endif

