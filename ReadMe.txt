non bitboard atomic chess engine

1) to set up the position copy the fen to the clipboard and type f+ENTER
2) to start analyzing type i+ENTER ( i = infinite search )
3) to quit analyzing type q+ENTER

for more detailed instructions type help+ENTER

Motivation

Available atomic chess engines such as Pulsar and Atomkraft suffer from bugs. Pulsar's move generation is flawed when it comes to en passant captures. Atomkraft, which is a modification of Stockfish, crashes is certain positions. ( Also Pulsar allows castling in check. )

I decided to write an atomic chess engine from scratch instead of rewriting some existing engine.

I call it primitivo because it is written in a very simple undestandable way without using advanced techniques such as bitboard representation or Zobrist hashing ( still it is able to analyze at around 150 kNodes/sec on a laptop ).

Method

Primitivo uses an alphabeta search. This is a lossless method, meaning that it finds the same best move as would have been found by a full minimax search ( runs no risk of pruning the optimal move ).

Heuristic evaluation is very simple:

- material balance
- mobility ( simply the number of pseudo legal moves by each side )
- attackers on king ( number of attackers on the squares around the king )

Based on these primitivo can reach a depth of 8 within a few minutes in complicated positions and is capable of finding reasonable moves, though admittedly not as good moves as more mature bitboard engines.

Any suggestions are welcome.