#ifndef ___PLAYER____H_
#define ___PLAYER____H_

#include <stdlib.h>
#include "stone.h"
#include "turn.h"
#include "turnpool.h"



class CBoard;

class CPlayer{

	private:

		int m_stone_points_left;
		int m_stone_count;
		int m_number_of_possible_turns;
		int m_position_points;

		int m_teammate;
		int m_nemesis;
		int m_number;

		CStone m_stone[STONE_COUNT_ALL_SHAPES];
	public:
		CPlayer():m_stone_points_left(0), m_stone_count(0), m_number_of_possible_turns(0), m_position_points(0),
		          m_teammate(-1), m_nemesis(-1), m_number(0) {}

		void init(const CBoard& board, const int playernumber);
		void refresh_data(const CBoard& board);

		int get_number() const; //now returns a value from 0 to 3!
		int get_stone_points_left() const;
		int get_position_points() const; //returns an evaluation of the position
		int get_number_of_possible_turns() const;
		int get_stone_count() const;

		int get_teammate() const;
		int get_nemesis() const;
		void set_teammate(int playernumber);
		void set_nemesis(int playernumber);

		CStone& get_stone(int number){
			return m_stone[number];
		}
};

inline
int CPlayer::get_number_of_possible_turns()const{
	return m_number_of_possible_turns;
}

inline
int CPlayer::get_stone_points_left()const{
	return m_stone_points_left;
}


inline
int CPlayer::get_number()const{
	return CPlayer::m_number;
}


inline
int CPlayer::get_position_points()const{
	return CPlayer::m_position_points;
}


inline
int CPlayer::get_stone_count()const{
	return CPlayer::m_stone_count;
}

inline
int CPlayer::get_teammate()const{
	return CPlayer::m_teammate;
}

inline
int CPlayer::get_nemesis()const{
	return CPlayer::m_nemesis;
}

inline
void CPlayer::set_teammate(int playernumber){
	CPlayer::m_teammate = playernumber;
}

inline
void CPlayer::set_nemesis(int playernumber){
	CPlayer::m_nemesis = playernumber;
}



#endif

