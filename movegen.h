#ifndef MOVEGEN_H
#define MOVEGEN_H
#endif

#include <time.h>

using namespace std;

#define MYDEBUG

#define BOARD_WIDTH_SHIFT 3
#define BOARD_WIDTH (1 << BOARD_WIDTH_SHIFT)

#define BOARD_HEIGHT_SHIFT 3
#define BOARD_HEIGHT (1 << BOARD_HEIGHT_SHIFT)

#define VALID_FILE(R) ((R>=0)&&(R<BOARD_WIDTH))
#define VALID_RANK(R) ((R>=0)&&(R<BOARD_HEIGHT))

#define BOARD_SIZE BOARD_WIDTH*BOARD_HEIGHT

#define SQUARE_NONE (1<<7)

#define NUM_PIECES 6

#define KNIGHT 0
#define BISHOP 1
#define ROOK 2
#define QUEEN 3
#define KING 4
#define PAWN 5

#define MAX_PIECE (1<<8)
extern int material_values[MAX_PIECE];

// bit 6 indicates no piece
#define NO_PIECE (1 << 6)
// bit 5 indicates white piece
#define WHITE_PIECE (1 << 5)
#define WHITE WHITE_PIECE
#define BLACK_PIECE (0)
#define BLACK BLACK_PIECE
// bits 4..0 are to store piece type
#define PIECE_OF(P) (P & 31)

#define COLOR_OF(P) (P & WHITE_PIECE)
#define SIGN_OF(P) ((P & WHITE_PIECE)?1:-1)

#define FILE_MASK ((1 << BOARD_WIDTH_SHIFT)-1)
#define RANK_MASK (((1 << BOARD_HEIGHT_SHIFT)-1) << BOARD_WIDTH_SHIFT)

#define FILE_OF(SQ) (SQ & FILE_MASK)
#define RANK_OF(SQ) ((SQ & RANK_MASK) >> BOARD_WIDTH_SHIFT)

#define SQUARE_FROM_FR(F,R) (F + (R << BOARD_WIDTH_SHIFT))

#define ZERO_TYPE (0)
#define ZERO_INFO (0)
#define PAWNPUSHBYTWO (1)
#define ENPASSANT (1 << 2)
#define CASTLING (1 << 3)
#define PROMOTION (1 << 4)
#define CAPTURE (1 << 5)
#define END_VECTOR (1 << 6)
#define END_PIECE (1 << 7)

#define MATE_SCORE 10000
#define DRAW_SCORE 0

#define CASTLING_RIGHT_K (1 << 3)
#define CASTLING_RIGHT_K_DISABLE_SQUARE (63)
#define CASTLING_RIGHT_Q (1 << 2)
#define CASTLING_RIGHT_Q_DISABLE_SQUARE (56)
#define CASTLING_RIGHT_W CASTLING_RIGHT_K|CASTLING_RIGHT_Q
#define CASTLING_RIGHT_k (1 << 1)
#define CASTLING_RIGHT_k_DISABLE_SQUARE (7)
#define CASTLING_RIGHT_q (1)
#define CASTLING_RIGHT_q_DISABLE_SQUARE (0)
#define CASTLING_RIGHT_b CASTLING_RIGHT_k|CASTLING_RIGHT_q
#define CASTLING_RIGHT_K_OF(c) (c==WHITE?CASTLING_RIGHT_K:CASTLING_RIGHT_k)
#define CASTLING_RIGHT_Q_OF(c) (c==WHITE?CASTLING_RIGHT_Q:CASTLING_RIGHT_q)
#define CASTLING_EMPTYSQ_W_K_1 (61)
#define CASTLING_EMPTYSQ_W_K_2 (62)
#define CASTLING_EMPTYSQ_W_Q_1 (59)
#define CASTLING_EMPTYSQ_W_Q_2 (58)
#define CASTLING_EMPTYSQ_W_Q_3 (57)
#define CASTLING_EMPTYSQ_B_K_1 (5)
#define CASTLING_EMPTYSQ_B_K_2 (6)
#define CASTLING_EMPTYSQ_B_Q_1 (3)
#define CASTLING_EMPTYSQ_B_Q_2 (2)
#define CASTLING_EMPTYSQ_B_Q_3 (1)
#define CASTLING_RIGHT_NONE (0)
#define CASTLE_SHORT (0)
extern const char* CASTLE_SAN[2];
#define CASTLE_LONG (1)

#define WHITE_KING_CASTLE_FROM_SQUARE (60)
#define BLACK_KING_CASTLE_FROM_SQUARE (4)

const char piece_letters[]="NBRQKP";
const char black_piece_letters[]="nbrqkp";

typedef unsigned char CastlingRights;
typedef unsigned char Square;
typedef unsigned char File;
typedef unsigned char Rank;
typedef unsigned char MoveDetail;
typedef unsigned char Piece;
typedef unsigned char Color;
typedef unsigned char Bool;
typedef unsigned char Turn;
typedef unsigned char Depth;
typedef unsigned char Used;

#define True 1
#define False 0

#define SIMPLE False

////////////////////////////////////////////////////////////////////
// heuristic tuning

#define ATTACKER_BONUS (120)
#define MOBILITY_BONUS (20)

#define KNIGHT_VALUE (300)
#define BISHOP_VALUE (300)
#define ROOK_VALUE (500)
#define QUEEN_VALUE (900)
#define PAWN_VALUE (150)

////////////////////////////////////////////////////////////////////

struct Move
{
	Square from_sq;
	Square to_sq;
	MoveDetail type;
	MoveDetail info;

	int eval;
	Depth depth;
	Bool done;

	void print(Bool);
	char* algeb();

	Bool equal_to(Move);
};

extern int move_table_ptr[BOARD_SIZE][NUM_PIECES];

extern Move move_table[BOARD_SIZE*NUM_PIECES*50];

extern void init_move_table();

extern int nodes;
extern int total_used;
extern int collisions;

#define HASH_TABLE_SHIFT (17)
#define HASH_TABLE_SIZE (1 << HASH_TABLE_SHIFT)
const unsigned int HASH_TABLE_MASK=(HASH_TABLE_SIZE-1);

#define POSITION_TRUNK_SIZE (BOARD_SIZE+3)

#define MOVE_LIST_LENGTH (8)

typedef char PositionTrunk[POSITION_TRUNK_SIZE];

typedef unsigned char MoveCount;

struct MoveList
{
	MoveCount no_moves;
	MoveCount current_move;

	MoveCount last_updated;

	Move move_list[MOVE_LIST_LENGTH];

	void init();
	void add(Move,Bool);
	void init_iteration();
	Move* next_move(Bool);

	Move* get_last_updated();
};

struct HashTableEntry
{
	PositionTrunk position;
	Depth depth;
	Used used;

	MoveList best_moves;

	void init();
};

extern HashTableEntry hash_table[HASH_TABLE_SIZE];

extern void erase_hash_table();

extern Bool quit_search;

extern Bool verbose;

extern char principal_variation[200];
extern int principal_value;

struct Position
{
	char board[BOARD_SIZE];

	Turn turn;
	
	CastlingRights castling_rights;

	Square ep_square;

	Square black_king_pos;
	Square white_king_pos;

	char current_sq;
	int current_ptr;
	int pawn_status;
	int castling_status;
	int halfmove_clock;
	int fullmove_number;

	Piece prom_status;
	Bool end_pseudo_legal_moves;
	Bool end_legal_moves;
	Move current_pseudo_legal_move;

	void init_move_iterator();

	void reset();

	void next_pseudo_legal_move();
	void next_legal_move();

	int attackers_on_king(Color);

	int material_balance;
	int calc_material_balance();
	int number_of_pseudo_legal_moves(Color);
	int heuristic_eval;
	int calc_heuristic_eval();

	Square algeb_to_square(char*);
	void set_from_fen(char*);
	void print();

	Bool is_in_check(Color);
	Bool is_exploded(Color);

	Bool adjacent_kings();

	Bool is_in_check_square(Square,Color);

	void make_move(Move);

	int search_depth;
	int principal_value;

	void search();
	void i_search();
	int search_recursive(Depth,int,int,Bool);
	char* calc_principal_variation(MoveCount);
	char* score_of(int);
	void report_search_results(MoveCount);

	char* square_to_algeb(Square);

	unsigned int hash_key;
	unsigned int calc_hash_key();

	void store_move(Move,Depth,int,Bool);
	HashTableEntry* look_up();

	Move* look_up_move(Move);
};