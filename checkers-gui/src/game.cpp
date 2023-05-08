#include "include/game.h"
#include "include/king.h"

// todo:
// remake code into more functions, current player and opponent pointers
// add menu
// optimise drawing
// add animation
// add setting rows and check if set correctly
// switch to polymorphism completely (get rid of is_king flag)
// evaluate kings
// consider current moving piece to eliminate situation where two pieces have possible captures
// add setting one piece as a function with check

namespace checkers
{
	game::game(int fps) : m_is_finished(false), m_fps(fps),
		m_window(sf::VideoMode(s_square_size* s_size, s_square_size* s_size), "Checkers", sf::Style::Default, m_settings),
		m_selected_piece(NULL), m_moving_piece(NULL), m_available_capture(false), m_tile(), m_clock(), m_event(), m_settings(), m_current_player(NULL)
	{
		// todo: menu

		// simplified: players init
		m_player_1 = new player('W', "Player1");
		m_player_2 = new player('B', "Player2");

		// set play order and evaluation direction
		m_player_1->set_first(true);
		m_player_2->set_first(false);
		m_player_1->set_next_player(m_player_2);
		m_player_2->set_next_player(m_player_1);
		m_current_player = m_player_1;
		
		// board init
		m_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));

		// fill the board with pieces
		//populate_board();
		populate_board_debug();

		// set pointers to piece lists
		m_player_1->set_list(&m_p_list_1);
		m_player_2->set_list(&m_p_list_2);

		// SFML setup, using original methods in camelCase style instead of snake_case
		m_settings.antialiasingLevel = 8;
		m_window.setFramerateLimit(fps);
		m_window.setVerticalSyncEnabled(true);
		m_tile.setSize(sf::Vector2f(s_square_size, s_square_size));

		// evaluate available moves for the first player
		int dummy = 0;
		m_available_capture = evaluate(&m_p_list_1, m_board, &dummy, m_player_1);

#ifdef _DEBUG
		std::cout << "List of pieces of first player" << std::endl;
		print_pieces(&m_p_list_1);

		std::cout << "List of pieces of second player" << std::endl;
		print_pieces(&m_p_list_2);
#endif
	}

	std::ostream& operator<<(std::ostream& os, const std::vector<std::vector<piece*>>* board)
	{
		os << "\t  ";
		for (char a = 'a'; a < 'a' + checkers::s_size; ++a) // colums as letters
			os << a << "   ";
		/*for (int i = 0; i < checkers::size; ++i)
			os << i << "   ";*/
		os << std::endl << std::endl << std::endl;
		for (int i = 0; i < checkers::s_size; ++i)
		{
			//os << game::size - i << "\t| ";
			os << i << "\t| ";
			for (int j = 0; j < s_size; ++j)
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

	void game::switch_turn(void)
	{
		m_current_player = m_current_player->get_next_player();
#ifdef _DEBUG
		std::cout << "Current player: " << m_current_player->get_name() << std::endl;
#endif // _DEBUG
	}

	void game::populate_board(void)
	{
		assert(m_board->size() == s_size);

		// todo: change to algorithm
		// rows of the second player (upper)
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < s_size; ++j)
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
		for (int i = s_size - 1; i >= s_size - 3; --i)
		{
			for (int j = 0; j < s_size; ++j)
			{
				if ((i + j) % 2 != 0)
				{
					(*m_board)[j][i] = new piece(m_player_1->get_sign(), j, i);
					m_p_list_1.push_back((*m_board)[j][i]);
					m_player_1->add_piece();
				}
			}
		}
	}

	void game::populate_board_debug(void)
	{
		assert(m_board->size() == s_size);

		// todo: change to algorithm
		// rows of the second player (upper)
		int j = 1;
		for (int i = 0; i < 8; ++i)
		{
			if ((i + j) % 2 != 0)
			{
				(*m_board)[j][i] = new piece(m_player_2->get_sign(), j, i);
				m_p_list_2.push_back((*m_board)[j][i]);
				m_player_2->add_piece();
			}
			++j;
			if (j >= 3)
				j = 1;
		}
		// rows of the first player (lower)
		j = 9;
		for (int i = s_size - 1; i >= s_size - 8; --i)
		{
			if ((i + j) % 2 != 0)
			{
				(*m_board)[j][i] = new piece(m_player_1->get_sign(), j, i);
				m_p_list_1.push_back((*m_board)[j][i]);
				m_player_1->add_piece();
			}
			--j;
			if (j >= 6)
				j = 9;
		}

		(*m_board)[7][8] = new piece(m_player_1->get_sign(), 7, 8);
		m_p_list_1.push_back((*m_board)[7][8]);
		m_player_1->add_piece();

		(*m_board)[8][7] = new piece(m_player_1->get_sign(), 8, 7);
		m_p_list_1.push_back((*m_board)[8][7]);
		m_player_1->add_piece();

		(*m_board)[6][9] = new piece(m_player_1->get_sign(), 6, 9);
		m_p_list_1.push_back((*m_board)[6][9]);
		m_player_1->add_piece();
	}

	std::vector<std::vector<piece*>>* game::get_board(void) { return m_board; }

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
		std::cout << "player " << m_player_1->get_name() << "'s score: " << m_player_2->get_captured_pieces() << "; player " << m_player_2->get_name() << "'s score: " << m_player_1->get_captured_pieces() << std::endl;
	}

	void game::draw(sf::RenderWindow& window)
	{
		for (int i = 0; i < s_size; i++)
		{
			for (int j = 0; j < s_size; j++)
			{
				m_tile.setPosition(sf::Vector2f(s_square_size * i, s_square_size * j));
				if ((i + j) % 2 == 0)
					m_tile.setFillColor(sf::Color(193, 173, 158, 255));
				else
					m_tile.setFillColor(sf::Color(133, 94, 66, 255));
				window.draw(m_tile);
			}
		}
	}

	void game::highlight_selected(sf::RenderWindow& window, int x, int y)
	{
		sf::RectangleShape selected_tile;
		selected_tile.setSize(sf::Vector2f(s_square_size, s_square_size));
		selected_tile.setFillColor(sf::Color(173, 134, 106, 255));
		selected_tile.setPosition(sf::Vector2f(s_square_size * x, s_square_size * y));
		window.draw(selected_tile);
	}

	void game::highlight_available(sf::RenderWindow& window, int x, int y)
	{
		sf::RectangleShape available_tile;
		available_tile.setSize(sf::Vector2f(s_square_size, s_square_size));
		available_tile.setFillColor(sf::Color(103, 194, 106, 255));
		available_tile.setPosition(sf::Vector2f(s_square_size * x, s_square_size * y));
		window.draw(available_tile);
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

				if (m_event.type == sf::Event::MouseButtonPressed && m_event.mouseButton.button == sf::Mouse::Left)
				{
					if (m_selected_piece != NULL) // choice after highlighting
					{
						// getting coords of the click after highlighting selected piece, ignore clicks outside
						int x = sf::Mouse::getPosition(m_window).x / (m_window.getSize().x / s_size);
						int y = sf::Mouse::getPosition(m_window).y / (m_window.getSize().y / s_size);
						if (x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1)
							break;

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
								// temporary: delete to debug
								piece* piece_to_delete = (*m_board)[x_d][y_d];
								(*m_board)[x_d][y_d] = NULL;

								m_current_player->get_next_player()->make_capture();
								delete_from_list(m_current_player->get_next_player()->get_list(), piece_to_delete);

								if (m_current_player->get_next_player()->get_list()->empty())
								{
									if (m_current_player->is_first())
										m_first_won = true;
									else
										m_second_won = true;
								}

								// create new piece which represents dead piece during multicapture
								m_to_delete_list.push_back(new piece(std::tolower(m_current_player->get_next_player()->get_sign()), x_d, y_d));
								m_current_player->set_combo(true);
#ifdef _DEBUG
								std::cout << m_current_player->get_name() << " combo" << std::endl;
#endif
							}

							// move the piece (piece, which is moving -> both capture and normal move), keep selected_piece pointer for the possible king
							(*m_board)[m_selected_piece->get_x()][m_selected_piece->get_y()] = NULL;
							m_selected_piece->set_x(x);
							m_selected_piece->set_y(y);
							(*m_board)[x][y] = m_selected_piece;
							selected = false;

#ifdef _DEBUG
							std::cout << "List of pieces of first player" << std::endl;
							print_pieces(&m_p_list_1);
							std::cout << "List of pieces of second player" << std::endl;
							print_pieces(&m_p_list_2);
#endif		

							// tmp flag indicating, that the king check was made this round
							bool made_king_check = false;

							// switch turn, if no combo
							if (!m_player_1->get_combo() && !m_player_2->get_combo())
							{
								// king function
								if (!made_king_check)
									m_current_player->kings(m_selected_piece, m_board);
								made_king_check = true;
								switch_turn();
							}
							else // section to test (fixes stuff)
							{
								clear_list(&m_p_list_1);
								clear_list(&m_p_list_2);
							}

							// evaluate current player and check if there is more captures, if not, check for new kings
							int dummy = 0;
							m_available_capture = evaluate(m_current_player->get_list(), m_board, &dummy, m_current_player);

							// exit the combo, if no more captures
							if (m_current_player->get_combo() && !m_available_capture)
							{
								// delete opponent's pieces of multi capture, clear failed list of possible moves, cancel combo, evaluate again
								clear_to_delete_list(&m_to_delete_list, &m_p_list_1);
								clear_to_delete_list(&m_to_delete_list, &m_p_list_2);

								clear_list(&m_p_list_1);
								clear_list(&m_p_list_2);
								m_current_player->set_combo(false);
								m_current_player->get_next_player()->set_combo(false);
#ifdef _DEBUG
								std::cout << "Combo cancelled" << std::endl;
#endif
								// king function
								if (!made_king_check)
									m_current_player->kings(m_selected_piece, m_board);
								made_king_check = true;
								switch_turn();
								m_available_capture = evaluate(m_current_player->get_list(), m_board, &dummy, m_current_player);
							}
							else // continue the combo
								clear_list(m_current_player->get_next_player()->get_list());

							m_selected_piece = NULL;

							// check for empty evaluation?
						}
					}
					else
						selected = !selected;
				}
			}

			// end of the game
			if (m_first_won || m_second_won)
			{
				std::cout << "Game is finished" << std::endl;
				break;
			}

			//m_window.clear();
			draw(m_window);

			// first choice, nothing is already highlighted
			if (selected)
			{
				m_selected_piece = NULL;

				int x = sf::Mouse::getPosition(m_window).x / (m_window.getSize().x / s_size);
				int y = sf::Mouse::getPosition(m_window).y / (m_window.getSize().y / s_size);
				if (x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1)
					break;

				// check if the correspoding field contains a piece
				if ((*m_board)[x][y] != NULL)
				{
#ifdef _DEBUG
					std::cout << "x: " << x << "; y: " << y << "; piece: " << (*m_board)[x][y] << std::endl;
#endif
					// check if player owns this piece
					if ((*m_board)[x][y]->get_sign() == m_current_player->get_sign())
					{
#ifdef _DEBUG
						std::cout << "That piece belongs to you" << std::endl;
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
						std::cout << "That piece does not belong to you" << std::endl;
#endif
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
			for (int i = 0; i < s_size; ++i)
				for (int j = 0; j < s_size; ++j)
					if ((*m_board)[i][j] != NULL)
						(*m_board)[i][j]->draw(m_window);

			// print pieces in multicapture
			for_each(m_to_delete_list.begin(), m_to_delete_list.end(), [this](piece* p) { p->draw(m_window); });

			// sleep time complementary to the frame time
			sf::Time elapsed_time = m_clock.restart();
			if (elapsed_time.asSeconds() < m_frame_duration)
				sf::sleep(sf::seconds(m_frame_duration - elapsed_time.asSeconds()));
			m_window.display();
		}
		print_results();
	}

	bool game::evaluate(std::list<piece*>* list, std::vector<std::vector<piece*>>* board, int* counter, base_player* player)
	{
		bool av_capture = false;

		// todo: list with each piece with a possible capture and corresponding score, after for_each add capture for only the higest score
		for_each(list->begin(), list->end(), [this, &board, &list, &counter, &av_capture, &player](piece* p)
			{
				// x coordinate of evaluated piece
				int x = p->get_x();
				// y coordinate of evaluated piece
				int y = p->get_y();

				// todo: cancel possible moves or captures if another type (piece or king) has higher score
				// or change to one common moves enabler
				if (!p->is_king())
				{
#ifdef _DEBUG
					std::cout << "evaluating normal piece" << std::endl;
					std::cout << "x: " << x << "; y: " << y << std::endl;

					if ((*board)[x][y] != NULL)
						std::cout << (*board)[x][y] << std::endl;
#endif
					// captures
					bool possible_capture_top_left = false;
					bool possible_capture_top_right = false;
					bool possible_capture_bottow_left = false;
					bool possible_capture_bottom_right = false;

					// capture top right (0)
					if (x + 2 <= s_size - 1 && y - 2 >= 0 && (*board)[x + 1][y - 1] != NULL && (*board)[x + 1][y - 1]->get_sign() == player->get_next_player()->get_sign() && (*board)[x + 2][y - 2] == NULL)
						possible_capture_top_right = true;

					// capture top left (1)
					if (x - 2 >= 0 && y - 2 >= 0 && (*board)[x - 1][y - 1] != NULL && (*board)[x - 1][y - 1]->get_sign() == player->get_next_player()->get_sign() && (*board)[x - 2][y - 2] == NULL)
						possible_capture_top_left = true;

					// capture bottom right (2)
					if (x + 2 <= s_size - 1 && y + 2 <= s_size - 1 && (*board)[x + 1][y + 1] != NULL && (*board)[x + 1][y + 1]->get_sign() == player->get_next_player()->get_sign() && (*board)[x + 2][y + 2] == NULL)
						possible_capture_bottom_right = true;

					// capture bottom left (3)
					if (x - 2 >= 0 && y + 2 <= s_size - 1 && (*board)[x - 1][y + 1] != NULL && (*board)[x - 1][y + 1]->get_sign() == player->get_next_player()->get_sign() && (*board)[x - 2][y + 2] == NULL)
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
							std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
							std::list<piece*> copy_of_list;
							if (*counter == NULL) // first call of evaluation
							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*m_board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

							}
							else // the function call is being recursive 
							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
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
								evaluate(&copy_of_list, copy_of_board, &moves, player);
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
								evaluate(&copy_of_list, copy_of_board, counter, player);
							}
						}

						if (possible_capture_top_left)
						{
							//(*p).get_av_list()->push_back(new available_capture(x - 2, y - 2, x - 1, y - 1));
							capture_counter[1] = 1;

							// copy the board and make empty list for moved piece
							std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
							std::list<piece*> copy_of_list;
							if (*counter == NULL) // first call of evaluation
							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*m_board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

							}
							else // the function call is being recursive
							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
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
								evaluate(&copy_of_list, copy_of_board, &moves, player);
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
								evaluate(&copy_of_list, copy_of_board, counter, player);
							}
						}

						if (possible_capture_bottom_right)
						{
							//(*p).get_av_list()->push_back(new available_capture(x + 2, y + 2, x + 1, y + 1));
							capture_counter[2] = 1;

							// copy the board and make empty list for moved piece
							std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
							std::list<piece*> copy_of_list;
							if (*counter == NULL) // first call of evaluation

							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*m_board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

							}
							else // the function call is being recursive 
							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
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
								evaluate(&copy_of_list, copy_of_board, &moves, player);
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
								evaluate(&copy_of_list, copy_of_board, counter, player);
							}
						}

						if (possible_capture_bottow_left)
						{
							//(*p).get_av_list()->push_back(new available_capture(x - 2, y + 2, x - 1, y + 1));
							capture_counter[3] = 1;

							// copy the board and make empty list for moved piece
							std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
							std::list<piece*> copy_of_list;
							if (*counter == NULL) // first call of evaluation
							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*m_board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

							}
							else // the function call is being recursive
							{
								for (int i = 0; i < s_size; ++i)
									for (int j = 0; j < s_size; ++j)
										if ((*board)[i][j] != NULL)
											(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
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
								evaluate(&copy_of_list, copy_of_board, &moves, player);
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
								evaluate(&copy_of_list, copy_of_board, counter, player);
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
									std::cout << "top right direction: " << capture_counter[0] << std::endl;
#endif
									(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
									break;
								}
								case 1:
								{
#ifdef _DEBUG
									std::cout << "top left direction: " << capture_counter[1] << std::endl;
#endif
									(*p).get_av_list()->push_back(new available_capture(x - 2, y - 2, x - 1, y - 1));
									break;
								}
								case 2:
								{
#ifdef _DEBUG
									std::cout << "bottom right direction: " << capture_counter[2] << std::endl;
#endif
									(*p).get_av_list()->push_back(new available_capture(x + 2, y + 2, x + 1, y + 1));
									break;
								}
								case 3:
								{
#ifdef _DEBUG
									std::cout << "bottom left direction: " << capture_counter[3] << std::endl;
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
							if (x != s_size - 1 && y != 0)
							{
								if ((*board)[x + 1][y - 1] == NULL)
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
								if ((*board)[x - 1][y - 1] == NULL)
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
							if (x != s_size - 1 && y != s_size - 1)
							{
								if ((*board)[x + 1][y + 1] == NULL)
								{
#ifdef _DEBUG
									std::cout << "available move to the right!" << std::endl;
#endif		
									(*p).get_av_list()->push_back(new available_move(x + 1, y + 1));
								}
							}

							// moves to left
							if (x != 0 && y != s_size - 1)
							{
								if ((*board)[x - 1][y + 1] == NULL)
								{
#ifdef _DEBUG
									std::cout << "available move to the left!" << std::endl;
#endif
									(*p).get_av_list()->push_back(new available_move(x - 1, y + 1));
								}
							}
						}
					}
				}
				else // king piece
				{
#ifdef _DEBUG
					std::cout << "evaluating the king" << std::endl;
					std::cout << "x: " << x << "; y: " << y << std::endl;

					if ((*board)[x][y] != NULL)
						std::cout << (*board)[x][y] << std::endl;
#endif
					// check for possible captures (change to lists with true only?)
					// vector, where true indicates a piece to be captured on specific place, it can be captured going into a few separate locations
					std::vector<bool> possible_capture_top_right;
					std::vector<bool> possible_capture_top_left;
					std::vector<bool> possible_capture_bottom_right;
					std::vector<bool> possible_capture_bottom_left;

					// condensed form of the lists above
					bool at_least_one_capture_top_right = false;
					bool at_least_one_capture_top_left = false;
					bool at_least_one_capture_bottom_right = false;
					bool at_least_one_capture_bottom_left = false;

					// lists (vectors) of moves, that can be made when no capture is available
					std::vector<available_move*> local_moves_top_right; // change to not a pointer
					std::vector<available_move*> local_moves_top_left; // change to not a pointer
					std::vector<available_move*> local_moves_bottom_right; // change to not a pointer
					std::vector<available_move*> local_moves_bottom_left; // change to not a pointer

					// lists (vectors) of captures: one capture in each direction, every in possible location
					std::vector<available_capture*> local_captures_top_right;
					std::vector<available_capture*> local_captures_top_left;
					std::vector<available_capture*> local_captures_bottom_right;
					std::vector<available_capture*> local_captures_bottom_left;

					// capture top right (0) + -
#ifdef _DEBUG
					std::cout << "top right checking" << std::endl;
#endif
					int i = 1;
					while (x + i + 1 <= s_size - 1 && y - i - 1 >= 0)
					{
#ifdef _DEBUG
						std::cout << "checking: x: " << x + i << ", y: " << y - i << " and place to go: x: " << x + i + 1 << ", y: " << y - i - 1 << std::endl;
#endif
						// searching for own piece (cannot jump across them)
						if ((*board)[x + i][y - i] != NULL && (*board)[x + i][y - i]->get_sign() == player->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found own piece and breaking" << std::endl;
#endif
							possible_capture_top_right.push_back(false);
							break;
						}
						else if ((*board)[x + i][y - i] != NULL && (*board)[x + i][y - i]->get_sign() == player->get_next_player()->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
							if ((*board)[x + i + 1][y - i - 1] == NULL)
							{
#ifdef _DEBUG
								std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
								possible_capture_top_right.push_back(true);
								at_least_one_capture_top_right = true;

								// adding all possible capture options
								int j = 0;
								while (x + i + 1 + j <= s_size - 1 && y - i - 1 - j >= 0 && (*board)[x + i + 1 + j][y - i - 1 - j] == NULL)
								{
#ifdef _DEBUG
									std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x + i + 1 + j << ", y: " << y - i - 1 - j << std::endl;
#endif
									local_captures_top_right.push_back(new available_capture(x + i + 1 + j, y - i - 1 - j, x + i, y - i));
									++j;
								}
#ifdef _DEBUG
								std::cout << "found and added all possible capture options, continuing" << std::endl;
#endif
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*next piece is not empty and braking" << std::endl;
#endif
								possible_capture_top_right.push_back(false);
								break;
							}
						}
						else // empty field
						{
							if ((*board)[x + i + 1][y - i - 1] != NULL)
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty and next field is not empty" << std::endl;
#endif
								if (at_least_one_capture_top_right)
								{
#ifdef _DEBUG
									std::cout << "*no more places to check, it is a singular check" << std::endl;
#endif
									break;
								}
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty" << std::endl;
#endif
							}
							local_moves_top_right.push_back(new available_move(x + i, y - i));
							possible_capture_top_right.push_back(false);
						}
						
						++i;
					}
					if (x + i <= s_size - 1 && y - i >= 0 && (*board)[x + i][y - i] == NULL && !at_least_one_capture_top_right) // last, not checked field (checking looks for next which is outside the boundaries)
					{
#ifdef _DEBUG
						std::cout << "last but not least" << std::endl;
#endif
						local_moves_top_right.push_back(new available_move(x + i, y - i));
					}

					// capture top left (1) - -
#ifdef _DEBUG
					std::cout << "top left checking" << std::endl;
#endif
					i = 1;
					while (x - i - 1 >= 0 && y - i - 1 >= 0)
					{
#ifdef _DEBUG
						std::cout << "checking: x: " << x - i << ", y: " << y - i << " and place to go: x: " << x - i - 1 << ", y: " << y - i - 1 << std::endl;
#endif
						// searching for own piece (cannot jump across them)
						if ((*board)[x - i][y - i] != NULL && (*board)[x - i][y - i]->get_sign() == player->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found own piece and breaking" << std::endl;
#endif
							possible_capture_top_left.push_back(false);
							break;
						}
						else if ((*board)[x - i][y - i] != NULL && (*board)[x - i][y - i]->get_sign() == player->get_next_player()->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
							if ((*board)[x - i - 1][y - i - 1] == NULL)
							{
#ifdef _DEBUG
								std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
								possible_capture_top_left.push_back(true);
								at_least_one_capture_top_left = true;

								// adding all possible capture options
								int j = 0;
								while (x - i - 1 - j >= 0 && y - i - 1 - j >= 0 && (*board)[x - i - 1 - j][y - i - 1 - j] == NULL)
								{
#ifdef _DEBUG
									std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x - i - 1 - j << ", y: " << y - i - 1 - j << std::endl;
#endif
									local_captures_top_left.push_back(new available_capture(x - i - 1 - j, y - i - 1 - j, x - i, y - i));
									++j;
								}
#ifdef _DEBUG
								std::cout << "found and added all possible capture options, continuing" << std::endl;
#endif
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*next piece is not empty and braking" << std::endl;
#endif
								possible_capture_top_left.push_back(false);
								break;
							}
						}
						else // empty field
						{
							if ((*board)[x - i - 1][y - i - 1] != NULL)
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty and next field is not empty" << std::endl;
#endif
								if (at_least_one_capture_top_right)
								{
#ifdef _DEBUG
									std::cout << "*no more places to check, it is a singular check" << std::endl;
#endif
									break;
								}
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty" << std::endl;
#endif
							}
							local_moves_top_left.push_back(new available_move(x - i, y - i));
							possible_capture_top_left.push_back(false);
						}
						++i;
					}
					if (x - i >= 0 && y - i >= 0 && (*board)[x - i][y - i] == NULL && !at_least_one_capture_top_left) // last, not checked field (checking looks for next which is outside the boundaries)
					{
#ifdef _DEBUG
						std::cout << "last but not least" << std::endl;
#endif
						local_moves_top_left.push_back(new available_move(x - i, y - i));
					}


					// capture bottom right (2) + +
#ifdef _DEBUG
					std::cout << "bottom right checking" << std::endl;
#endif
					i = 1;
					while (x + i + 1 <= s_size - 1 && y + i + 1 <= s_size - 1)
					{
#ifdef _DEBUG
						std::cout << "checking: x: " << x + i << ", y: " << y + i << " and place to go: x: " << x + i + 1 << ", y: " << y + i + 1 << std::endl;
#endif
						// searching for own piece (cannot jump across them)
						if ((*board)[x + i][y + i] != NULL && (*board)[x + i][y + i]->get_sign() == player->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found own piece and breaking" << std::endl;
#endif
							possible_capture_bottom_right.push_back(false);
							break;
						}
						else if ((*board)[x + i][y + i] != NULL && (*board)[x + i][y + i]->get_sign() == player->get_next_player()->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
							if ((*board)[x + i + 1][y + i + 1] == NULL)
							{
#ifdef _DEBUG
								std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
								possible_capture_bottom_right.push_back(true);
								at_least_one_capture_bottom_right = true;

								// adding all possible capture options
								int j = 0;
								while (x + i + 1 + j <= s_size - 1 && y + i + 1 + j <= s_size - 1 && (*board)[x + i + 1 + j][y + i + 1 + j] == NULL)
								{
#ifdef _DEBUG
									std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x + i + 1 + j << ", y: " << y + i + 1 + j << std::endl;
#endif
									local_captures_bottom_right.push_back(new available_capture(x + i + 1 + j, y + i + 1 + j, x + i, y + i));
									++j;
								}
#ifdef _DEBUG
								std::cout << "found and added all possible capture options, continuing" << std::endl;
#endif
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*next piece is not empty and braking" << std::endl;
#endif
								possible_capture_bottom_right.push_back(false);
								break;
							}
						}
						else // empty field
						{
							if ((*board)[x + i + 1][y + i + 1] != NULL)
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty and next field is not empty" << std::endl;
#endif
								if (at_least_one_capture_top_right)
								{
#ifdef _DEBUG
									std::cout << "*no more places to check, it is a singular check" << std::endl;
#endif
									break;
								}
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty" << std::endl;
#endif
							}
							local_moves_bottom_right.push_back(new available_move(x + i, y + i));
							possible_capture_bottom_right.push_back(false);
						}

						++i;
					}
					if (x + i <= s_size - 1 && y + i <= s_size - 1 && (*board)[x + i][y + i] == NULL && !at_least_one_capture_bottom_right) // last, not checked field (checking looks for next which is outside the boundaries)
					{
#ifdef _DEBUG
						std::cout << "last but not least" << std::endl;
#endif
						local_moves_bottom_right.push_back(new available_move(x + i, y + i));
					}

					// capture bottom left (3) - +
#ifdef _DEBUG
					std::cout << "bottom left checking" << std::endl;
#endif
					i = 1;
					while (x - i - 1 >= 0 && y + i + 1 <= s_size - 1)
					{
#ifdef _DEBUG
						std::cout << "checking: x: " << x - i << ", y: " << y + i << " and place to go: x: " << x - i - 1 << ", y: " << y + i + 1 << std::endl;
#endif
						// searching for own piece (cannot jump across them)
						if ((*board)[x - i][y + i] != NULL && (*board)[x - i][y + i]->get_sign() == player->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found own piece and breaking" << std::endl;
#endif
							possible_capture_bottom_left.push_back(false);
							break;
						}
						else if ((*board)[x - i][y + i] != NULL && (*board)[x - i][y + i]->get_sign() == player->get_next_player()->get_sign())
						{
#ifdef _DEBUG
							std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
							if ((*board)[x - i - 1][y + i + 1] == NULL)
							{
#ifdef _DEBUG
								std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
								possible_capture_bottom_left.push_back(true);
								at_least_one_capture_bottom_left = true;

								// adding all possible capture options
								int j = 0;
								while (x - i - 1 - j >= 0 && y + i + 1 + j <= s_size - 1 && (*board)[x - i - 1 - j][y + i + 1 + j] == NULL)
								{
#ifdef _DEBUG
									std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x - i - 1 - j << ", y: " << y + i + 1 + j << std::endl;
#endif
									local_captures_bottom_left.push_back(new available_capture(x - i - 1 - j, y + i + 1 + j, x - i, y + i));
									++j;
								}
#ifdef _DEBUG
								std::cout << "found and added all possible capture options, continuing" << std::endl;
#endif
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*next piece is not empty and braking" << std::endl;
#endif
								possible_capture_bottom_left.push_back(false);
								break;
							}
						}
						else // empty field
						{
							if ((*board)[x - i - 1][y + i + 1] != NULL)
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty and next field is not empty" << std::endl;
#endif
								if (at_least_one_capture_top_right)
								{
#ifdef _DEBUG
									std::cout << "*no more places to check, it is a singular check" << std::endl;
#endif
									break;
								}
							}
							else
							{
#ifdef _DEBUG
								std::cout << "*there is no capture, checked field is empty" << std::endl;
#endif
							}
							local_moves_bottom_left.push_back(new available_move(x - i, y + i));
							possible_capture_bottom_left.push_back(false);
						}

						++i;
					}
					if (x - i >= 0 && y + i <= s_size - 1 && (*board)[x - i][y + i] == NULL && !at_least_one_capture_bottom_left) // last, not checked field (checking looks for next which is outside the boundaries)
					{
#ifdef _DEBUG
						std::cout << "Added last possible field" << std::endl;
#endif
						local_moves_bottom_left.push_back(new available_move(x - i, y + i));
					}
#ifdef _DEBUG
					// list top right moves and captures
					std::cout << "king top right moves" << std::endl;
					for_each(possible_capture_top_right.begin(), possible_capture_top_right.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

					std::cout << "local moves" << std::endl;
					for_each(local_moves_top_right.begin(), local_moves_top_right.end(), [](available_move* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << std::endl; });

					std::cout << "local captures" << std::endl;
					for_each(local_captures_top_right.begin(), local_captures_top_right.end(), [](available_capture* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << "; x_d: " << a->get_x_d() << "; y_d: " << a->get_y_d() << std::endl; });
					
					// list top left moves and captures
					std::cout << "king top left moves" << std::endl;
					for_each(possible_capture_top_left.begin(), possible_capture_top_left.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

					std::cout << "local moves" << std::endl;
					for_each(local_moves_top_left.begin(), local_moves_top_left.end(), [](available_move* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << std::endl; });

					std::cout << "local captures" << std::endl;
					for_each(local_captures_top_left.begin(), local_captures_top_left.end(), [](available_capture* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << "; x_d: " << a->get_x_d() << "; y_d: " << a->get_y_d() << std::endl; });

					// list bottom right moves and captures
					std::cout << "king bottom right moves" << std::endl;
					for_each(possible_capture_bottom_right.begin(), possible_capture_bottom_right.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

					std::cout << "local moves" << std::endl;
					for_each(local_moves_bottom_right.begin(), local_moves_bottom_right.end(), [](available_move* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << std::endl; });

					std::cout << "local captures" << std::endl;
					for_each(local_captures_bottom_right.begin(), local_captures_bottom_right.end(), [](available_capture* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << "; x_d: " << a->get_x_d() << "; y_d: " << a->get_y_d() << std::endl; });

					// list bottom left moves and captures
					std::cout << "king bottom left moves" << std::endl;
					for_each(possible_capture_bottom_left.begin(), possible_capture_bottom_left.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

					std::cout << "local moves" << std::endl;
					for_each(local_moves_bottom_left.begin(), local_moves_bottom_left.end(), [](available_move* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << std::endl; });

					std::cout << "local captures" << std::endl;
					for_each(local_captures_bottom_left.begin(), local_captures_bottom_left.end(), [](available_capture* a) { std::cout << "-x: " << a->get_x() << "; y: " << a->get_y() << "; x_d: " << a->get_x_d() << "; y_d: " << a->get_y_d() << std::endl; });
#endif
					if (at_least_one_capture_top_right || at_least_one_capture_top_left || at_least_one_capture_bottom_right || at_least_one_capture_bottom_left)
					{
						av_capture = true;

						int top_right = local_captures_top_right.size();
						int top_left = local_captures_top_left.size();
						int bottom_right = local_captures_bottom_right.size();
						int bottom_left = local_captures_bottom_left.size();
						
						// for storing recursively evaluated capture counters
						std::vector<int> capture_counter[4] = { std::vector<int>(top_right), std::vector<int>(top_left), std::vector<int>(bottom_right), std::vector<int>(bottom_left) }; // 0 - top right, 1 - top left, 2 - bottom right, 3 - bottom left

						// top right recursive evaluation
						if (at_least_one_capture_top_right)
						{
							for_each(local_captures_top_right.begin(), local_captures_top_right.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y](available_capture* a) mutable
								{
									//(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
									capture_counter[0][i] = 1; // change here to get from counter, then increment?

									// copy coords
									int x_to_go = a->get_x();
									int y_to_go = a->get_y();
									int x_to_delete = a->get_x_d();
									int y_to_delete = a->get_y_d();

									// copy the board and make empty list for moved piece
									std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
									std::list<piece*> copy_of_list;
									if (*counter == NULL) // first call of evaluation
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*m_board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

									}
									else // the function call is being recursive 
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
									}

									// make planned move
									piece* moving_piece = (*copy_of_board)[x][y];
									(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
									moving_piece->set_x(x_to_go);
									moving_piece->set_y(y_to_go);
									(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
									copy_of_list.push_back(moving_piece);
									(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
										evaluate(&copy_of_list, copy_of_board, &moves, player);
										capture_counter[0][i] = moves;

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
										evaluate(&copy_of_list, copy_of_board, counter, player);
									}
									++i;
								});
#ifdef _DEBUG
							// print
							std::cout << "for every capture option, these are multi capture counters" << std::endl;
							for_each(capture_counter[0].begin(), capture_counter[0].end(), [i = 1](int c) mutable { std::cout << i++ << ": " << c << std::endl; });
#endif
						}

						// top left recursive evaluation
						if (at_least_one_capture_top_left)
						{
							for_each(local_captures_top_left.begin(), local_captures_top_left.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y](available_capture* a) mutable
								{
									//(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
									capture_counter[1][i] = 1; // change here to get from counter, then increment?

									// copy coords
									int x_to_go = a->get_x();
									int y_to_go = a->get_y();
									int x_to_delete = a->get_x_d();
									int y_to_delete = a->get_y_d();

									// copy the board and make empty list for moved piece
									std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
									std::list<piece*> copy_of_list;
									if (*counter == NULL) // first call of evaluation
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*m_board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

									}
									else // the function call is being recursive 
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
									}

									// make planned move
									piece* moving_piece = (*copy_of_board)[x][y];
									(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
									moving_piece->set_x(x_to_go);
									moving_piece->set_y(y_to_go);
									(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
									copy_of_list.push_back(moving_piece);
									(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
										evaluate(&copy_of_list, copy_of_board, &moves, player);
										capture_counter[1][i] = moves;

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
										evaluate(&copy_of_list, copy_of_board, counter, player);
									}
									++i;
								});

#ifdef _DEBUG
							// print
							std::cout << "for every capture option, these are multi capture counters" << std::endl;
							for_each(capture_counter[1].begin(), capture_counter[1].end(), [i = 1](int c) mutable { std::cout << i++ << ": " << c << std::endl; });
#endif
						}

						// bottom right recursive evaluation
						if (at_least_one_capture_bottom_right)
						{
							for_each(local_captures_bottom_right.begin(), local_captures_bottom_right.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y](available_capture* a) mutable
								{
									//(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
									capture_counter[2][i] = 1; // change here to get from counter, then increment?

									// copy coords
									int x_to_go = a->get_x();
									int y_to_go = a->get_y();
									int x_to_delete = a->get_x_d();
									int y_to_delete = a->get_y_d();

									// copy the board and make empty list for moved piece
									std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
									std::list<piece*> copy_of_list;
									if (*counter == NULL) // first call of evaluation
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*m_board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

									}
									else // the function call is being recursive 
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
									}

									// make planned move
									piece* moving_piece = (*copy_of_board)[x][y];
									(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
									moving_piece->set_x(x_to_go);
									moving_piece->set_y(y_to_go);
									(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
									copy_of_list.push_back(moving_piece);
									(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
										evaluate(&copy_of_list, copy_of_board, &moves, player);
										capture_counter[2][i] = moves;

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
										evaluate(&copy_of_list, copy_of_board, counter, player);
									}
									++i;
								});
#ifdef _DEBUG
							// print
							std::cout << "for every capture option, these are multi capture counters" << std::endl;
							for_each(capture_counter[2].begin(), capture_counter[2].end(), [i = 1](int c) mutable { std::cout << i++ << ": " << c << std::endl; });
#endif
						}

						// bottom left recursive evaluation
						if (at_least_one_capture_bottom_left)
						{
							for_each(local_captures_bottom_left.begin(), local_captures_bottom_left.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y](available_capture* a) mutable
								{
									//(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
									capture_counter[3][i] = 1; // change here to get from counter, then increment?

									// copy coords
									int x_to_go = a->get_x();
									int y_to_go = a->get_y();
									int x_to_delete = a->get_x_d();
									int y_to_delete = a->get_y_d();

									// copy the board and make empty list for moved piece
									std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
									std::list<piece*> copy_of_list;
									if (*counter == NULL) // first call of evaluation
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*m_board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*m_board)[i][j]->get_sign(), (*m_board)[i][j]->get_x(), (*m_board)[i][j]->get_y());

									}
									else // the function call is being recursive 
									{
										for (int i = 0; i < s_size; ++i)
											for (int j = 0; j < s_size; ++j)
												if ((*board)[i][j] != NULL)
													(*copy_of_board)[i][j] = new piece((*board)[i][j]->get_sign(), (*board)[i][j]->get_x(), (*board)[i][j]->get_y());
									}

									// make planned move
									piece* moving_piece = (*copy_of_board)[x][y];
									(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
									moving_piece->set_x(x_to_go);
									moving_piece->set_y(y_to_go);
									(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
									copy_of_list.push_back(moving_piece);
									(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
										evaluate(&copy_of_list, copy_of_board, &moves, player);
										capture_counter[3][i] = moves;

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
										evaluate(&copy_of_list, copy_of_board, counter, player);
									}
									++i;
								});
#ifdef _DEBUG
							// print
							std::cout << "for every capture option, these are multi capture counters" << std::endl;
							for_each(capture_counter[3].begin(), capture_counter[3].end(), [i = 1](int c) mutable { std::cout << i++ << ": " << c << std::endl; });
#endif
						}

						// find maximal capture counter
						int max_captures = 0;
						for (int i = 0; i < 4; ++i)
							for_each(capture_counter[i].begin(), capture_counter[i].end(), [&max_captures](int c) { if (c > max_captures) max_captures = c; });
#ifdef _DEBUG
						std::cout << "found max captures: " << max_captures << std::endl;
#endif
						for (int i = 0; i < 4; ++i)
							for_each(capture_counter[i].begin(), capture_counter[i].end(), [&max_captures, &i, &p, j = 0, &local_captures_top_right, &local_captures_top_left, &local_captures_bottom_right, &local_captures_bottom_left](int c) mutable
								{
									if (c == max_captures)
									{
										switch (i)
										{
										case 0:
											p->get_av_list()->push_back(new available_capture(local_captures_top_right[j]->get_x(), local_captures_top_right[j]->get_y(), local_captures_top_right[j]->get_x_d(), local_captures_top_right[j]->get_y_d()));
											break;
										case 1:
											p->get_av_list()->push_back(new available_capture(local_captures_top_left[j]->get_x(), local_captures_top_left[j]->get_y(), local_captures_top_left[j]->get_x_d(), local_captures_top_left[j]->get_y_d()));
											break;
										case 2:
											p->get_av_list()->push_back(new available_capture(local_captures_bottom_right[j]->get_x(), local_captures_bottom_right[j]->get_y(), local_captures_bottom_right[j]->get_x_d(), local_captures_bottom_right[j]->get_y_d()));
											break;
										case 3:
											p->get_av_list()->push_back(new available_capture(local_captures_bottom_left[j]->get_x(), local_captures_bottom_left[j]->get_y(), local_captures_bottom_left[j]->get_x_d(), local_captures_bottom_left[j]->get_y_d()));
											break;
										}
									}
									++j;
								});
					}
					else // only moves
					{
						for_each(local_moves_top_right.begin(), local_moves_top_right.end(), [&p](available_move* a)
							{
								p->get_av_list()->push_back(new available_move(a->get_x(), a->get_y()));
							});
						
						for_each(local_moves_top_left.begin(), local_moves_top_left.end(), [&p](available_move* a)
							{
								p->get_av_list()->push_back(new available_move(a->get_x(), a->get_y()));
							});

						for_each(local_moves_bottom_right.begin(), local_moves_bottom_right.end(), [&p](available_move* a)
							{
								p->get_av_list()->push_back(new available_move(a->get_x(), a->get_y()));
							});

						for_each(local_moves_bottom_left.begin(), local_moves_bottom_left.end(), [&p](available_move* a)
							{
								p->get_av_list()->push_back(new available_move(a->get_x(), a->get_y()));
							});
					}
					

					// evaluate
					
					// check if is there at least one capture in each direction (flags)

					// evaluate in each direction to reveal highest score
					
					// if there is available capture, add all available capture moves to the max score

					// else add all normal moves

				}
				
				

			});
#ifdef _DEBUG
		std::cout << "Evaluation returns: ";
		av_capture ? (std::cout << "true") : (std::cout << "false");
		std::cout << std::endl;
#endif
		return av_capture;
	}

	void game::clear_list(std::list<piece*>* list) { for_each(list->begin(), list->end(), [this](piece* p) { p->get_av_list()->clear(); }); }

	void game::print_pieces(std::list<piece*>* list, std::ostream& os)
	{
		std::for_each(list->begin(), list->end(), [i = 1, this, &os](piece* p) mutable
			{
				os << i++ << "; sign: " << p << "; x: " << p->get_x() << "; y: " << p->get_y();
				if (p->is_king())
					os << "; king";
				os << std::endl;
			});
	}

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