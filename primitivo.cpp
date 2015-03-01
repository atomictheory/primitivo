#include <iostream>
#include <fstream>

#include "movegen.h"

#include <Windows.h>
#include <process.h>

using namespace std;

char buf[100];

Position game[200];
int game_ptr=0;

Position p;

void search_thread(void* param)
{
	p.search();
}

void i_search_thread(void* param)
{
	p.i_search();
}

void main(int argc, char** argv)
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
			if((buf[0]=='l')||(buf[0]=='m'))
			{
				Bool end_legal;
				p.init_move_iterator();
				char algeb[6];
				if(buf[0]=='m')
				{
					strncpy_s(algeb,buf+1,5);
					buf[5]=0;
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

				_beginthread(search_thread,0,NULL);

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
				_beginthread(i_search_thread,0,NULL);
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
				ofstream o("c:\\Unzip\\hash.txt",ios::binary);
				o.write((char*)&hash_table,sizeof(hash_table));
				o.close();
				cout << "hash table saved" << endl;
			}

			if(buf[0]=='h')
			{
				ifstream i("c:\\Unzip\\hash.txt",ios::binary);
				i.read((char*)&hash_table,sizeof(hash_table));
				i.close();
				cout << "hash table loaded" << endl;
			}
		}
	}while(!do_exit);

}