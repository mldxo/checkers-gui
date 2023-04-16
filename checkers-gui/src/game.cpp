#include "include/game.h"

// todo: (*done)
// * enable only current turn moves
// * enable only possible moves (captures)
// * enable hit series
// add check if king to evaluation
//		if the counter won't work: get value from argument and copy to local variable, then pass it to resursive function call
// remake code into more functions, current player and opponent pointers
// * fix the namescheme
// add menu
// add console input
// add state saving 
// add game load
// add statistics

namespace checkers
{
	game::game(int fps) : m_board(new std::vector<std::vector<piece*>>(size, std::vector<piece*>(size, 0))), m_first_turn(true), m_console_mode(false), m_player_1(NULL), m_player_2(NULL), m_current_player(NULL), m_p_list_1(NULL), m_p_list_2(NULL), m_to_delete_list(NULL), m_is_finished(false), m_first_won(false), m_second_won(false), m_selected_piece(NULL), m_available_capture(false), m_fps(fps), m_clock(), m_settings(), m_window(sf::VideoMode((unsigned int)(square_size * size), (unsigned int)(square_size * size)), "Checkers", sf::Style::Default, m_settings), m_event()
	{
		// SFML setup
		m_settings.antialiasingLevel = 16;
		m_window.setFramerateLimit(fps);
		m_window.setVerticalSyncEnabled(true);
	}

	std::ostream& operator<<(std::ostream& os, const std::vector<std::vector<piece*>>* board)
	{
		os << "\t  ";
		for (char a = 'a'; a < 'a' + checkers::size; ++a) // colums as letters
			os << a << "   ";
		/*for (int i = 0; i < checkers::size; ++i)
			os << i << "   ";*/
		os << std::endl << std::endl << std::endl;
		for (int i = 0; i < checkers::size; ++i)
		{
			//os << game::size - i << "\t| ";
			os << i << "\t| ";
			for (int j = 0; j < size; ++j)
				os << (*board)[j][i] << " | ";
			os << std::endl << std::endl;
		}
		return os;
	}

	game::~game()
	{
		delete m_board;
		delete m_player_1;
		delete m_player_2;
	}

	void game::switch_turn(void) { m_first_turn = !m_first_turn; }

	void game::menu(void)
	{
		// todo: menu
		// this code is the following option from the future menu: "New Game -> Player vs Player"

		// simplified: players init
		std::string name_1 = "Some player 1";
		std::string name_2 = "Some player 2";
		m_player_1 = new player('W', name_1);
		m_player_2 = new player('B', name_2);

		// set play order and evaluation direction
		m_player_1->set_first(true);
		m_player_2->set_first(false);
		m_player_1->set_next_player(m_player_2);
		m_player_2->set_next_player(m_player_1);

		// todo: change to algorithm
		// rows of the second player (upper)
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if ((i + j) % 2 != 0)
				{
					(*m_board)[j][i] = new piece(m_player_2->get_sign(), j, i);
					m_p_list_2.push_back((*m_board)[j][i]);
					m_player_2->add_piece();
				}
			}
		}
		// rows of the first player (lower)
		for (int i = size - 1; i >= size - 3; --i)
		{
			for (int j = 0; j < size; ++j)
			{
				if ((i + j) % 2 != 0)
				{
					(*m_board)[j][i] = new piece(m_player_1->get_sign(), j, i);
					m_p_list_1.push_back((*m_board)[j][i]);
					m_player_1->add_piece();
				}
			}
		}

		// evaluate available moves for the first player
		int dummy = 0;
		m_available_capture = evaluate(m_p_list_1, m_board, &dummy, m_player_1);

		#ifdef _DEBUG
		std::cout << "List of pieces of first player" << std::endl;
		print_pieces(&m_p_list_1);

		std::cout << "List of pieces of second player" << std::endl;
		print_pieces(&m_p_list_2);
		#endif
	}

	void game::loop(void)
	{
		bool selected = false;
		m_clock.restart();

		// main loop
		while (m_window.isOpen())
		{
			while (m_window.pollEvent(m_event))
			{
				if (m_event.type == sf::Event::Closed)
					m_window.close();

				if (m_event.type == sf::Event::MouseButtonReleased && m_event.mouseButton.button == sf::Mouse::Left)
				{
					//if (!available_capture) // check for new kings made
					//{

					//}

					if (m_selected_piece != NULL) // choice after highlighting
					{
						// getting coords of the click after highlighting selected piece
						int x = sf::Mouse::getPosition(m_window).x / (m_window.getSize().x / size);
						int y = sf::Mouse::getPosition(m_window).y / (m_window.getSize().y / size);
						
						// find corresponding piece
						bool is_found = false;
						available_move* found_move = NULL;

						all_of(m_selected_piece->get_av_list()->begin(), m_selected_piece->get_av_list()->end(), [&x, &y, &is_found, &found_move](available_move* a)
						{
							// check if selected coords match any of possible moves
							if (a->get_x() == x && a->get_y() == y)
							{
								#ifdef _DEBUG
								std::cout << a->get_x() << " " << a->get_y() << std::endl;
								#endif		
								found_move = a;
								is_found = true;
								return false;
							}
							return true;
						});
						if (!is_found) // deselection when wrong coords given
						{
							selected = false;
							m_selected_piece = NULL;
						}
						else // making a move
						{
							// if available move is a capture
							//if (found_move->is_capture())
							//{
							//	// coords to delete
							//	int x_d = 0;
							//	int y_d = 0;
							//}
							
							if (found_move->is_capture())
							{
								// mark found capture
								available_capture* found_capture = dynamic_cast<available_capture*>(found_move);
								int x_d = found_capture->get_x_d();
								int y_d = found_capture->get_y_d();
								
								#ifdef _DEBUG
								std::cout << "CONTROL" << std::endl;
								std::cout << "Coords to delete" << x_d << " " << y_d << std::endl;
								#endif

								// now it is not needed
								(*m_board)[x_d][y_d]->set_captured();

								// temporary: delete to _DEBUG
								piece* piece_to_delete = (*m_board)[x_d][y_d];
								(*m_board)[x_d][y_d] = NULL;
								
								if (m_first_turn)
								{
									m_player_2->capture();
									delete_from_list(&m_p_list_2, piece_to_delete);
									
									// push to the "to delete list" (delete later) --- or maybe just to display it during multicapture
									m_to_delete_list.push_back(new piece('b', x_d, y_d));

									m_player_1->set_combo(true);
									std::cout << "player's 1 combo" << std::endl;
								}
								else
								{
									m_player_1->capture();
									delete_from_list(&m_p_list_1, piece_to_delete);
									
									// push to the "to delete list" (delete later)
									m_to_delete_list.push_back(new piece('w', x_d, y_d));

									m_player_2->set_combo(true);
									std::cout << "player's 2 combo" << std::endl;
								}
							}
							//else // quitting the combo
							//{
							//	player_1->set_combo(false);
							//	player_2->set_combo(false);

							//	clear_to_delete_list(&to_delete_list, &p_list_1);
							//	clear_to_delete_list(&to_delete_list, &p_list_2);

							//	std::cout << "combo deleted" << std::endl;
							//}

							// move the piece (piece, which is moving -> both capture and normal move)
							(*m_board)[m_selected_piece->get_x()][m_selected_piece->get_y()] = NULL;
							m_selected_piece->set_x(x);
							m_selected_piece->set_y(y);
							(*m_board)[x][y] = m_selected_piece;
							m_selected_piece = NULL;
							selected = false;
							
							#ifdef _DEBUG
							std::cout << "List of pieces of first player" << std::endl;
							print_pieces(&m_p_list_1);
							std::cout << "List of pieces of second player" << std::endl;
							print_pieces(&m_p_list_2);
							#endif		

							// add end of game *

							//// switch turn, if no combo
							if (!m_player_1->get_combo() && !m_player_2->get_combo())
								switch_turn();
							else // section to test (fixes stuff)
							{
								clear_list(&m_p_list_1);
								clear_list(&m_p_list_2);
							}
							// add end of game *

							// evaluate current player
							int dummy = 0;
							if (m_first_turn)
							{
								// evaluate next move
								m_available_capture = evaluate(m_p_list_1, m_board, &dummy, m_player_1);

								// exit the combo, if no more captures
								if (m_player_1->get_combo() && !m_available_capture)
								{
									// delete opponent's pieces of multi capture, clear failed list of possible moves, cancel combo, evaluate again
									clear_to_delete_list(&m_to_delete_list, &m_p_list_1);
									clear_to_delete_list(&m_to_delete_list, &m_p_list_2);

									clear_list(&m_p_list_1);
									clear_list(&m_p_list_2);
									m_player_1->set_combo(false);
									m_player_2->set_combo(false);
									switch_turn();
									m_available_capture = evaluate(m_p_list_2, m_board, &dummy, m_player_2);
								}
								else
									clear_list(&m_p_list_2);
								// check for empty evaluation?
							}
							else // second turn
							{
								// evaluate next move
								m_available_capture = evaluate(m_p_list_2, m_board, &dummy, m_player_2);

								// exit the combo, if no more captures
								if (m_player_2->get_combo() && !m_available_capture)
								{
									// delete opponent's pieces of multi capture, clear failed list of possible moves, cancel combo, evaluate again
									clear_to_delete_list(&m_to_delete_list, &m_p_list_1);
									clear_to_delete_list(&m_to_delete_list, &m_p_list_2);

									clear_list(&m_p_list_1);
									clear_list(&m_p_list_2);
									m_player_1->set_combo(false);
									m_player_2->set_combo(false);
									switch_turn();
									m_available_capture = evaluate(m_p_list_1, m_board, &dummy, m_player_1);
								}
								else
									clear_list(&m_p_list_1);
							}
						}
					}
					else
						selected = !selected;
				}
				else if (m_event.type == sf::Event::MouseButtonReleased && m_event.mouseButton.button == sf::Mouse::Right)
				{
					selected = false;
					m_selected_piece = NULL;
				}
			}

			m_window.clear();
			draw(m_window);
			

			// first choice, nothing is already highlighted
			if (selected)
			{
				m_selected_piece = NULL;

				int x = sf::Mouse::getPosition(m_window).x / (m_window.getSize().x / size);
				int y = sf::Mouse::getPosition(m_window).y / (m_window.getSize().y / size);

				// if the correspoding field contains a piece
				if ((*m_board)[x][y] != NULL)
				{
					#ifdef _DEBUG
					std::cout << "x: " << x << "; y: " << y << "; piece: " << (*m_board)[x][y] << std::endl;
					#endif

					// check if player owns this piece
					if (m_first_turn)
					{
						if ((*m_board)[x][y]->get_sign() == m_player_1->get_sign())
						{
							#ifdef _DEBUG
							std::cout << ":)" << std::endl;
							#endif
							
							bool found_capture = false;
							if (!(*m_board)[x][y]->get_av_list()->empty())
							{
								// find at least one move that is a capture
								all_of((*m_board)[x][y]->get_av_list()->begin(), (*m_board)[x][y]->get_av_list()->end(), [&found_capture](available_move* a)
									{
										if (a->is_capture())
										{
											found_capture = true;
											return false;
										}
										return true;
									});
								#ifdef _DEBUG
								for_each((*m_board)[x][y]->get_av_list()->begin(), (*m_board)[x][y]->get_av_list()->end(), [](available_move* a) { std::cout << "available: x: " << a->get_x() << "; y: " << a->get_y() << std::endl; });
								#endif
							}
							if ((found_capture && m_available_capture) || (!found_capture && !m_available_capture)) // this lets making only capture moves, comment out to enable testing - replace to xnor
								m_selected_piece = (*m_board)[x][y];
						}
						else
						{
							#ifdef _DEBUG
							std::cout << ":(" << std::endl;
							#endif
						}
					}
					else
					{
						if ((*m_board)[x][y]->get_sign() == m_player_2->get_sign())
						{
							#ifdef _DEBUG
							std::cout << ":)" << std::endl;
							#endif

							bool found_capture = false;
							if (!(*m_board)[x][y]->get_av_list()->empty())
							{
								// find at least one move that is a capture
								all_of((*m_board)[x][y]->get_av_list()->begin(), (*m_board)[x][y]->get_av_list()->end(), [&found_capture](available_move* a)
									{
										if (a->is_capture())
										{
											found_capture = true;
											return false;
										}
										return true;
									});
								#ifdef _DEBUG
								for_each((*m_board)[x][y]->get_av_list()->begin(), (*m_board)[x][y]->get_av_list()->end(), [](available_move* a) { std::cout << "available: x: " << a->get_x() << "; y: " << a->get_y() << std::endl; });
								#endif
							}
							if ((found_capture && m_available_capture) || (!found_capture && !m_available_capture)) // this lets making only capture moves, comment out to enable testing
								m_selected_piece = (*m_board)[x][y];
						}
						else
						{
							#ifdef _DEBUG
							std::cout << ":(" << std::endl;
							#endif
						}
					}
				}
				else
				{
					#ifdef _DEBUG
					std::cout << "x: " << x << "; y: " << y << std::endl;
					#endif
					
					m_selected_piece = NULL;
				}
				selected = false;
			}

			// highlight selected piece and its corresponding moves, when moves exist
			if (m_selected_piece != NULL)
			{
				if (!(m_selected_piece->get_av_list()->empty()))
				{
					highlight_selected(m_window, m_selected_piece->get_x(), m_selected_piece->get_y());
					for_each(m_selected_piece->get_av_list()->begin(), m_selected_piece->get_av_list()->end(), [this](available_move* a) { highlight_available(m_window, a->get_x(), a->get_y()); });
				}
				else
					m_selected_piece = NULL;
			}

			// print alive pieces
			for (int i = 0; i < size; ++i)
				for (int j = 0; j < size; ++j)
					if ((*m_board)[i][j] != NULL)
						(*m_board)[i][j]->draw(m_window);

			// print pieces in multicapture
			for_each(m_to_delete_list.begin(), m_to_delete_list.end(), [this](piece* p) { p->draw(m_window); });

			m_window.display();
		}
		sf::Time elapsed_time = m_clock.restart();

		if (elapsed_time.asSeconds() < m_frame_duration)
			sf::sleep(sf::seconds(m_frame_duration - elapsed_time.asSeconds()));
	}

	void game::play_in_console(void)
	{

	}

	std::vector<std::vector<piece*>>* game::get_board(void) { return m_board; }

	/*void game::rotate_board(void)
	{
		is_rotated = !is_rotated;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size - i; ++j)
			{
				piece* tmp = (*board)[i][j];
				(*board)[i][j] = (*board)[size - 1 - i][size - 1 - j];
				(*board)[size - 1 - i][size - 1 - j] = tmp;

				if (i == j)
				{
					piece* tmp = (*board)[i][size - 1 - i];
					(*board)[i][size - 1 - i] = (*board)[size - 1 - i][i];
					(*board)[size - 1 - i][i] = tmp;
				}
			}
		}
	}*/

	void game::print_results(std::ostream& os)
	{
		if (!m_first_won && !m_second_won)
			std::cout << "game wasn't finished" << std::endl;
		else if (m_first_won && m_second_won)
			std::cout << "Draw" << std::endl;
		else if (m_first_won)
			std::cout << "player: \"" << m_player_1->get_name() << "\" won!" << std::endl;
		else
			std::cout << "player: \"" << m_player_2->get_name() << "\" won!" << std::endl;
	}


	void game::draw(sf::RenderWindow& window)
	{
		sf::RectangleShape tile;
		tile.setSize(sf::Vector2f(square_size, square_size));
		for (int i = 0; i < size; i++)
		{
			for (int j = 0; j < size; j++)
			{
				tile.setPosition(sf::Vector2f(square_size * i, square_size * j));
				if ((i + j) % 2 == 0)
					tile.setFillColor(sf::Color(193, 173, 158, 255));
				else
					tile.setFillColor(sf::Color(133, 94, 66, 255));
				window.draw(tile);
			}
		}
	}

	void game::highlight_selected(sf::RenderWindow& window, int x, int y)
	{
		sf::RectangleShape tile;
		tile.setSize(sf::Vector2f(square_size, square_size));
		tile.setFillColor(sf::Color(173, 134, 106, 255));
		tile.setPosition(sf::Vector2f(square_size * x, square_size * y));
		window.draw(tile);
	}

	void game::highlight_available(sf::RenderWindow& window, int x, int y)
	{
		sf::RectangleShape tile;
		tile.setSize(sf::Vector2f(square_size, square_size));
		tile.setFillColor(sf::Color(103, 194, 106, 255));
		tile.setPosition(sf::Vector2f(square_size * x, square_size * y));
		window.draw(tile);
	}

	bool game::evaluate(std::list<piece*> list, std::vector<std::vector<piece*>>* board_p, int* counter, base_player* player)
	{
		bool av_capture = false;
		for_each(list.begin(), list.end(), [this, &board_p, &list, &counter, &av_capture, &player](piece* p)
			{
				int x = p->get_x();
				int y = p->get_y();
				
				#ifdef _DEBUG
				std::cout << "evaluating" << std::endl;
				std::cout << "x: " << x << "; y: " << y << std::endl;

				if ((*board_p)[x][y] != NULL)
					std::cout << (*board_p)[x][y] << std::endl;
				#endif

				// captures
				bool possible_capture_top_left = false;
				bool possible_capture_top_right = false;
				bool possible_capture_bottow_left = false;
				bool possible_capture_bottom_right = false;

				// capture top right (0)
				if (x + 2 <= size - 1 && y - 2 >= 0 && (*board_p)[x + 1][y - 1] != NULL && (*board_p)[x + 1][y - 1]->get_sign() == player->get_next_player()->get_sign() && !((*board_p)[x + 1][y - 1]->is_captured()) && (*board_p)[x + 2][y - 2] == NULL)
					possible_capture_top_right = true;

				// capture top left (1)
				if (x - 2 >= 0 && y - 2 >= 0 && (*board_p)[x - 1][y - 1] != NULL && (*board_p)[x - 1][y - 1]->get_sign() == player->get_next_player()->get_sign() && !((*board_p)[x - 1][y - 1]->is_captured()) && (*board_p)[x - 2][y - 2] == NULL)
					possible_capture_top_left = true;

				// capture bottom right (2)
				if (x + 2 <= size - 1 && y + 2 <= size - 1 && (*board_p)[x + 1][y + 1] != NULL && (*board_p)[x + 1][y + 1]->get_sign() == player->get_next_player()->get_sign() && !((*board_p)[x + 1][y + 1]->is_captured()) && (*board_p)[x + 2][y + 2] == NULL)
					possible_capture_bottom_right = true;

				// capture bottom left (3)
				if (x - 2 >= 0 && y + 2 <= size - 1 && (*board_p)[x - 1][y + 1] != NULL && (*board_p)[x - 1][y + 1]->get_sign() == player->get_next_player()->get_sign() && !((*board_p)[x - 1][y + 1]->is_captured()) && (*board_p)[x - 2][y + 2] == NULL)
					possible_capture_bottow_left = true;


				if (possible_capture_top_left || possible_capture_top_right || possible_capture_bottow_left || possible_capture_bottom_right)
				{
					av_capture = true;

					// evaluate copy of the board recursively in every direction and find highest number of captures to add to base moves list
					int capture_counter[4] = { 0, 0, 0, 0 }; // 0 - top right, 1 - top left, 2 - bottom right, 3 - bottom left

					if (possible_capture_top_right)
					{
						//(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
						capture_counter[0] = 1; // change here to get from counter, then increment?

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(size, std::vector<piece*>(size, 0));
						std::list<piece*> copy_of_list;
						if (*counter == NULL) // first call of evaluation
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*m_board)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());
							
						}
						else // the function call is being recursive 
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*board_p)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*board_p)[i][j]->get_sign(), (*board_p)[i][j]->get_x(), (*board_p)[i][j]->get_y());
						}

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y];
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x + 2);
						moving_piece->set_y(y - 2);
						(*copy_of_board)[x + 2][y - 2] = moving_piece;
						copy_of_list.push_back(moving_piece);
						(*copy_of_board)[x + 1][y - 1] = NULL;
						moving_piece = NULL; // now, copy of board contains board with moved piece and the list contains only moved piece
						std::cout << copy_of_board << std::endl;
						

						//evaluate recursively - separate in every direction - call tree
						if (*counter == NULL)
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter null" << std::endl;
							#endif

							int moves = 1;
							evaluate(copy_of_list, copy_of_board, &moves, player);
							capture_counter[0] = moves;

							#ifdef _DEBUG
							std::cout << "moves counter (top right): " << moves << std::endl;
							#endif
						}
						else
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter not null" << std::endl;
							#endif

							(*counter)++;
							evaluate(copy_of_list, copy_of_board, counter, player);
						}
					}

					if (possible_capture_top_left)
					{
						//(*p).get_av_list()->push_back(new available_capture(x - 2, y - 2, x - 1, y - 1));
						capture_counter[1] = 1;

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(size, std::vector<piece*>(size, 0));
						std::list<piece*> copy_of_list;
						if (*counter == NULL) // first call of evaluation
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*m_board)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

						}
						else // the function call is being recursive
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*board_p)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*board_p)[i][j]->get_sign(), (*board_p)[i][j]->get_x(), (*board_p)[i][j]->get_y());
						}

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y];
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x - 2);
						moving_piece->set_y(y - 2);
						(*copy_of_board)[x - 2][y - 2] = moving_piece;
						copy_of_list.push_back(moving_piece);
						(*copy_of_board)[x - 1][y - 1] = NULL;
						moving_piece = NULL; // now, copy of board contains board with moved piece and the list contains only moved piece
						std::cout << copy_of_board << std::endl;


						//evaluate recursively - separate in every direction - call tree
						if (*counter == NULL)
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter null" << std::endl;
							#endif

							int moves = 1;
							evaluate(copy_of_list, copy_of_board, &moves, player);
							capture_counter[1] = moves;
							
							#ifdef _DEBUG
							std::cout << "moves counter (top left): " << moves << std::endl;
							#endif
						}
						else
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter not null" << std::endl;
							#endif
							
							(*counter)++;
							evaluate(copy_of_list, copy_of_board, counter, player);
						}
					}

					if (possible_capture_bottom_right)
					{
						//(*p).get_av_list()->push_back(new available_capture(x + 2, y + 2, x + 1, y + 1));
						capture_counter[2] = 1;

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(size, std::vector<piece*>(size, 0));
						std::list<piece*> copy_of_list;
						if (*counter == NULL) // first call of evaluation
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*m_board)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

						}
						else // the function call is being recursive 
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*board_p)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*board_p)[i][j]->get_sign(), (*board_p)[i][j]->get_x(), (*board_p)[i][j]->get_y());
						}

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y];
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x + 2);
						moving_piece->set_y(y + 2);
						(*copy_of_board)[x + 2][y + 2] = moving_piece;
						copy_of_list.push_back(moving_piece);
						(*copy_of_board)[x + 1][y + 1] = NULL;
						moving_piece = NULL; // now, copy of board contains board with moved piece and the list contains only moved piece
						std::cout << copy_of_board << std::endl;


						//evaluate recursively - separate in every direction - call tree
						if (*counter == NULL)
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter null" << std::endl;
							#endif
							
							int moves = 1;
							evaluate(copy_of_list, copy_of_board, &moves, player);
							capture_counter[2] = moves;
							
							#ifdef _DEBUG	
							std::cout << "moves counter (bottom right): " << moves << std::endl;
							#endif
						}
						else
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter not null" << std::endl;
							#endif

							(*counter)++;
							evaluate(copy_of_list, copy_of_board, counter, player);
						}
					}

					if (possible_capture_bottow_left)
					{
						//(*p).get_av_list()->push_back(new available_capture(x - 2, y + 2, x - 1, y + 1));
						capture_counter[3] = 1;

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(size, std::vector<piece*>(size, 0));
						std::list<piece*> copy_of_list;
						if (*counter == NULL) // first call of evaluation
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*m_board)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

						}
						else // the function call is being recursive
						{
							for (int i = 0; i < size; ++i)
								for (int j = 0; j < size; ++j)
									if ((*board_p)[i][j] != NULL)
										(*copy_of_board)[i][j] = new piece((*board_p)[i][j]->get_sign(), (*board_p)[i][j]->get_x(), (*board_p)[i][j]->get_y());
						}

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y];
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x - 2);
						moving_piece->set_y(y + 2);
						(*copy_of_board)[x - 2][y + 2] = moving_piece;
						copy_of_list.push_back(moving_piece);
						(*copy_of_board)[x - 1][y + 1] = NULL;
						moving_piece = NULL; // now, copy of board contains board with moved piece and the list contains only moved piece
						#ifdef _DEBUG
						std::cout << copy_of_board << std::endl;
						#endif	

						//evaluate recursively - separate in every direction - call tree
						if (*counter == NULL)
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter null" << std::endl;
							#endif
							
							int moves = 1;
							evaluate(copy_of_list, copy_of_board, &moves, player);
							capture_counter[3] = moves;
							
							#ifdef _DEBUG
							std::cout << "moves counter (bottom left): " << moves << std::endl;
							#endif
						}
						else
						{
							#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter not null" << std::endl;
							#endif

							(*counter)++;
							evaluate(copy_of_list, copy_of_board, counter, player);
						}
					} // all recursive function made

					// tmp: write all counters

					//find max counter
					int max = capture_counter[0];
					for (int i = 1; i < 4; ++i)
						if (capture_counter[i] > max)
							max = capture_counter[i];
					#ifdef _DEBUG
					std::cout << "found max counter: " << max << std::endl;
					#endif

					// if counter == max push back available capture
					for (int i = 0; i < 4; ++i)
						if (capture_counter[i] == max)
						{
							switch (i)
							{
							case 0:
							{
								#ifdef _DEBUG
								std::cout << "kierunek top right: " << capture_counter[0] << std::endl;
								#endif
								(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
								break;
							}
							case 1:
							{
								#ifdef _DEBUG
								std::cout << "kierunek top left: " << capture_counter[1] << std::endl;
								#endif
								(*p).get_av_list()->push_back(new available_capture(x - 2, y - 2, x - 1, y - 1));
								break;
							}
							case 2:
							{
								#ifdef _DEBUG
								std::cout << "kierunek bottom right: " << capture_counter[2] << std::endl;
								#endif
								(*p).get_av_list()->push_back(new available_capture(x + 2, y + 2, x + 1, y + 1));
								break;
							}
							case 3:
							{
								#ifdef _DEBUG
								std::cout << "kierunek bottom left: " << capture_counter[3] << std::endl;
								#endif
								(*p).get_av_list()->push_back(new available_capture(x - 2, y + 2, x - 1, y + 1));
								break;
							}
							default:
								throw std::exception("Evaluation error on piece " + p->get_sign());
							}
						}
				}
				else
				{
					if (*counter != NULL)
						return;

					// different direction of ordinary move
					if (player->is_first())
					{
						// moves to right
						if (x != size - 1 && y != 0)
						{
							if ((*board_p)[x + 1][y - 1] == NULL)
							{
								#ifdef _DEBUG
								std::cout << "available move to the right!" << std::endl;
								#endif
								(*p).get_av_list()->push_back(new available_move(x + 1, y - 1));
							}
						}

						// moves to left
						if (x != 0 && y != 0)
						{
							if ((*board_p)[x - 1][y - 1] == NULL)
							{
								#ifdef _DEBUG
								std::cout << "available move to the left!" << std::endl;
								#endif
								(*p).get_av_list()->push_back(new available_move(x - 1, y - 1));
							}
						}
					}
					else // next player is primary player (first, lower on board): checked by assertions
					{
						// moves to right
						if (x != size - 1 && y != size - 1)
						{
							if ((*board_p)[x + 1][y + 1] == NULL)
							{
								#ifdef _DEBUG
								std::cout << "available move to the right!" << std::endl;
								#endif		
								(*p).get_av_list()->push_back(new available_move(x + 1, y + 1));
							}
						}

						// moves to left
						if (x != 0 && y != size - 1)
						{
							if ((*board_p)[x - 1][y + 1] == NULL)
							{
								#ifdef _DEBUG
								std::cout << "available move to the left!" << std::endl;
								#endif
								(*p).get_av_list()->push_back(new available_move(x - 1, y + 1));
							}
						}
					}
				}

			});
			#ifdef _DEBUG
			std::cout << "---available returning: ";
			av_capture ? (std::cout << "true") : (std::cout << "false");
			std::cout << std::endl;
			#endif
			return av_capture;
	}

	void game::clear_list(std::list<piece*>* list) { for_each(list->begin(), list->end(), [this](piece* p) { p->get_av_list()->clear(); }); }
	
	void game::print_pieces(std::list<piece*>* list, std::ostream& os) { std::for_each(list->begin(), list->end(), [i = 1, this, &os](piece* p) mutable { os << i++ << "; sign: " << p << "; x: " << p->get_x() << "; y: " << p->get_y() << std::endl; }); }

	void game::delete_from_list(std::list<piece*>* list, piece* piece_to_delete) { list->remove(piece_to_delete); }

	void game::clear_to_delete_list(std::list<piece*>* del_list, std::list<piece*>* src_list)
	{
		while (!(del_list->empty()))
		{
			// temporary piece from "to delete list"
			piece* tmp = del_list->front();

			int x_d = tmp->get_x();
			int y_d = tmp->get_y();
			piece* piece_to_delete = (*m_board)[x_d][y_d];
			(*m_board)[x_d][y_d] = NULL;

			delete_from_list(src_list, piece_to_delete);

			del_list->pop_front();
			//delete tmp;
		}
	}
}