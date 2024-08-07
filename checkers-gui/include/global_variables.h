#ifndef GLOBAL_H
#define GLOBAL_H

#include <map>

namespace checkers
{
	// board size
	static const int s_size = 10;
	// map containing letters and their corresponding integers
	static std::map<char, int> s_coords{ {'a', 1}, { 'b', 2 }, { 'c', 3 }, { 'd', 4 }, { 'e', 5 }, { 'f', 6 }, { 'g', 7 }, { 'h', 8 }, { 'i', 9 }, { 'j', 10 } };
	// maximal amount of pieces in the game
	static const int s_max_piece_count = s_size * s_size / 2 - 1;
	// initialized pieces
	static int s_pieces_initialized = 0;
	// minimal score that the game can achieve
	static const int s_min_score = -1 * (s_max_piece_count - 1);
	// maximal score that the game can achieve
	static const int s_max_score = s_max_piece_count - 1;
	// debug counter for game copying constructor
	static int s_game_copied = 0;
}

#endif
