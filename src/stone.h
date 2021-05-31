#ifndef ___STONE____H__
#define ___STONE____H__

#include "constants.h"

/* Stone-Constants */

const int STONE_SIZE[STONE_COUNT_ALL_SHAPES] =
					{
						1, //0
						2, //1
						2, //2
						3, //3
						2, //4
						3, //5
						3, //6
						3, //7
						4, //8
						3, //9
						3, //10
						3, //11
						3, //12
						3, //13
						3, //14
						3, //15
						3, //16
						4, //17
						4, //18
						4, //19
						5  //20
					};


const int STONE_POSITION_POINTS[STONE_COUNT_ALL_SHAPES] =
					{
						8, //0
						4, //1
						2, //2
						4, //3
						8, //4
						1, //5
						2, //6
						2, //7
						4, //8
						1, //9
						2, //10
						2, //11
						2, //12
						2, //13
						1, //14
						1, //15
						8, //16
						1, //17
						1, //18
						1, //19
						4  //20
					};



const int STONE_POINTS[STONE_COUNT_ALL_SHAPES] =
					{
						1, //0
						2, //1
						3, //2
						3, //3
						4, //4
						4, //5
						4, //6
						4, //7
						4, //8
						5, //9
						5, //10
						5, //11
						5, //12
						5, //13
						5, //14
						5, //15
						5, //16
						5, //17
						5, //18
						5, //19
						5  //20
					};

const int STONE_MIRRORABLE[STONE_COUNT_ALL_SHAPES] =
					{
						MIRRORABLE_NOT,			//0
						MIRRORABLE_NOT,			//1
						MIRRORABLE_OPTIONAL,	//2
						MIRRORABLE_NOT,			//3
						MIRRORABLE_NOT,			//4
						MIRRORABLE_IMPORTANT,	//5
						MIRRORABLE_OPTIONAL,	//6
						MIRRORABLE_IMPORTANT,	//7
						MIRRORABLE_NOT,			//8
						MIRRORABLE_IMPORTANT,	//9
						MIRRORABLE_OPTIONAL,	//10
						MIRRORABLE_OPTIONAL,	//11
						MIRRORABLE_OPTIONAL,	//12
						MIRRORABLE_OPTIONAL,	//13
						MIRRORABLE_IMPORTANT,	//14
						MIRRORABLE_IMPORTANT,	//15
						MIRRORABLE_NOT,			//16
						MIRRORABLE_IMPORTANT,	//17
						MIRRORABLE_IMPORTANT,	//18
						MIRRORABLE_IMPORTANT,	//19
						MIRRORABLE_NOT			//20
					};

const int STONE_ROTATEABLE[STONE_COUNT_ALL_SHAPES] =
					{
						ROTATEABLE_NOT,		//0
						ROTATEABLE_TWO,		//1
						ROTATEABLE_FOUR,	//2
						ROTATEABLE_TWO,		//3
						ROTATEABLE_NOT,		//4
						ROTATEABLE_FOUR,	//5
						ROTATEABLE_FOUR,	//6
						ROTATEABLE_TWO,		//7
						ROTATEABLE_TWO,		//8
						ROTATEABLE_FOUR,	//9
						ROTATEABLE_FOUR,	//10
						ROTATEABLE_FOUR,	//11
						ROTATEABLE_FOUR,	//12
						ROTATEABLE_FOUR,	//13
						ROTATEABLE_TWO,		//14
						ROTATEABLE_FOUR,	//15
						ROTATEABLE_NOT,		//16
						ROTATEABLE_FOUR,	//17
						ROTATEABLE_FOUR,	//18
						ROTATEABLE_FOUR,	//19
						ROTATEABLE_TWO		//20
					};

const TStoneField STONE_FIELD[STONE_COUNT_ALL_SHAPES]=
					{
					  {1,8,8,8,8,	//0
						8,8,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,1,8,8,8,	//1
						0,1,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{1,0,8,8,8,	//2
						1,1,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,1,0,8,8,	//3
						0,2,0,8,8,
						0,1,0,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{1,1,8,8,8,	//4
						1,1,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,1,0,8,8,	//5
						0,2,0,8,8,
						1,1,0,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,1,0,8,8,	//6
						0,2,1,8,8,
						0,1,0,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,0,0,8,8,	//7
						1,1,0,8,8,
						0,1,1,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,0,1,0,8,	//8
						0,0,2,0,8,
						0,0,2,0,8,
						0,0,1,0,8,
						8,8,8,8,8,},

						{0,1,0,8,8,	//9
						1,2,0,8,8,
						1,1,0,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{1,1,0,8,8,	//10
						0,2,0,8,8,
						1,1,0,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,1,0,8,8,	//11
						0,2,0,8,8,
						1,2,1,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{1,0,0,8,8,	//12
						2,0,0,8,8,
						1,2,1,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{1,1,0,8,8,	//13
						0,1,1,8,8,
						0,0,1,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{1,0,0,8,8,	//14
						1,2,1,8,8,
						0,0,1,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{1,0,0,8,8,	//15
						1,2,1,8,8,
						0,1,0,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,1,0,8,8,	//16
						1,2,1,8,8,
						0,1,0,8,8,
						8,8,8,8,8,
						8,8,8,8,8,},

						{0,0,1,0,8,	//17
						0,0,2,0,8,
						0,0,2,0,8,
						0,1,1,0,8,
						8,8,8,8,8,},

						{0,0,1,0,8,	//18
						0,0,2,0,8,
						0,1,1,0,8,
						0,1,0,0,8,
						8,8,8,8,8,},

						{0,1,0,0,8,	//19
						0,2,1,0,8,
						0,2,0,0,8,
						0,1,0,0,8,
						8,8,8,8,8,},

						{0,0,1,0,0,	//20
						0,0,2,0,0,
						0,0,2,0,0,
						0,0,2,0,0,
						0,0,1,0,0},
					};


/* ende Stone-Constants */


class CBoard;

struct Orientation {
    int mirrored = 0;
    int rotation = 0;

    Orientation() {}
    Orientation(int mirrored, int rotation): mirrored(mirrored), rotation(rotation) {}

    inline void rotate_left(int max_rotation) {
		rotation--;
		if (rotation < 0)
			rotation += max_rotation;
    }

    inline void rotate_right(int max_rotation) {
		rotation = (rotation + 1) % max_rotation;
	}

	inline void mirror_over_x(int mirrorable, int max_rotation) {
		if (mirrorable == MIRRORABLE_NOT) return;
		mirrored = (mirrored + 1) % 2;
		if (rotation % 2 == 1)
			rotation = (rotation + 2) % max_rotation;
	}

	inline void mirror_over_y(int mirrorable, int max_rotation) {
		if (mirrorable == MIRRORABLE_NOT) return;
		mirrored = (mirrored + 1) % 2;
		if (rotation % 2 == 0)
			rotation = (rotation + 2) % max_rotation;
	}
};

struct Shape {
    int shape;
    int size;
    int mirrorable;
    int rotateable;
    int points;
    int position_points;

    Shape(int shape):
    	shape(shape),
    	size(STONE_SIZE[shape]),
    	mirrorable(STONE_MIRRORABLE[shape]),
    	rotateable(STONE_ROTATEABLE[shape]),
    	points(STONE_POINTS[shape]),
    	position_points(STONE_POSITION_POINTS[shape]) {}

    inline bool is_position_inside_stone(const int y, const int x) const{
        if (y < 0 || y >= size || x < 0 || x >= size) return false;
        return true;
    }

	TSingleStone get_field(const int y, const int x, const Orientation& orientation) const {
    	return get_field(y, x, orientation.mirrored, orientation.rotation);
    }

	TSingleStone get_field(const int y, const int x, const int mirrored, const int rotation) const;
};

class CStone {
	private:
		int m_available;

		Shape shape;
		Orientation orientation;

	public:
		CStone(): m_available(0), shape(0) {}
		void init (const int shape);

		const Shape& get_shape() const { return shape; }

		inline TSingleStone get_stone_field(const int y, const int x, const Orientation &orientation2) const {
			return shape.get_field(y, x, orientation2);
		}

		inline TSingleStone get_stone_field(const int y, const int x) const {
			return shape.get_field(y, x, orientation.mirrored, orientation.rotation);
		}

		int calculate_possible_turns_in_position(const CBoard& spiel, const int playernumber, const int fieldY, const int fieldX) const;

		inline const int get_stone_size() const {
			return shape.size;
		}
		inline const int get_stone_points() const {
			return shape.points;
		}
		inline const int get_stone_shape() const {
			return shape.shape;
		}
		inline const int get_rotateable() const {
			return shape.rotateable;
		}
		inline const int get_mirrorable() const {
			return shape.mirrorable;
		}
		inline const int get_rotate_counter() const {
			return orientation.rotation;
		}
		inline const int get_mirror_counter() const {
			return orientation.mirrored;
		}
		inline const int get_stone_position_points() const {
			return shape.position_points;
		}
		inline const int get_available() const {
			return m_available;
		}

		void set_available(const int value);
		void available_decrement();
		void available_increment();

		inline void rotate_left() { orientation.rotate_left(shape.rotateable); }
		inline void rotate_right() { orientation.rotate_right(shape.rotateable); }
		inline void mirror_over_x() { orientation.mirror_over_x(shape.mirrorable, shape.rotateable); }
		inline void mirror_over_y() { orientation.mirror_over_y(shape.mirrorable, shape.rotateable); }
		inline void mirror_rotate_to(const int mirror_counter, const int rotate_counter) {
		    set_orientation(Orientation(mirror_counter, rotate_counter));
		}
		inline void set_orientation(const Orientation&& orientation2) { orientation = orientation2; }
};

inline
void CStone::set_available(const int value){
	m_available = value;
}

inline
void CStone::available_increment(){
	m_available++;
}

inline
void CStone::available_decrement(){
	m_available--;
}

#endif
