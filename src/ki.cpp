#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef WIN32
#include <windows.h>
#endif
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif
#include "ki.h"

#include "board.h"
#include "timer.h"

#define BIGGEST_X_STONES 9

/**
 * For the given stone, calculate all possible turns on the entire board.
 */
void CKi::calculate_possible_turns(const CBoard& board, const CStone& stone, const int playernumber) {
	for (int x = 0; x < board.get_field_size_x(); x++) {
		for (int y = 0; y < board.get_field_size_y(); y++) {
			if (board.get_game_field(playernumber, y, x) == FIELD_ALLOWED){
				calculate_possible_turns_in_position(board, stone, playernumber, y, x);
			}
		}
	}
}

/**
 * For the given stone and position, try all positions and calculate possible turns.
 */
void CKi::calculate_possible_turns_in_position(const CBoard& board, const CStone& stone, const int player, const int fieldY, const int fieldX) {
	int mirror;

	if (stone.get_mirrorable() == MIRRORABLE_IMPORTANT)
	    mirror = 1;
	else
	    mirror = 0;

	for (int m = 0; m <= mirror; m++) {
		for (int r = 0; r < stone.get_rotateable(); r++){
			for (int x = 0; x < stone.get_stone_size(); x++){
				for (int y = 0; y < stone.get_stone_size(); y++){
					if (stone.get_stone_field(y, x, m, r) == STONE_FIELD_ALLOWED) {
						if (board.is_valid_turn(stone, player, fieldY - y, fieldX - x)){
							m_turnpool.add_turn(player, &stone, fieldY - y, fieldX - x);
						}
					}
				}
			}
		}
	}
}

struct THREADDATA {
	CKi *ki;
	int from, to;
	int best_points;
	int current_player;
	int ki_fehler;
	const CTurn* best;
	const CBoard* board;
};

#ifdef WIN32
DWORD WINAPI kiThread(LPVOID p)
#else
void* kiThread(void* p)
#endif
{
	THREADDATA *data = (THREADDATA*)p;
	CBoard spiel(data->board->get_field_size_x(), data->board->get_field_size_y());

	int new_points;
#ifdef HAVE_PTHREAD_CREATE
	if (data->from > data->to)
	    pthread_exit((void*)0);
#else
	if (data->from > data->to)
	    return nullptr;
#endif

	spiel.follow_situation(*data->board, data->ki->m_turnpool.get_turn(data->from));
	data->best_points = CKi::get_ultimate_points(spiel, data->current_player, data->ki_fehler, data->ki->m_turnpool.get_turn(data->from));
	data->best = data->ki->m_turnpool.get_turn(data->from);

	for (int n = data->from + 1; n <= data->to; n++){
		spiel.follow_situation(*data->board, data->ki->m_turnpool.get_turn(n));
		new_points = CKi::get_ultimate_points(spiel, data->current_player, data->ki_fehler, data->ki->m_turnpool.get_turn(n));

		if (new_points >= data->best_points) {
			data->best = data->ki->m_turnpool.get_turn(n);
			data->best_points = new_points;
		}
	}
#ifdef HAVE_PTHREAD_CREATE
	pthread_exit((void*)0);
#endif
	return nullptr;
}

const CTurn* CKi::get_ultimate_turn(CBoard& spiel, const int current_player, const int ki_fehler) {
	CKi::build_up_turnpool_biggest_x_stones(spiel, current_player, BIGGEST_X_STONES);

	const CTurn* best;
	int best_points;
	int i;
#ifdef HAVE_PTHREAD_CREATE
	pthread_t threads[8];
#elif defined WIN32
	HANDLE threads[8];
#endif

	volatile THREADDATA data[8];
	if (num_threads > 8) num_threads = 8;

// 	printf("AI using %d threads\n",num_threads);
	for (i = 0; i < num_threads; i++)
	{
		data[i].ki = this;
		data[i].best = nullptr;
		data[i].best_points = 0;
		data[i].current_player = current_player;
		data[i].ki_fehler = ki_fehler;
		data[i].board = &spiel;

		data[i].from=2+ i * (CKi::m_turnpool.size() - 1) / num_threads;
		data[i].to= 2 + (i+1) * (CKi::m_turnpool.size() - 1) / num_threads - 1;
		if (i == num_threads - 1) data[i].to= CKi::m_turnpool.size();

#ifdef WIN32
		DWORD tid;
		threads[i]=CreateThread(nullptr,0,kiThread,(void*)&data[i],0,&tid);
#elif defined HAVE_PTHREAD_CREATE
		pthread_create(&threads[i],nullptr,kiThread,(void*)&data[i]);
#else
		kiThread((void*)&data[i]);
#endif
	}

	CBoard follow_situation(data->board->get_field_size_x(), data->board->get_field_size_y());
	best = CKi::m_turnpool.get_turn(1);
	follow_situation.follow_situation(spiel, best);

	best_points = get_ultimate_points(follow_situation, current_player, ki_fehler, best); //Bewertung hier!!!

	for (i = 0; i < num_threads; i++)
	{
#ifdef HAVE_PTHREAD_CREATE
		int *pi;
		pthread_join(threads[i],(void**)&pi);
#elif defined WIN32
		WaitForSingleObject(threads[i],INFINITE);
#endif
		if (data[i].best_points>best_points && data[i].best!=nullptr)
		{
			best_points=data[i].best_points;
			best=data[i].best;
		}
	}

	return best;
}

void CKi::build_up_turnpool_biggest_x_stones(CBoard& spiel, const int playernumber, const int max_stored_stones){
	m_turnpool.delete_all_turns();
	int stored_stones = 0;
	int stored_turns = 0;

	for (int n = STONE_COUNT_ALL_SHAPES -1; n >= 0; n--){
		const CStone& stone = spiel.get_player(playernumber)->get_stone(n);

		if (stone.get_available()){
			calculate_possible_turns(spiel, stone, playernumber);
			if (m_turnpool.size() > stored_turns){
				stored_stones++;
				stored_turns = m_turnpool.size();
				if (stored_stones >= max_stored_stones) {
					return;
				}
			}
		}
	}
}

int CKi::get_distance_points(CBoard& follow_situation, const int playernumber, const CTurn& turn) {
	const CStone& stone = follow_situation.get_player(playernumber)->get_stone(turn.stone_number);

	return
		abs(follow_situation.get_player_start_x(playernumber) - turn.x + stone.get_stone_size() / 2)
		+
		abs(follow_situation.get_player_start_y(playernumber) - turn.y + stone.get_stone_size() / 2);
}

int CKi::get_ultimate_points(CBoard& follow_situation, const int playernumber, const int ai_error, const CTurn& turn){
	int sum = 0;
	for (int p = 0; p < PLAYER_MAX; p++){
		if (p != playernumber){
			if (p != follow_situation.get_teammate(playernumber)){
                sum -= follow_situation.get_position_points(p);
			}
		} else {
            sum += follow_situation.get_position_points(p);
            sum -= follow_situation.get_stone_points_left(p) * 175;
		}
	}
    sum += get_distance_points(follow_situation, playernumber, turn) * 20;
	return ((100+(rand() % ((ai_error) + 1))) * sum) / 100;
}

const CTurn* CKi::get_ki_turn(CBoard& spiel, const int player, int ai_error) {
	if (spiel.get_number_of_possible_turns(player) == 0) return nullptr;
	return CKi::get_ultimate_turn(spiel, player, ai_error);
}
