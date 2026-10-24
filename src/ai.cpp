#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef WIN32
#include <windows.h>
#endif
#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#endif
#include "ai.h"

#include "board.h"
#include "timer.h"

#define BIGGEST_X_STONES 9


static int get_distance_points(CBoard& follow_situation, const int playernumber, const CTurn& turn) {
    const CStone& stone = follow_situation.get_player(playernumber)->get_stone(turn.stone_number);

    return
            abs(follow_situation.get_player_start_x(playernumber) - turn.x + stone.get_stone_size() / 2)
            +
            abs(follow_situation.get_player_start_y(playernumber) - turn.y + stone.get_stone_size() / 2);
}

/* The worker threads and the calling thread score turns concurrently; rand() shares one hidden
   state between them. Use a per-thread state where rand_r() is available. */
static int ai_random(int max_exclusive) {
    if (max_exclusive <= 0) return 0;
#ifdef WIN32
    return rand() % max_exclusive;
#else
    static thread_local unsigned int seed = 0;
    if (seed == 0) seed = (unsigned int)rand() | 1;
    return rand_r(&seed) % max_exclusive;
#endif
}

static int get_ultimate_points(CBoard& follow_situation, const int playernumber, const int ai_error, const CTurn& turn) {
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
    return ((100+ai_random(ai_error + 1)) * sum) / 100;
}

/**
 * For the given stone, calculate all possible turns on the entire board.
 */
void CAI::calculate_possible_turns(const CBoard& board, const Shape& shape, const int playernumber) {
	for (int x = 0; x < board.get_field_size_x(); x++) {
		for (int y = 0; y < board.get_field_size_y(); y++) {
			if (board.get_game_field(playernumber, y, x) == FIELD_ALLOWED){
				calculate_possible_turns_in_position(board, shape, playernumber, y, x);
			}
		}
	}
}

/**
 * For the given stone and position, try all positions and calculate possible turns.
 */
void CAI::calculate_possible_turns_in_position(const CBoard& board, const Shape& shape, const int player, const int fieldY, const int fieldX) {
	int mirror;

	if (shape.mirrorable == MIRRORABLE_IMPORTANT)
	    mirror = 1;
	else
	    mirror = 0;

	for (int m = 0; m <= mirror; m++) {
		for (int r = 0; r < shape.rotateable; r++){
			for (int x = 0; x < shape.size; x++){
				for (int y = 0; y < shape.size; y++){
					if (shape.get_field(y, x, m, r) == STONE_FIELD_ALLOWED) {
						if (board.is_valid_turn(shape, player, fieldY - y, fieldX - x, m, r)){
							m_turnpool.add_turn(player, shape.shape, fieldY - y, fieldX - x, m, r);
						}
					}
				}
			}
		}
	}
}

struct THREADDATA {
	CAI *ai;
	int from, to;
	int best_points;
	int current_player;
	int ai_error;
	const CTurn* best;
	const CBoard* board;
};

#ifdef WIN32
DWORD WINAPI aiThread(LPVOID p)
#else
void* aiThread(void* p)
#endif
{
    auto data = (THREADDATA*)p;
    auto ai = data->ai;
    const CBoard* original = data->board;
	CBoard board(original->get_field_size_x(), original->get_field_size_y());

	int new_points;
	/* Return instead of pthread_exit(): it is not guaranteed to run the destructor of board */
	if (data->from > data->to)
	    return nullptr;

	board.follow_situation(*original, *ai->m_turnpool.get_turn(data->from));
	data->best_points = get_ultimate_points(board, data->current_player, data->ai_error, *data->ai->m_turnpool.get_turn(data->from));
	data->best = ai->m_turnpool.get_turn(data->from);

	for (int n = data->from + 1; n <= data->to; n++){
	    const CTurn* turn = data->ai->m_turnpool.get_turn(n);

		board.follow_situation(*original, *turn);
		new_points = get_ultimate_points(board, data->current_player, data->ai_error, *turn);

		if (new_points >= data->best_points) {
			data->best = turn;
			data->best_points = new_points;
		}
	}

	return nullptr;
}

const CTurn* CAI::get_ultimate_turn(CBoard& board, const int current_player, const int ai_error) {
	build_up_turnpool_biggest_x_stones(board, current_player, BIGGEST_X_STONES);

	const CTurn* best;
	int best_points;
	int i;
#ifdef HAVE_PTHREAD_CREATE
	pthread_t threads[8];
#elif defined WIN32
	HANDLE threads[8];
#endif

	volatile THREADDATA data[8];
	/* false if the thread could not be created and the slice was computed right here */
	bool started[8];
	if (num_threads > 8) num_threads = 8;
	if (num_threads < 1) num_threads = 1;

// 	printf("AI using %d threads\n",num_threads);
	for (i = 0; i < num_threads; i++)
	{
		data[i].ai = this;
		data[i].best = nullptr;
		data[i].best_points = 0;
		data[i].current_player = current_player;
		data[i].ai_error = ai_error;
		data[i].board = &board;

		// skip the first, because we are doing that ourselves in this thread.
		data[i].from = 2 + i * (m_turnpool.size() - 1) / num_threads;
		data[i].to = 2 + (i+1) * (m_turnpool.size() - 1) / num_threads - 1;
		if (i == num_threads - 1) data[i].to = m_turnpool.size();

#ifdef WIN32
		DWORD tid;
		threads[i]=CreateThread(nullptr,0,aiThread,(void*)&data[i],0,&tid);
		started[i]=(threads[i]!=nullptr);
#elif defined HAVE_PTHREAD_CREATE
		started[i]=(pthread_create(&threads[i],nullptr,aiThread,(void*)&data[i])==0);
#else
		started[i]=false;
#endif
		/* Without a thread, evaluate this slice synchronously instead of dropping it */
		if (!started[i])
			aiThread((void*)&data[i]);
	}

	CBoard follow_situation(board.get_field_size_x(), board.get_field_size_y());
	best = m_turnpool.get_turn(1);
	follow_situation.follow_situation(board, *best);

	best_points = get_ultimate_points(follow_situation, current_player, ai_error, *best);

	for (i = 0; i < num_threads; i++)
	{
#ifdef HAVE_PTHREAD_CREATE
		int *pi;
		if (started[i])pthread_join(threads[i], (void**)&pi);
#elif defined WIN32
		if (started[i])WaitForSingleObject(threads[i],INFINITE);
#endif
		if (data[i].best_points > best_points && data[i].best != nullptr)
		{
			best_points = data[i].best_points;
			best = data[i].best;
		}
	}

	return best;
}

void CAI::build_up_turnpool_biggest_x_stones(CBoard& board, const int playernumber, const int max_stored_stones) {
	m_turnpool.delete_all_turns();

	int stored_stones = 0;
	int stored_turns = 0;

	for (int n = STONE_COUNT_ALL_SHAPES - 1; n >= 0; n--) {
		const CStone& stone = board.get_player(playernumber)->get_stone(n);

		if (stone.get_available()) {
			calculate_possible_turns(board, stone.get_shape(), playernumber);

			// If this stone has any possible turns, we increment the stored_stones counter
			if (m_turnpool.size() > stored_turns) {
				stored_stones++;
				stored_turns = m_turnpool.size();
				// This way we only use turns from the biggest n stones and can skip processing smaller stones
				if (stored_stones >= max_stored_stones) {
					return;
				}
			}
		}
	}
}

const CTurn* CAI::get_ai_turn(CBoard& board, const int player, int ai_error) {
	if (board.get_number_of_possible_turns(player) == 0) return nullptr;
	return CAI::get_ultimate_turn(board, player, ai_error);
}
