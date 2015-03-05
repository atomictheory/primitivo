#include <iostream>
#include <fstream>

#include "movegen.h"

using namespace std;

char buf[100];

Position game[200];
int game_ptr=0;

Position p;

void *search_thread(void *arg0)
{
	p.search();
	return arg0;
}

void *i_search_thread(void *arg0)
{
	p.i_search();
	return arg0;
}

//int main(int argc, char** argv)
int main()
{

	init_move_table();

	Bool do_exit=False;

	p.reset();

	p.print();

	do
	{
		std::cin.getline(buf,100);

		if(buf[0]=='x'){do_exit=True;}
		else
		{
#if defined _WIN64 || defined _WIN32
			if(buf[0]=='f')
			{
				OpenClipboard(NULL);
				char fen[200];
				strncpy_s(fen, (char*)GetClipboardData(CF_TEXT),200);
				CloseClipboard();

				p.set_from_fen(fen);
				p.print();
				game_ptr=0;
			}
#endif
			if(buf[0]=='r')
			{
				game_ptr=0;
				p.reset();
				p.print();
			}
			if(buf[0]=='p')
			{
				p.print();
			}
			if(strcmp(buf,"help")==0)
			{
				cout << endl;
				cout << "form of commmands: [one letter command][command argument]" << endl;
				cout << endl;
				cout << "commands:" << endl;
				cout << endl;
				cout << "r: reset board" << endl;
				cout << "f: set board from fen on clipboard" << endl;
				cout << "p: print board" << endl;
				cout << "l: list legal moves" << endl;
				cout << "m[algeb]: make move given in algebraic notation ( example: 'me2e4' )" << endl;
				cout << "u: unmake last move" << endl;
				cout << "g[depth]: search to depth, possible values of depth = 1 ... 9 ( example: 'g6' )" << endl;
				cout << "i: infinite search" << endl;
				cout << "q: quit search" << endl;
				cout << "s: save hash table" << endl;
				cout << "h: load hash table" << endl;
				cout << "e: erase hash table" << endl;
				cout << endl;
				cout << "x: exit" << endl;
				cout << endl;
				cout << "for this help type help+ENTER" << endl;
				cout << endl;
				buf[0]=0;
				
			}
			if((buf[0]=='l')||(buf[0]=='m'))
			{
				Bool end_legal;
				p.init_move_iterator();
				char algeb[6];
				if(buf[0]=='m')
				{
#if defined _WIN64 || defined _WIN32
					strcpy_s(algeb,sizeof(&buf[1]),&buf[1]);
					buf[5]='\0';
#else
					strcpy(algeb,&buf[1]);
#endif
				}
				do
				{
					p.next_legal_move();
					end_legal=p.end_legal_moves;
					if(!end_legal)
					{
						if(buf[0]=='l')
						{
							p.current_pseudo_legal_move.print(SIMPLE);
						}
						else
						{
							if(strcmp(algeb,p.current_pseudo_legal_move.algeb())==0)
							{
								game[game_ptr++]=p;
								p.make_move(p.current_pseudo_legal_move);
								p.print();
								end_legal=True;
							}
						}
					}
				}while(!end_legal);
			}

			if(buf[0]=='g')
			{
				

				Depth depth=4;

				if((buf[1]>='1')&&(buf[1]<='9'))
				{
					depth=(Depth)(buf[1]-'1');
				}

				cout << "search to depth " << (int)(depth+1) << endl;

				p.search_depth=depth;

				verbose=True;

				quit_search=False;

#if defined _WIN64 || defined _WIN32
				_beginthread(search_thread,0,NULL);
#else
				pthread_t pool[1];
				pthread_create(&pool[0],NULL,search_thread,NULL);
#endif
			}

			if(buf[0]=='u')
			{
				if(game_ptr-->0)
				{
					p=game[game_ptr];
					p.print();
				}
			}

			if(buf[0]=='i')
			{
				quit_search=False;
#if defined _WIN64 || defined _WIN32
				_beginthread(i_search_thread,0,NULL);
#else
				pthread_t pool[1];
				pthread_create(&pool[0],NULL,i_search_thread,NULL);
#endif
			}

			if(buf[0]=='q')
			{
				quit_search=True;
			}

			if(buf[0]=='e')
			{
				erase_hash_table();
				p.print();
			}

			if(buf[0]=='s')
			{
				ofstream o("hash.txt",ios::binary);
				o.write((char*)&hash_table,sizeof(hash_table));
				o.write((char*)&total_used,sizeof(int));
				o.close();
				cout << "hash table saved" << endl;
			}

			if(buf[0]=='h')
			{
				ifstream i("hash.txt",ios::binary);
				i.read((char*)&hash_table,sizeof(hash_table));
				i.read((char*)&total_used,sizeof(int));
				i.close();
				cout << "hash table loaded" << endl;
			}
		}
	}while(!do_exit);

	return 0;
}
