#include "movegen.h"

#include <iostream>

#include "xxhash.h"

using namespace std;

int material_values[MAX_PIECE];

Bool quit_search;

Bool verbose;

int move_table_ptr[BOARD_SIZE][NUM_PIECES];

Move move_table[BOARD_SIZE*NUM_PIECES*50];

HashTableEntry hash_table[HASH_TABLE_SIZE];

char principal_variation[200];

const char* CASTLE_SAN[2]={"O-O","O-O-O"};

void Position::init_move_iterator()
{
	current_sq=0;
	current_ptr=0;
	pawn_status=0;
	prom_status=0;
	castling_status=0;
}

void erase_hash_table()
{
	memset((void*)&hash_table,0,HASH_TABLE_SIZE*sizeof(HashTableEntry));
}

void init_move_table()
{

	erase_hash_table();
	
	material_values[WHITE|KNIGHT]=KNIGHT_VALUE;
	material_values[WHITE|BISHOP]=BISHOP_VALUE;
	material_values[WHITE|ROOK]=ROOK_VALUE;
	material_values[WHITE|QUEEN]=QUEEN_VALUE;
	material_values[WHITE|KING]=0;
	material_values[WHITE|PAWN]=PAWN_VALUE;

	material_values[KNIGHT]=-KNIGHT_VALUE;
	material_values[BISHOP]=-BISHOP_VALUE;
	material_values[ROOK]=-ROOK_VALUE;
	material_values[QUEEN]=-QUEEN_VALUE;
	material_values[KING]=0;
	material_values[PAWN]=-PAWN_VALUE;

	int ptr=0;

	for(Square sq=0;sq<BOARD_SIZE;sq++)
	{
		int file=sq & FILE_MASK;
		int rank=(sq & RANK_MASK) >> BOARD_WIDTH_SHIFT;

		for(Piece pc=0;pc<NUM_PIECES;pc++)
		{

			move_table_ptr[sq][pc]=ptr;

			if(pc==KNIGHT)
			{
				for(int i=-2;i<=2;i++)
				{
					for(int j=-2;j<=2;j++)
					{
						
						if((abs(i*j)==2)&&(VALID_FILE(file+i))&&(VALID_RANK(rank+j)))
						{
							File to_file=file+i;
							Rank to_rank=rank+j;

							Square to_sq=SQUARE_FROM_FR(to_file,to_rank);

							move_table[ptr].from_sq=sq;
							move_table[ptr].to_sq=to_sq;
							move_table[ptr].type=END_VECTOR;
							ptr++;
						}
					}
				}

				move_table[ptr].type=END_PIECE;
				ptr++;

			}

			if((pc==BISHOP)||(pc==ROOK)||(pc==QUEEN)||(pc==KING))
			{
				int last_move=-1;
				for(int i=-1;i<=1;i++)
				{
					for(int j=-1;j<=1;j++)
					{
						if(
							((pc==BISHOP)&&(abs(i*j)==1))
							||
							((pc==ROOK)&&(i*j==0)&&((abs(i)+abs(j))>0))
							||
							(((pc==QUEEN)||(pc==KING))&&((abs(i)+abs(j))>0))
						)
						{
							int step=1;
							char valid;
							do
							{
								valid=(VALID_FILE(file+step*i))&&(VALID_RANK(rank+step*j));
								if(valid)
								{
									File to_file=file+step*i;
									Rank to_rank=rank+step*j;

									Square to_sq=SQUARE_FROM_FR(to_file,to_rank);

									move_table[ptr].from_sq=sq;
									move_table[ptr].to_sq=to_sq;
									move_table[ptr].type=(pc!=KING)?ZERO_TYPE:END_VECTOR;
									last_move=ptr;
									ptr++;
									step++;
								}
								else if(last_move>=0)
								{
									move_table[last_move].type|=END_VECTOR;
									last_move=-1;
								}
							}while(valid&&(pc!=KING));
						}
					}
				}

				move_table[ptr].type=END_PIECE;
				ptr++;

			}

			if(pc==PAWN)
			{
				move_table[ptr].type=END_PIECE;
				ptr++;
			}

		}
	}
}

void Position::set_from_fen(char* fen)
{
	Square sq=0;
	char f;

	memset(board,NO_PIECE,BOARD_SIZE);

	black_king_pos=0;
	white_king_pos=0;

	init_move_iterator();

	// defaults
	turn=WHITE;
	castling_rights=CASTLING_RIGHT_NONE;
	ep_square=SQUARE_NONE;
	halfmove_clock=0;
	fullmove_number=1;

	do
	{
		f=*fen++;
		if((f>='1')&&(f<='9'))
		{
			f-='0';
			while(f--)
			{
				board[sq++]=NO_PIECE;
			}
		}
		else
		{
			switch(f)
			{
				case 'N': board[sq++]=WHITE_PIECE|KNIGHT;break;
				case 'n': board[sq++]=BLACK_PIECE|KNIGHT;break;
				case 'B': board[sq++]=WHITE_PIECE|BISHOP;break;
				case 'b': board[sq++]=BLACK_PIECE|BISHOP;break;
				case 'R': board[sq++]=WHITE_PIECE|ROOK;break;
				case 'r': board[sq++]=BLACK_PIECE|ROOK;break;
				case 'Q': board[sq++]=WHITE_PIECE|QUEEN;break;
				case 'q': board[sq++]=BLACK_PIECE|QUEEN;break;
				case 'K':
						{
							board[sq]=WHITE_PIECE|KING;
							white_king_pos=sq++;
							break;
						}
				case 'k':
						{
							board[sq]=BLACK_PIECE|KING;
							black_king_pos=sq++;
							break;
						}
				case 'P': board[sq++]=WHITE_PIECE|PAWN;break;
				case 'p': board[sq++]=BLACK_PIECE|PAWN;break;
			}
		}
	}while(f&&(f!=' ')&&(sq<BOARD_SIZE));

	while(f&&(f!=' ')){f=*fen++;};

	if(f!=' ')
	{
		// invalid fen, use defaults
		return;
	}

	f=*fen++;

	turn=f=='w'?WHITE:BLACK;

	f=*fen++;

	if(f!=' ')
	{
		// invalid fen, use defaults
		return;
	}

	f=*fen++;

	while(f&&(f!=' '))
	{
		switch(f)
		{
			case 'K': castling_rights|=CASTLING_RIGHT_K;break;
			case 'Q': castling_rights|=CASTLING_RIGHT_Q;break;
			case 'k': castling_rights|=CASTLING_RIGHT_k;break;
			case 'q': castling_rights|=CASTLING_RIGHT_q;break;
		}
		f=*fen++;
	}

	if(f!=' ')
	{
		// invalid fen, use defaults
		return;
	}

	f=*fen;

	if(f!='-')
	{
		ep_square=algeb_to_square(fen);
	}

	// ignore halfmove clock

	// ignore fullmove number

}

unsigned int Position::calc_hash_key()
{
	hash_key=XXH32((void*)this,sizeof(PositionTrunk),0);

	hash_key&=HASH_TABLE_MASK;

	return hash_key;
}

void Position::print()
{

	cout << endl;

	for(Square sq=0;sq<BOARD_SIZE;sq++)
	{
		Piece p=board[sq];

		if(FILE_OF(sq)==0)
		{
			cout << (char)(BOARD_HEIGHT-1-RANK_OF(sq)+'1') << "  ";
		}

		if(p!=NO_PIECE)
		{
			if(p & WHITE_PIECE)
			{
				cout << piece_letters[p & ~WHITE_PIECE];
			}
			else
			{
				cout << black_piece_letters[p];
			}
		}
		else
		{
			cout << ".";
		}

		if(FILE_OF(sq)==FILE_MASK)
		{
			cout << endl;
		}
	}

	cout << endl << "   ";

	for(File f=0;f<BOARD_WIDTH;f++)
	{
		cout << (char)(f+'a');
	}

	string side_to_move=(turn==WHITE?"white":"black");

	HashTableEntry* entry=look_up();

	cout 
		<< endl 
		<< endl 
		<< "turn: [" << side_to_move.c_str() 
			<< "] , castling rights: [" 
			<< (castling_rights&CASTLING_RIGHT_K?"K":"")
			<< (castling_rights&CASTLING_RIGHT_Q?"Q":"")
			<< (castling_rights&CASTLING_RIGHT_k?"k":"")
			<< (castling_rights&CASTLING_RIGHT_q?"q":"")
			<< "] , ep square: [" 
			<< (ep_square&SQUARE_NONE?"":square_to_algeb(ep_square))
			<< "]"
		<< endl 
		<< "material balance " << calc_material_balance() << endl
		<< "attackers on white king " << attackers_on_king(WHITE) << endl
		//<< "white king in check? ( assuming black's turn ) " << (int)is_in_check(WHITE) << endl
		<< "attackers on black king " << attackers_on_king(BLACK) << endl
		//<< "black king in check? ( assuming white's turn ) " << (int)is_in_check(BLACK) << endl
		<< "mobility white " << number_of_pseudo_legal_moves(WHITE) << endl
		<< "mobility black " << number_of_pseudo_legal_moves(BLACK) << endl
		<< "heuristic value " << calc_heuristic_eval() << endl
		<< endl;

	if(entry!=NULL)
	{
		cout << "best moves in the position: " << endl << endl;

		MoveList book=entry->best_moves;

		book.init_iteration();

		for(int i=0;i<book.no_moves;i++)
		{
			Move* m=book.next_move(turn);

			cout << (i+1) << ". " << m->algeb()	<< " ( " << m->eval << " ) " << endl;
		}

		cout << endl;
	}
}

Bool Position::is_exploded(Color c)
{
	return c==WHITE?(board[white_king_pos]!=(WHITE_PIECE|KING)):(board[black_king_pos]!=(BLACK_PIECE|KING));
}

void Position::next_pseudo_legal_move()
{
	end_pseudo_legal_moves=False;

	if(is_exploded(turn))
	{
		end_pseudo_legal_moves=True;
		return;
	}

	if(castling_status==0)
	{

		castling_status++;

		if(castling_rights & CASTLING_RIGHT_K_OF(turn))
		{

			if(turn==WHITE)
			{

				if(
					(castling_rights & CASTLING_RIGHT_K)
					&&
					(board[CASTLING_EMPTYSQ_W_K_1] & NO_PIECE)
					&&
					// cannot castle through check
					(!is_in_check_square(CASTLING_EMPTYSQ_W_K_1,WHITE))
					&&
					(board[CASTLING_EMPTYSQ_W_K_2] & NO_PIECE)
					&&
					// cannot castle in check
					(!is_in_check_square(WHITE_KING_CASTLE_FROM_SQUARE,WHITE))
				)
				{
					current_pseudo_legal_move.from_sq=WHITE_KING_CASTLE_FROM_SQUARE;
					current_pseudo_legal_move.to_sq=CASTLING_EMPTYSQ_W_K_2;
					current_pseudo_legal_move.type=CASTLING;
					current_pseudo_legal_move.info=CASTLE_SHORT;
					return;
				}

			}
			else
			{
				if(
					(castling_rights & CASTLING_RIGHT_k)
					&&
					(board[CASTLING_EMPTYSQ_B_K_1] & NO_PIECE)
					&&
					// cannot castle through check
					(!is_in_check_square(CASTLING_EMPTYSQ_B_K_1,BLACK))
					&&
					(board[CASTLING_EMPTYSQ_B_K_2] & NO_PIECE)
					&&
					// cannot castle in check
					(!is_in_check_square(BLACK_KING_CASTLE_FROM_SQUARE,BLACK))
				)
				{
					current_pseudo_legal_move.from_sq=BLACK_KING_CASTLE_FROM_SQUARE;
					current_pseudo_legal_move.to_sq=CASTLING_EMPTYSQ_B_K_2;
					current_pseudo_legal_move.type=CASTLING;
					current_pseudo_legal_move.info=CASTLE_SHORT;
					return;
				}
			}

		}

	}

	if(castling_status==1)
	{

		castling_status++;

		if(castling_rights & CASTLING_RIGHT_Q_OF(turn))
		{

			if(turn==WHITE)
			{

				if(
					(castling_rights & CASTLING_RIGHT_Q)
					&&
					(board[CASTLING_EMPTYSQ_W_Q_1] & NO_PIECE)
					&&
					// cannot castle through check
					(!is_in_check_square(CASTLING_EMPTYSQ_W_Q_1,WHITE))
					&&
					(board[CASTLING_EMPTYSQ_W_Q_2] & NO_PIECE)
					&&
					(board[CASTLING_EMPTYSQ_W_Q_3] & NO_PIECE)
					&&
					// cannot castle in check
					(!is_in_check_square(WHITE_KING_CASTLE_FROM_SQUARE,WHITE))
				)
				{
					current_pseudo_legal_move.from_sq=WHITE_KING_CASTLE_FROM_SQUARE;
					current_pseudo_legal_move.to_sq=CASTLING_EMPTYSQ_W_Q_2;
					current_pseudo_legal_move.type=CASTLING;
					current_pseudo_legal_move.info=CASTLE_LONG;
					return;
				}

			}
			else
			{
				if(
					(castling_rights & CASTLING_RIGHT_q)
					&&
					(board[CASTLING_EMPTYSQ_B_Q_1] & NO_PIECE)
					&&
					// cannot castle through check
					(!is_in_check_square(CASTLING_EMPTYSQ_B_Q_1,BLACK))
					&&
					(board[CASTLING_EMPTYSQ_B_Q_2] & NO_PIECE)
					&&
					(board[CASTLING_EMPTYSQ_B_Q_3] & NO_PIECE)
					&&
					// cannot castle in check
					(!is_in_check_square(BLACK_KING_CASTLE_FROM_SQUARE,BLACK))
				)
				{
					current_pseudo_legal_move.from_sq=BLACK_KING_CASTLE_FROM_SQUARE;
					current_pseudo_legal_move.to_sq=CASTLING_EMPTYSQ_B_Q_2;
					current_pseudo_legal_move.type=CASTLING;
					current_pseudo_legal_move.info=CASTLE_LONG;
					return;
				}
			}

		}

	}

	look_for_new_square:

	// check if already finished board
	if(current_sq>=BOARD_SIZE)
	{
		end_pseudo_legal_moves=True;
		return;
	}

	// look for next non empty square, if the current square is empty
	while((current_sq<BOARD_SIZE)&&((board[current_sq] & NO_PIECE)||(COLOR_OF(board[current_sq])!=turn)))
	{
		current_sq++;
	}

	// check if finished board
	if(current_sq>=BOARD_SIZE)
	{
		end_pseudo_legal_moves=True;
		return;
	}

	examine_current_move:

	Piece from_piece=PIECE_OF(board[current_sq]);

	int start_ptr=move_table_ptr[current_sq][from_piece];

	Move current_move=move_table[start_ptr+current_ptr];

	if(current_move.type & END_PIECE)
	{

		if(from_piece!=PAWN)
		{

			current_sq++;
			current_ptr=0;

			goto look_for_new_square;

		}
		else
		{
			if(pawn_status>3)
			{
				current_sq++;
				current_ptr=0;
				pawn_status=0;

				goto look_for_new_square;
			}
			else
			{

				Rank from_file=FILE_OF(current_sq);

				Rank from_rank=RANK_OF(current_sq);

				Color from_color=COLOR_OF(board[current_sq]);

				if((from_rank==(BOARD_HEIGHT-1))||(from_rank==0))
				{
					// illegal pawn
					current_sq++;
					current_ptr=0;
					pawn_status=0;

					goto look_for_new_square;
				}

				switch(pawn_status)
				{
					// push by one
					case 0:
						{
							Square to_sq=from_color==WHITE?current_sq-BOARD_WIDTH:current_sq+BOARD_WIDTH;
							Rank prom_rank=from_color==WHITE?1:6;

							if(board[to_sq] & NO_PIECE)
							{
								current_pseudo_legal_move.from_sq=current_sq;
								current_pseudo_legal_move.to_sq=to_sq;

								if(from_rank==prom_rank)
								{

									current_pseudo_legal_move.type=PROMOTION;
									current_pseudo_legal_move.info=from_color|prom_status;

									prom_status++;

									if(prom_status>3)
									{
										pawn_status++;
										prom_status=0;
									}
								}
								else
								{
									current_pseudo_legal_move.type=ZERO_TYPE;
									current_pseudo_legal_move.info=ZERO_INFO;

									pawn_status++;
								}

								return;
							}
							else
							{
								pawn_status++;
								goto examine_current_move;
							}
							
						} break;
					// push by two
					case 1:
						{
							Bool is_on_orig_rank=from_color==WHITE?(RANK_OF(current_sq)==6):(RANK_OF(current_sq)==1);

							if(is_on_orig_rank)
							{
								Square pass_sq=from_color==WHITE?current_sq-BOARD_WIDTH:current_sq+BOARD_WIDTH;
								Square to_sq=from_color==WHITE?current_sq-2*BOARD_WIDTH:current_sq+2*BOARD_WIDTH;

								if((board[pass_sq] & NO_PIECE)&&(board[to_sq] & NO_PIECE))
								{

									current_pseudo_legal_move.from_sq=current_sq;
									current_pseudo_legal_move.to_sq=to_sq;

									current_pseudo_legal_move.type=PAWNPUSHBYTWO;
									current_pseudo_legal_move.info=pass_sq;

									pawn_status++;

									return;
								}
								else
								{
									pawn_status++;
									goto examine_current_move;
								}
							}
							else
							{
								pawn_status++;
								goto examine_current_move;
							}
						} break;
					// capture left
					case 2:
						{
							if((from_file>0)&&(true))
							{
								Square capt_sq=from_color==WHITE?current_sq-BOARD_WIDTH-1:current_sq+BOARD_WIDTH-1;

								Piece capt_piece=board[capt_sq];

								if(
									((!(capt_piece & NO_PIECE))&&(COLOR_OF(capt_piece)!=from_color))
									||
									(capt_sq==ep_square)
									)
								{
									current_pseudo_legal_move.from_sq=current_sq;
									current_pseudo_legal_move.to_sq=capt_sq;

									current_pseudo_legal_move.type=CAPTURE;
									current_pseudo_legal_move.info=ZERO_INFO;

									if(capt_sq==ep_square)
									{
										current_pseudo_legal_move.type|=ENPASSANT;
										current_pseudo_legal_move.info|=current_sq-1;
									}
									pawn_status++;

									return;
								}
								else
								{
									pawn_status++;
									goto examine_current_move;
								}
							}
							else
							{
								pawn_status++;
								goto examine_current_move;
							}
						} break;
					// capture right
					case 3:
						{
							if((from_file<(BOARD_WIDTH-1))&&(true))
							{
								Square capt_sq=from_color==WHITE?current_sq-7:current_sq+9;

								Piece capt_piece=board[capt_sq];

								if(
									((!(capt_piece & NO_PIECE))&&(COLOR_OF(capt_piece)!=from_color))
									||
									(capt_sq==ep_square)
									)
								{
									current_pseudo_legal_move.from_sq=current_sq;
									current_pseudo_legal_move.to_sq=capt_sq;

									current_pseudo_legal_move.type=CAPTURE;
									current_pseudo_legal_move.info=ZERO_INFO;

									if(capt_sq==ep_square)
									{
										current_pseudo_legal_move.type|=ENPASSANT;
										current_pseudo_legal_move.info=current_sq+1;
									}
									pawn_status++;

									return;
								}
								else
								{
									pawn_status++;
									goto examine_current_move;
								}
							}
							else
							{
								pawn_status++;
								goto examine_current_move;
							}
						} break;
				}
			}
		}
	}

	Piece to_piece=board[current_move.to_sq];

	if(to_piece & NO_PIECE)
	{
		current_pseudo_legal_move=current_move;

		current_pseudo_legal_move.type=ZERO_TYPE;
		current_pseudo_legal_move.info=ZERO_INFO;

		current_ptr++;
		return;
	}

	from_piece=board[current_sq];

	if(COLOR_OF(to_piece)!=COLOR_OF(from_piece))
	{
		current_pseudo_legal_move=current_move;

		current_pseudo_legal_move.type=CAPTURE;
		current_pseudo_legal_move.info=ZERO_INFO;

		while(!(current_move.type & END_VECTOR))
		{
			current_ptr++;
			current_move=move_table[start_ptr+current_ptr];
		}

		current_ptr++;

		return;
	}

	while(!(current_move.type & END_VECTOR))
	{
		current_ptr++;
		current_move=move_table[start_ptr+current_ptr];
	}

	current_ptr++;

	goto examine_current_move;

}

void Position::next_legal_move()
{
	Bool legal;

	do
	{
		
		next_pseudo_legal_move();

		if(end_pseudo_legal_moves)
		{
			end_legal_moves=True;
			return;
		}

		Position old=*this;

		make_move(current_pseudo_legal_move);

		Bool is_in_check_old=is_in_check(old.turn);
		Bool is_exploded_turn=is_exploded(turn);
		Bool is_exploded_old=is_exploded(old.turn);

		legal=
			// a move is legal if the side making it is not in check after making the move
			(!is_in_check_old)
			||
			// or if the opponent exploded without the side making the move exploding
			( (is_exploded_turn) && (!is_exploded_old) );

		if(legal)
		{
			is_in_check_old=is_in_check(old.turn);
		}

		if(adjacent_kings())
		{
			// adjacent kings make any pseudo legal move legal
			legal=True;
		}

		*this=old;

	}while(!legal);

	end_legal_moves=False;
}

void Move::print(Bool detailed)
{
	if(detailed)
	{
	cout
		<< "<---------------"
		<< endl
		<< "MOVE  : from sq " << (int)from_sq
		<< " to sq " << (int)to_sq
		<< " type " << (int)type
		<< " info " << (int)info
		<< endl
		<< "algeb : " << algeb()
		<< endl
		<< "--------------->"
		<< endl;
	}
	else
	{
		cout
		<< algeb() << (type&CAPTURE?"x":"") << (type&ENPASSANT?"ep":"") << (type&CASTLING?CASTLE_SAN[info]:"")
		<< endl;
	}
}

char algeb_puff[6];
char* Move::algeb()
{
	File file_from=FILE_OF(from_sq);
	Rank rank_from=RANK_OF(from_sq);
	File file_to=FILE_OF(to_sq);
	Rank rank_to=RANK_OF(to_sq);

	algeb_puff[0]=file_from+'a';
	algeb_puff[1]=(BOARD_HEIGHT-1-rank_from)+'1';
	algeb_puff[2]=file_to+'a';
	algeb_puff[3]=(BOARD_HEIGHT-1-rank_to)+'1';
	
	if(type & PROMOTION)
	{
		algeb_puff[4]=black_piece_letters[PIECE_OF(info)];
		algeb_puff[5]=0;
	}
	else
	{
		algeb_puff[4]=0;
	}
	return algeb_puff;
}

Square Position::algeb_to_square(char* algeb)
{
	if((algeb[0]<'a')||(algeb[0]>('a'+BOARD_WIDTH-1))){return SQUARE_NONE;}
	if((algeb[1]<'1')||(algeb[1]>('1'+BOARD_HEIGHT-1))){return SQUARE_NONE;}

	return(algeb[0]-'a'+((BOARD_HEIGHT-1)-(algeb[1]-'1'))*BOARD_WIDTH);
}

Bool Position::is_in_check(Color c)
{

	if(is_exploded(c)){return True;}

	Square king_pos=c==WHITE?white_king_pos:black_king_pos;

	// first look for pawn checks
	if(c==WHITE)
	{
		if(RANK_OF(king_pos)>=2)
		{
			if(FILE_OF(king_pos)>0)
			{
				// check on the left
				if(board[king_pos-BOARD_WIDTH-1]==(BLACK_PIECE|PAWN))
				{
					return True;
				}
			}
			if(FILE_OF(king_pos)<(BOARD_WIDTH-1))
			{
				// check on the right
				if(board[king_pos-BOARD_WIDTH+1]==(BLACK_PIECE|PAWN))
				{
					return True;
				}
			}
		}
	}
	else
	{
		if(RANK_OF(king_pos)<(BOARD_HEIGHT-2))
		{
			if(FILE_OF(king_pos)>0)
			{
				// check on the left
				if(board[king_pos+BOARD_WIDTH-1]==(WHITE_PIECE|PAWN))
				{
					return True;
				}
			}
			if(FILE_OF(king_pos)<(BOARD_WIDTH-1))
			{
				// check on the right
				if(board[king_pos+BOARD_WIDTH+1]==(WHITE_PIECE|PAWN))
				{
					return True;
				}
			}
		}
	}

	for(Piece p=KNIGHT;p<=QUEEN;p++)
	{
		
		int start_ptr=move_table_ptr[king_pos][p];

		Piece test_piece=c==WHITE?BLACK_PIECE|p:WHITE_PIECE|p;

		examine_check:

		Move test_move=move_table[start_ptr];

		if(test_move.type & END_PIECE){goto piece_finished;}

		Piece query_piece=board[test_move.to_sq];

		if(test_piece==query_piece)
		{
			return True;
		}

		if(query_piece & NO_PIECE)
		{

			start_ptr++;

			goto examine_check;

		}
		else
		{

			if(move_table[start_ptr].type & END_VECTOR){start_ptr++;goto examine_check;}

			look_for_next_vector:

			start_ptr++;

			if(move_table[start_ptr].type & END_PIECE){goto piece_finished;}

			if(move_table[start_ptr].type & END_VECTOR){start_ptr++;goto examine_check;}

			goto look_for_next_vector;
			
		}

		piece_finished:

		int piece_done=1;

	}

	return False;
}

Bool Position::is_in_check_square(Square sq,Color c)
{
	Position dummy=*this;

	if(c==WHITE)
	{
		dummy.white_king_pos=sq;
		dummy.board[sq]=WHITE_PIECE|KING;
	}
	else
	{
		dummy.black_king_pos=sq;
		dummy.board[sq]=BLACK_PIECE|KING;
	}

	return dummy.is_in_check(c);
}

void Position::make_move(Move m)
{

	ep_square=SQUARE_NONE;

	Piece from_piece=board[m.from_sq];
	board[m.from_sq]=NO_PIECE;

	if(m.type & CAPTURE)
	{
		board[m.to_sq]=NO_PIECE;
		int start_ptr=move_table_ptr[m.to_sq][KING];

		while(! (move_table[start_ptr].type & END_PIECE) )
		{
			Square to_sq=move_table[start_ptr].to_sq;

			if(PIECE_OF(board[to_sq])!=PAWN)
			{
				board[to_sq]=NO_PIECE;
			}

			start_ptr++;
		}

		if(m.type&ENPASSANT)
		{
			board[m.info]=NO_PIECE;
		}

	}
	else
	{
		board[m.to_sq]=from_piece;

		if(m.type & PROMOTION)
		{
			board[m.to_sq]=m.info;
		}

		if(m.type & CASTLING)
		{
			if(m.info==CASTLE_SHORT)
			{
				if(turn==WHITE)
				{
					board[CASTLING_RIGHT_K_DISABLE_SQUARE]=NO_PIECE;
					board[CASTLING_EMPTYSQ_W_K_1]=WHITE_PIECE|ROOK;
				}
				else
				{
					board[CASTLING_RIGHT_k_DISABLE_SQUARE]=NO_PIECE;
					board[CASTLING_EMPTYSQ_B_K_1]=BLACK_PIECE|ROOK;
				}
			}
			else
			{
				if(turn==WHITE)
				{
					board[CASTLING_RIGHT_Q_DISABLE_SQUARE]=NO_PIECE;
					board[CASTLING_EMPTYSQ_W_Q_1]=WHITE_PIECE|ROOK;
				}
				else
				{
					board[CASTLING_RIGHT_q_DISABLE_SQUARE]=NO_PIECE;
					board[CASTLING_EMPTYSQ_B_Q_1]=BLACK_PIECE|ROOK;
				}
			}
		}

		if(from_piece==(WHITE_PIECE|KING))
		{
			white_king_pos=m.to_sq;
		}
		else if(from_piece==(BLACK_PIECE|KING))
		{
			black_king_pos=m.to_sq;
		}

		if(m.type&PAWNPUSHBYTWO)
		{
			ep_square=m.info;
		}
	}

	if(board[CASTLING_RIGHT_K_DISABLE_SQUARE] & NO_PIECE){castling_rights&=~CASTLING_RIGHT_K;}
	if(board[CASTLING_RIGHT_Q_DISABLE_SQUARE] & NO_PIECE){castling_rights&=~CASTLING_RIGHT_Q;}
	if(board[CASTLING_RIGHT_k_DISABLE_SQUARE] & NO_PIECE){castling_rights&=~CASTLING_RIGHT_k;}
	if(board[CASTLING_RIGHT_q_DISABLE_SQUARE] & NO_PIECE){castling_rights&=~CASTLING_RIGHT_q;}
	if(white_king_pos!=WHITE_KING_CASTLE_FROM_SQUARE){castling_rights&=~(CASTLING_RIGHT_K|CASTLING_RIGHT_Q);}
	if(black_king_pos!=BLACK_KING_CASTLE_FROM_SQUARE){castling_rights&=~(CASTLING_RIGHT_k|CASTLING_RIGHT_q);}

	turn=turn==WHITE?BLACK:WHITE;

	init_move_iterator();
}

int nodes;
int tbhits;
int phits;

time_t ltime0;
time_t ltime;

string line;

int Position::search_recursive(Depth depth,int alpha,int beta,Bool maximizing)
{

	nodes++;

	if((nodes % 10000000)==0)
	{
		if(verbose)
		{
			cout << "---> info: " << line.c_str() << endl;
		}
	}

	init_move_iterator();
	next_legal_move();

	if(end_legal_moves)
	{

		if(is_in_check(turn))
		{
			if(turn==WHITE)
			{
				return (-MATE_SCORE+depth);
			}
			else
			{
				return (MATE_SCORE-depth);
			}
		}
		else
		{
			return DRAW_SCORE;
		}
	}

	init_move_iterator();

	if(depth>search_depth)
	{

		// heuristic eval
		
		return calc_heuristic_eval();
	
	}

	int value=maximizing?-MATE_SCORE-1:MATE_SCORE+1;

	HashTableEntry* entry=look_up();

	HashTableEntry entry_copy;

	if(entry!=NULL)
	{
		entry_copy=*entry;
		
		entry_copy.best_moves.init_iteration();

		phits++;
	}

	Bool move_available;

	do
	{

		move_available=False;

		Move try_move;
		
		if(entry!=NULL)
		{
			Move* next_move=entry_copy.best_moves.next_move(maximizing);

			if(next_move!=NULL)
			{
				tbhits++;
				move_available=True;
				try_move=*next_move;
			}

		}

		Bool move_already_examined;

		if(!move_available) do
		{

			next_legal_move();

			move_already_examined=False;

			if(!end_legal_moves)
			{

				move_available=True;

				try_move=current_pseudo_legal_move;

				if(entry!=NULL)
				{
					for(int i=0;i<entry_copy.best_moves.no_moves;i++)
					{
						if(try_move.equal_to(entry_copy.best_moves.move_list[i]))
						{
							move_already_examined=True;
							move_available=False;
						}
					}
				}

			}
			else
			{
				move_available=False;
			}

		}while(((!move_available)&&(!end_legal_moves))||(move_already_examined));

		if(move_available)
		{

			if(depth==0)
			{
				if(verbose)
				{
					cout << try_move.algeb();
				}
			}

			string old_line=line;

			line+=try_move.algeb();line+=" ";

			//////////////////////////////////////////////////////////////////////

			Position old=*this;

			make_move(try_move);
						
				//////////////////////////////////////////////////////////////////////

				int eval=search_recursive(depth+1,alpha,beta,!maximizing);

				//////////////////////////////////////////////////////////////////////

			*this=old;

			//////////////////////////////////////////////////////////////////////

			Move* move_hit=look_up_move(try_move);

			int old_eval=-MATE_SCORE-1;
			if(move_hit!=NULL)
			{
				// update existing move always
				old_eval=move_hit->eval;
				move_hit->eval=eval;
			}

			//////////////////////////////////////////////////////////////////////

			line=old_line;

			time( &ltime );int elapsed=(int)ltime-(int)ltime0;
			float nodes_per_sec=elapsed>0?(float)nodes/(float)elapsed:0;

			if(depth==0)
			{
				if(verbose)
				{
					cout << " ( ";
					if(old_eval>(-MATE_SCORE-1))
					{
						cout << old_eval << " -> ";
					}
					cout << eval << " ) n " << nodes << " tm " << elapsed << " nps " << (int)nodes_per_sec << " phits " << phits << " tbhits " << tbhits << endl;
				}
			}

			if(maximizing)
			{
				if(eval>value)
				{
					value=eval;

					store_move(try_move,depth,eval,maximizing);
				}

				if(depth>=0)
				{
					if(value>alpha){alpha=value;}
				}
			}
			else
			{
				if(eval<value)
				{
					value=eval;

					store_move(try_move,depth,eval,maximizing);
				}

				if(depth>=0)
				{
					if(value<beta){beta=value;}
				}
			}
			
			if(beta<alpha)
			{
				return value;
			}

			if(quit_search){return value;};
		}
		
	}while(move_available);

	return value;
}

void Position::search()
{

	nodes=0;
	tbhits=0;
	phits=0;

	init_move_iterator();

	time( &ltime0 );

	line="";

	principal_value=search_recursive(0,-MATE_SCORE-1,MATE_SCORE+1,turn==WHITE);

	report_search_results(search_depth);

}

char score_puff[30];
char value_puff[20];
char* Position::score_of(int value)
{
	if(abs(value)<(MATE_SCORE-1000))
	{
		strcpy_s(score_puff,30,"cp ");
		_itoa_s(value,value_puff,20,10);
		strcat_s(score_puff,30,value_puff);
		return score_puff;
	}

	_itoa_s(value>0?MATE_SCORE-value:-MATE_SCORE-value,value_puff,20,10);
	strcpy_s(score_puff,30,"mate ");
	strcat_s(score_puff,30,value_puff);
	return score_puff;
}

void Position::report_search_results(MoveCount report_depth)
{

	if(quit_search){return;}

	calc_principal_variation(report_depth);

	cout << "depth " << (int)(report_depth+1) << " score " << score_of(principal_value);

	if(principal_variation[0]!=0)
	{
		cout << " pv " << principal_variation << endl;
	}
	else
	{
		cout << endl;
	}
}

void Position::i_search()
{

	verbose=False;

	cout << endl;

	for(search_depth=0;search_depth<20;search_depth++)
	{
		
		search();

		if(quit_search){return;}

	}
}

void Position::reset()
{
	set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Position::store_move(Move move,Depth depth,int eval,Bool maximizing)
{

	if(quit_search){return;}

	calc_hash_key();

	if(!hash_table[hash_key].used)
	{
		hash_table[hash_key].init();
		hash_table[hash_key].depth=depth;
	}

	if(depth<=hash_table[hash_key].depth)
	{

		if(memcmp((void*)&hash_table[hash_key].position,(void*)this,sizeof(PositionTrunk))!=0)
		{
			// overwrite other position
			hash_table[hash_key].init();
		}

		memcpy((void*)&hash_table[hash_key].position,(void*)this,sizeof(PositionTrunk));

		hash_table[hash_key].depth=depth;

		move.eval=eval;

		hash_table[hash_key].best_moves.add(move,maximizing);
	}
}

HashTableEntry* Position::look_up()
{
	calc_hash_key();

	int match=memcmp((void*)&hash_table[hash_key].position,(void*)this,sizeof(PositionTrunk));

	if(match==0)
	{
		return &hash_table[hash_key];
	}
	else
	{
		return NULL;
	}
}

Bool Move::equal_to(Move m)
{
	if(
		(from_sq==m.from_sq)
		&&
		(to_sq==m.to_sq)
		&&
		(type==m.type)
		&&
		(info==m.info)
	)
	{
		return True;
	}

	return False;
}

void MoveList::init()
{

	no_moves=0;
	current_move=0;
	
}

void MoveList::init_iteration()
{

	for(int i=0;i<no_moves;i++)
	{
		move_list[i].done=False;
	}
	current_move=0;
}

Move* MoveList::get_last_updated()
{
	return &move_list[last_updated];
}

Move* MoveList::next_move(Bool maximizing)
{
	if(current_move>=no_moves)
	{
		return NULL;
	}
	else
	{
		current_move++;

		int init_score=maximizing?-MATE_SCORE:MATE_SCORE;

		int sel_index=0;

		for(int i=0;i<no_moves;i++)
		{
			if(!move_list[i].done)
			{
				int eval=move_list[i].eval;

				if(maximizing)
				{
					if(eval>init_score)
					{
						init_score=eval;
						sel_index=i;
					}
				}
				else
				{
					if(eval<init_score)
					{
						init_score=eval;
						sel_index=i;
					}
				}
			}
		}

		move_list[sel_index].done=1;

		return(&move_list[sel_index]);
	}
}

void HashTableEntry::init()
{

	best_moves.init();
	used=True;

}

void MoveList::add(Move move,Bool maximizing)
{

	int sel_index=0;

	for(int i=0;i<no_moves;i++)
	{
		if(move_list[i].equal_to(move))
		{

			sel_index=i;

			goto set_move;

		}
	}

	if(no_moves<MOVE_LIST_LENGTH)
	{
		sel_index=no_moves++;
	}
	else
	{

		// throw out worst rather than best
		maximizing=!maximizing;

		int eval=maximizing?-MATE_SCORE-1:MATE_SCORE+1;

		for(int i=0;i<no_moves;i++)
		{
			if(maximizing)
			{
				if(move_list[i].eval>eval)
				{
					eval=move_list[i].eval;
					sel_index=i;
				}
			}
			else
			{
				if(move_list[i].eval<eval)
				{
					eval=move_list[i].eval;
					sel_index=i;
				}
			}
		}

	}

	set_move:

	move_list[sel_index]=move;

	last_updated=sel_index;

}

int Position::calc_material_balance()
{

	material_balance=0;

	for(Square sq=0;sq<BOARD_SIZE;sq++)
	{
		material_balance+=material_values[board[sq]];
	}

	return material_balance;
}

int Position::number_of_pseudo_legal_moves(Color c)
{
	Position dummy=*this;

	dummy.turn=c;

	dummy.init_move_iterator();

	int num_pseudo_legal=0;

	do
	{

		dummy.next_pseudo_legal_move();
		if(!dummy.end_pseudo_legal_moves)
		{
			num_pseudo_legal++;
		}

	}while(!dummy.end_pseudo_legal_moves);

	return num_pseudo_legal;
}

int Position::calc_heuristic_eval()
{
	
	heuristic_eval=calc_material_balance();

	heuristic_eval+=(attackers_on_king(BLACK)-attackers_on_king(WHITE))*ATTACKER_BONUS;

	heuristic_eval+=(number_of_pseudo_legal_moves(WHITE)-number_of_pseudo_legal_moves(BLACK))*MOBILITY_BONUS;

	return heuristic_eval;
}

char* Position::calc_principal_variation(MoveCount max_ply)
{
	char *ptr=principal_variation;

	*ptr=0;

	HashTableEntry* entry;
	MoveCount ply=0;

	Position clone=*this;

	Bool root=True;

	do
	{
		entry=clone.look_up();

		if(entry!=NULL)
		{
			//entry->best_moves.init_iteration();
			//Move *m=entry->best_moves.next_move(clone.turn==WHITE);

			Move *m=entry->best_moves.get_last_updated();

			if(root)
			{
				root=False;
			}

			strcpy_s(ptr,6,m->algeb());
			ptr+=strlen(m->algeb());
			*ptr=' ';
			ptr++;
			*ptr=0;

			clone.make_move(*m);

			ply++;
		}

	}while(((entry!=NULL)&&(ply<20))&&(!(ply>max_ply)));

	return principal_variation;
}

Bool Position::adjacent_kings()
{

	if(is_exploded(WHITE)||is_exploded(BLACK)){return False;}

	int ptr=move_table_ptr[white_king_pos][KING];

	// if the white king can capture the black king the kings are adjacent
	while(!(move_table[ptr].type & END_PIECE))
	{
		if(move_table[ptr].to_sq==black_king_pos){return True;}
		ptr++;
	}

	return False;

}

Move* Position::look_up_move(Move m)
{
	HashTableEntry* entry=look_up();

	if(entry==NULL)
	{
		return NULL;
	}

	MoveList* best_moves=&entry->best_moves;

	for(int i=0;i<best_moves->no_moves;i++)
	{
		if(best_moves->move_list[i].equal_to(m))
		{
			return(&best_moves->move_list[i]);
		}
	}

	return NULL;
}

int Position::attackers_on_king(Color c)
{
	int ptr=move_table_ptr[c==WHITE?white_king_pos:black_king_pos][KING];

	int attackers=0;

	while(!(move_table[ptr].type & END_PIECE))
	{
		Square attacked_square=move_table[ptr].to_sq;

		for(Piece test_piece=KNIGHT;test_piece<=QUEEN;test_piece++)
		{

			int ptr2=move_table_ptr[attacked_square][test_piece];

			Square test_sq;
			Piece curr_piece;

			while(move_table[ptr2].type!=END_PIECE)
			{

				test_sq=move_table[ptr2].to_sq;

				curr_piece=board[test_sq];

				if(curr_piece & NO_PIECE)
				{
					ptr2++;
				}
				else
				{
					if((PIECE_OF(curr_piece)==test_piece)&&(COLOR_OF(curr_piece)!=c))
					{
						attackers++;
					}

					while(move_table[ptr2].type!=END_VECTOR){ptr2++;}
					ptr2++;
				}
			}
		}

		ptr++;
	}

	return attackers;
}

char sq_to_algeb_puff[5];
char* Position::square_to_algeb(Square sq)
{
	File f=FILE_OF(sq);
	Rank r=RANK_OF(sq);

	sq_to_algeb_puff[0]=f+'a';
	sq_to_algeb_puff[1]=(BOARD_HEIGHT-1-r)+'1';
	sq_to_algeb_puff[2]=0;

	return sq_to_algeb_puff;
}