#include "include/game.h"
#include "include/king.h"

// todo:
// remake code into more functions, current player and opponent pointers
// add menu
// optimise drawing
// add animation
// add setting rows and check if set correctly
// switch to polymorphism completely (get rid of is_king flag)
// consider current moving piece to eliminate situation where two pieces have possible captures
// move some methods into private
// save highest evaluation of capture in current move
// add seters with value check
// debug 4 rows and specific debug situation with 3 white pieces and one king

namespace checkers
{
	game::game(int fps, std::istream& is, std::ostream& os) : m_console_game(false), m_is_finished(false), m_fps(fps),
		m_window(sf::VideoMode(s_square_size* s_size, s_square_size* s_size), "Checkers", sf::Style::Default, m_settings),
		m_selected(false), m_selected_piece(NULL), m_moving_piece(NULL), m_available_capture(false), m_last_capture_direction(-1), m_is(is), m_os(os),
		m_tiles(s_size, std::vector<sf::RectangleShape>(s_size, sf::RectangleShape())), m_clock(), m_event(), m_settings(), m_current_player(NULL)
	{
		assert(s_size % 2 == 0);

		//// create a font object
		//sf::Font font;
		//if (!font.loadFromFile("arial.ttf"))
		//	throw std::runtime_error("Font loading failure");
		//
		//// welcome text
		//sf::Text text;
		//text.setFont(font);
		//text.setCharacterSize(24);
		//text.setFillColor(sf::Color::White);
		//text.setPosition(50, 50);
		//text.setString("Checkers");

		//m_window.clear();
		//m_window.draw(text);
		//m_window.display();

		//// simplified: in-console choice
		//int choice = 0;
		//std::cout << "0 - player vs player" << std::endl;
		//std::cout << "any - player vs bot" << std::endl;
		//std::cout << "choose: ";
		//std::cin >> choice;

		//auto get_coords = std::bind(&game::get_click_coordinates, this);

		//if (choice == 0)
		//{
		//	m_player_1 = new player('W', "Player1", get_coords);
		//	m_player_2 = new player('B', "Player2", get_coords);
		//}
		//else
		//{
		//	m_player_1 = new player('W', "Player1", get_coords);
		//	m_player_2 = new bot('B', this);
		//}

		// simplified: set values
		if (!m_console_game)
		{
			auto get_coords = std::bind(&game::get_click_coordinates, this);
			m_player_1 = new player('W', "Player1", get_coords);
			m_player_2 = new player('B', "Player2", get_coords);
		}
		else
		{
			auto get_coords = std::bind(&game::get_coordinates_from_stream, this);
			m_player_1 = new player('W', "Player1", get_coords);
			m_player_2 = new player('B', "Player2", get_coords);
		}
		//m_player_2 = new bot('B', this);

		// set play order and evaluation direction
		m_player_1->set_first(true);
		m_player_2->set_first(false);
		m_player_1->set_next_player(m_player_2);
		m_player_2->set_next_player(m_player_1);
		m_current_player = m_player_1;

		// board init
		m_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));

		// set pointers to piece lists
		m_player_1->set_list(&m_p_list_1);
		m_player_2->set_list(&m_p_list_2);

		// fill the board with pieces
#ifdef _DEBUG
		//populate_board(4);
		populate_board_debug();
#else
		populate_board(s_size / 2 - 1);
		//populate_board_debug();
#endif

		// SFML setup, using original methods in camelCase style instead of snake_case
		m_settings.antialiasingLevel = 8;
		m_window.setFramerateLimit(fps);
		m_window.setVerticalSyncEnabled(true);

		for_each(m_tiles.begin(), m_tiles.end(), [i = 0](std::vector<sf::RectangleShape>& row) mutable
			{
				for_each(row.begin(), row.end(), [&i, j = 0](sf::RectangleShape& tile) mutable
					{
						tile.setSize(sf::Vector2f(s_square_size, s_square_size));
						tile.setPosition(sf::Vector2f(s_square_size * i, s_square_size * j));
						if ((i + j) % 2 == 0)
							tile.setFillColor(sf::Color(193, 173, 158, 255));
						else
							tile.setFillColor(sf::Color(133, 94, 66, 255));
						++j;
					});
				++i;
			});

		//m_tile.setSize(sf::Vector2f(s_square_size, s_square_size));

		/*for (int i = 0; i < s_size; ++i)
		{
			for (int j = 0; j < s_size; ++j)
			{
				m_tile.setPosition(sf::Vector2f(s_square_size * i, s_square_size * j));
				if ((i + j) % 2 == 0)
					m_tile.setFillColor(sf::Color(193, 173, 158, 255));
				else
					m_tile.setFillColor(sf::Color(133, 94, 66, 255));
				window.draw(m_tile);
			}
		}*/


		// evaluate available moves for the first player
		int dummy = 0;
		m_available_capture = evaluate(m_player_1->get_list(), m_board, &dummy, m_player_1, m_last_capture_direction, &m_to_delete_list, NULL);
#ifdef _DEBUG
		std::cout << "Game is evaluated" << std::endl;

		std::cout << "List of pieces of first player" << std::endl;
		print_pieces(m_player_1->get_list());

		std::cout << "List of pieces of second player" << std::endl;
		print_pieces(m_player_2->get_list());
#endif
		m_window.clear();
		draw(m_window);
	}

	game::game(const game& game) : m_fps(game.m_fps)
	{
		// copy player 1
		if (dynamic_cast<player*>(game.m_player_1))
			m_player_1 = new player(*dynamic_cast<player*>(game.m_player_1));
		else if (dynamic_cast<bot*>(game.m_player_1))
			m_player_1 = new bot(*dynamic_cast<bot*>(game.m_player_1));
		else
			m_player_1 = NULL;

		// copy player 2
		if (dynamic_cast<player*>(game.m_player_2))
			m_player_2 = new player(*dynamic_cast<player*>(game.m_player_2));
		else if (dynamic_cast<bot*>(game.m_player_2))
			m_player_2 = new bot(*dynamic_cast<bot*>(game.m_player_2));
		else
			m_player_2 = NULL;

		assert(m_player_1 != NULL);
		assert(m_player_2 != NULL);

		// establish current player
		m_current_player = game.m_current_player == game.m_player_1 ? m_player_1 : m_player_2;

		// set the next player fields
		m_player_1->set_next_player(m_player_2);
		m_player_2->set_next_player(m_player_1);

		// copy the board and recreate the lists
		m_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, NULL));
		std::vector<std::vector<piece*>>* original_board = game.m_board;

		for (int i = 0; i < s_size; ++i)
			for (int j = 0; j < s_size; ++j)
			{
				if ((*original_board)[i][j] != NULL)
				{
					piece* p = (*original_board)[i][j];
					if (dynamic_cast<king*>(p))
					{
						if (p->get_owner() == game.m_player_1)
							add_new_piece(&m_p_list_1, m_board, m_player_1, p);
						else if (p->get_owner() == game.m_player_2)
							add_new_piece(&m_p_list_2, m_board, m_player_2, p);
						else
							throw std::runtime_error("Copying the board: king piece: player signs not matching");
					}
					else
					{
						if (p->get_owner() == game.m_player_1)
							add_new_piece(&m_p_list_1, m_board, m_player_1, p);
						else if (p->get_owner() == game.m_player_2)
							add_new_piece(&m_p_list_2, m_board, m_player_2, p);
						else
							throw std::runtime_error("Copying the board: normal piece: player signs not matching");
					}

				}
			}
		m_player_1->set_list(&m_p_list_1);
		m_player_2->set_list(&m_p_list_2);
#ifdef _DEBUG
		std::cout << "Board after copying: " << std::endl;
		std::cout << m_board << std::endl;

		std::cout << "List of pieces of first player" << std::endl;
		print_pieces(&m_p_list_1);

		std::cout << "List of pieces of second player" << std::endl;
		print_pieces(&m_p_list_2);
#endif

		// recreate to delete list
		for_each(game.m_to_delete_list.begin(), game.m_to_delete_list.end(), [this](piece* p)
			{
				if (dynamic_cast<king*>(p))
					m_to_delete_list.push_back(new king(*dynamic_cast<king*>(p)));
				else
					m_to_delete_list.push_back(new piece(*p));
			});

		// recreate moving piece
		if (game.m_moving_piece == NULL)
			m_moving_piece = NULL;
		else
			m_moving_piece = (*m_board)[game.m_moving_piece->get_x()][game.m_moving_piece->get_y()];

		// check if the source game was normal
		assert(m_to_delete_list.empty() && m_moving_piece == NULL || !m_to_delete_list.empty() && m_moving_piece != NULL);

		// other fields
		m_last_capture_direction = game.m_last_capture_direction;
		m_console_game = game.m_console_game;
		m_is_finished = game.m_is_finished;
		m_first_won = game.m_first_won;
		m_second_won = game.m_second_won;
		m_frame_duration = game.m_frame_duration;
		m_selected_piece = NULL;

		// evaluate available moves for the current player
		int dummy = 0;
		m_available_capture = evaluate(m_current_player->get_list(), m_board, &dummy, m_current_player, m_last_capture_direction, &m_to_delete_list, m_moving_piece);
#ifdef _DEBUG
		std::cout << "Game copy evaluated" << std::endl;
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
#endif 
	}

	void game::populate_board(int rows)
	{
		assert(m_board->size() == s_size);
		assert(rows <= s_size / 2);

		// todo: change to algorithm
		// rows of the second player (upper)
		for (int i = 0; i < rows; ++i)
			for (int j = 0; j < s_size; ++j)
				if ((i + j) % 2 != 0)
					add_new_piece(&m_p_list_2, m_board, m_player_2, j, i, true);

		// rows of the first player (lower)
		for (int i = s_size - 1; i >= s_size - rows; --i)
			for (int j = 0; j < s_size; ++j)
				if ((i + j) % 2 != 0)
					add_new_piece(&m_p_list_1, m_board, m_player_1, j, i, true);
	}

	void game::populate_board_debug(void)
	{
		assert(m_board->size() == s_size);

		// rows of the second player (upper)
		//add_new_piece(m_player_2->get_list(), m_board, m_player_2, 1, 8, true);

		// rows of the first player (lower)
		//add_new_piece(m_player_1->get_list(), m_board, m_player_1, 5, 4, true);
	}

	void game::add_new_piece(std::list<piece*>* list, std::vector<std::vector<piece*>>* board, base_player* player, int x, int y, bool is_alive)
	{
		assert((*board)[x][y] == NULL);
		piece* p = new piece(player->get_sign(), x, y, is_alive, player);
		(*board)[x][y] = p;
		list->push_back(p);
		player->add_piece();
		bool king = player->change_to_king(p, board);
#ifdef _DEBUG
		if (king)
			m_os << "New piece is a king!" << std::endl;
		else
			m_os << "Added new piece!" << std::endl;
#endif
	}

	void game::add_new_piece(std::list<piece*>* list, std::vector<std::vector<piece*>>* board, base_player* player, piece* based_on)
	{
		int x = based_on->get_x();
		int y = based_on->get_y();
		assert((*board)[x][y] == NULL);
		if (dynamic_cast<king*>(based_on))
			(*board)[x][y] = new king(player->get_sign(), x, y, based_on->is_alive(), player);
		else
			(*board)[x][y] = new piece(player->get_sign(), x, y, based_on->is_alive(), player);
		list->push_back((*board)[x][y]);
		player->add_piece();
	}

	std::vector<std::vector<piece*>>* game::get_board(void) { assert(m_board != NULL); return m_board; }

	std::tuple<int, int> game::get_coordinates(void) { return m_current_player->get_coordinates(); }

	std::tuple<int, int> game::get_click_coordinates(void)
	{
		int x = sf::Mouse::getPosition(m_window).x / (m_window.getSize().x / s_size);
		int y = sf::Mouse::getPosition(m_window).y / (m_window.getSize().y / s_size);

		return std::make_tuple(x, y);
	}

	std::tuple<int, int> game::get_coordinates_from_stream(void)
	{
		// change to string, add checks if it is a number etc.
		int x = 0;
		int y = 0;

		while (x < 1 || x > 10)
		{
			m_os << "Give the x coordinate: ";
			m_is >> x;
		}
		while (y < 1 || y > 10)
		{
			m_os << "Give the y coordinate: ";
			m_is >> y;
		}
		--x;
		--y;

		return std::make_tuple(x, y);
	}

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
		for_each(m_tiles.begin(), m_tiles.end(), [&window](auto row)
			{
				for_each(row.begin(), row.end(), [&window](auto tile)
					{
						window.draw(tile);
					});
			});
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

	void game::handle_events(void)
	{

	}

	void game::draw_board(void)
	{

	}

	void game::select_piece(void)
	{

	}

	void game::move_piece(piece* piece_to_move, std::vector<std::vector<piece*>>* board, int x, int y)
	{
		assert((*board)[x][y] == NULL);
		assert((*board)[piece_to_move->get_x()][piece_to_move->get_y()] == piece_to_move);

		(*m_board)[x][y] = NULL;
		piece_to_move->set_x(x);
		piece_to_move->set_y(y);
	}

	void game::delete_piece(piece* piece_to_delete, std::vector<std::vector<piece*>>* board, base_player* owner)
	{
		int x = piece_to_delete->get_x();
		int y = piece_to_delete->get_y();

		assert((*board)[x][y] == piece_to_delete);
		assert(piece_to_delete->get_owner() == owner);

		(*board)[x][y] = NULL;

		m_current_player->get_next_player()->make_capture();
		delete_from_list(owner->get_list(), piece_to_delete);
	}

	void game::check_game_completion(void)
	{

	}

	void game::test_loop(void)
	{
		// main loop
		while (m_window.isOpen())
		{
			while (m_window.pollEvent(m_event))
			{
				if (m_event.type == sf::Event::Closed)
					m_window.close();
			}
			m_window.display();
			m_window.clear();
			draw(m_window);
			sf::sleep(sf::seconds(m_frame_duration));
		}
	}

	void game::loop(void)
	{
		// start frame clock
		m_clock.restart();

		// main loop
		while (m_window.isOpen())
		{
			while (m_window.pollEvent(m_event))
			{
				if (m_event.type == sf::Event::Closed || m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::Escape)
				{
					m_window.close();
#ifdef _DEBUG
					m_os << "Closing the game" << std::endl;
					//save_pieces_to_file();
#endif
				}

				if (m_event.type == sf::Event::MouseButtonPressed && m_event.mouseButton.button == sf::Mouse::Left || dynamic_cast<bot*>(m_current_player) || m_console_game)
				{
					if (m_player_1->get_pieces() == 0 || m_player_2->get_pieces() == 0)
					{
						m_os << "Game is finished. Click Escape to display the results" << std::endl;
					}

					if (m_selected_piece != NULL) // choice after highlighting
					{
						// getting coords of the click after highlighting selected piece, ignore clicks outside
						std::tuple<int, int> coordinates = get_coordinates();
						int x = std::get<0>(coordinates);
						int y = std::get<1>(coordinates);

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
							m_selected = false;
							m_selected_piece = NULL;
						}
						else // making a move
						{
							if (dynamic_cast<available_capture*>(found_move))
							{
								// mark found capture
								available_capture* found_capture = dynamic_cast<available_capture*>(found_move);
								int x_d = found_capture->get_x_d();
								int y_d = found_capture->get_y_d();

								// save capture direction: 0 - top right, 1 - top left, 2 - bottom right, 3 - bottom left
								if (x_d > x && y_d < y)
									m_last_capture_direction = 3;
								else if (x_d < x && y_d < y)
									m_last_capture_direction = 2;
								else if (x_d > x && y_d > y)
									m_last_capture_direction = 1;
								else if (x_d < x && y_d > y)
									m_last_capture_direction = 0;
								else
									throw std::runtime_error("Capturing in wrong direction");
#ifdef _DEBUG
								m_os << "/ saved last capture direction: ";
								switch (m_last_capture_direction)
								{
								case 0:
									m_os << "top right";
									break;
								case 1:
									m_os << "top left";
									break;
								case 2:
									m_os << "bottom right";
									break;
								case 3:
									m_os << "bottom left";
									break;
								}
								m_os << std::endl;

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

								// if there is no multicapture, set new moving piece
								if (m_to_delete_list.empty())
									m_moving_piece = m_selected_piece;

								// create new piece which represents dead piece during multicapture, it is indifferent whether it was normal piece or king
								m_to_delete_list.push_back(new piece(m_current_player->get_next_player()->get_sign(), x_d, y_d, false, m_current_player->get_next_player()));
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
							m_selected = false;

#ifdef _DEBUG
							std::cout << "List of pieces of first player" << std::endl;
							print_pieces(&m_p_list_1);
							std::cout << "List of pieces of second player" << std::endl;
							print_pieces(&m_p_list_2);
#endif		

							// tmp flag indicating, that the king check was made this round
							bool made_king_check = false;
							bool changed_to_king = false;

							// switch turn, if no combo
							if (!m_player_1->get_combo() && !m_player_2->get_combo())
							{
								// king function
								if (!made_king_check)
									changed_to_king = m_current_player->change_to_king(m_selected_piece, m_board);
								made_king_check = true;
#ifdef _DEBUG
								if (changed_to_king)
									m_os << m_current_player->get_name() << " changed his piece to king!" << std::endl;
#endif
								switch_turn();
							}
							else // section to test (fixes stuff)
							{
								clear_list(&m_p_list_1);
								clear_list(&m_p_list_2);
							}

							// evaluate current player and check if there is more captures, if not, check for new kings
							int dummy = 0;
							m_available_capture = evaluate(m_current_player->get_list(), m_board, &dummy, m_current_player, m_last_capture_direction, &m_to_delete_list, m_moving_piece);
#ifdef _DEBUG
							m_os << "dummy first: " << dummy << std::endl;
#endif
							// exit the combo, if no more captures
							if (m_current_player->get_combo() && !m_available_capture)
							{
								m_moving_piece = NULL;

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
									changed_to_king = m_current_player->change_to_king(m_selected_piece, m_board);
								made_king_check = true;
#ifdef _DEBUG
								if (changed_to_king)
									m_os << m_current_player->get_name() << " changed his piece to king!" << std::endl;
#endif
								switch_turn();
								m_available_capture = evaluate(m_current_player->get_list(), m_board, &dummy, m_current_player, m_last_capture_direction, &m_to_delete_list, m_moving_piece);
#ifdef _DEBUG
								m_os << "dummy second: " << dummy << std::endl;
#endif
							}
							else // continue the combo
								clear_list(m_current_player->get_next_player()->get_list());

							m_selected_piece = NULL;

							// check for empty evaluation?
						}
					}
					else
						m_selected = !m_selected;
				}
#ifdef _DEBUG
				if (m_event.type == sf::Event::MouseButtonPressed && m_event.mouseButton.button == sf::Mouse::Right)
				{
					// display debug info
					debug_info();
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::E)
				{
					// re-evaluate the game
					m_selected_piece = NULL;
					m_selected = false;
					int dummy = 0;
					m_available_capture = evaluate(m_current_player->get_list(), m_board, &dummy, m_current_player, m_last_capture_direction, &m_to_delete_list, m_moving_piece);
					m_os << "Game re-evaluated" << std::endl;
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::T)
				{
					// write a separator
					m_os << "--------------------------------------------------------------" << std::endl;
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::U && (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)))
				{
					// add new piece to the first player
					std::tuple<int, int> coords = get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_board)[x][y] == NULL)
						add_new_piece(&m_p_list_1, m_board, m_player_1, new king(m_player_1->get_sign(), x, y, true, m_player_1));
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::I && (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)))
				{
					std::cout << "here" << std::endl;

					// add new piece to the second player
					std::tuple<int, int> coords = get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_board)[x][y] == NULL)
						add_new_piece(&m_p_list_2, m_board, m_player_2, new king(m_player_2->get_sign(), x, y, true, m_player_2));
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::U)
				{
					// add new piece to the first player
					std::tuple<int, int> coords = get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_board)[x][y] == NULL)
						add_new_piece(&m_p_list_1, m_board, m_player_1, x, y, true);
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::I)
				{
					// add new piece to the second player
					std::tuple<int, int> coords = get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_board)[x][y] == NULL)
						add_new_piece(&m_p_list_2, m_board, m_player_2, x, y, true);
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::L)
				{
					// load pieces from file
					// tmp: load normal board
					populate_board(s_size / 2 - 1);
				}
				else if (m_event.type == sf::Event::KeyPressed && m_event.key.code == sf::Keyboard::S)
				{
					switch_turn();
				}
				else if (m_event.type == sf::Event::KeyPressed)
				{
					m_os << "Key code: \'" << m_event.key.code << "\' is not programmed." << std::endl;
				}
#endif
			}

			//// end of the game
			//if (m_first_won || m_second_won)
			//{
			//	//std::cout << "Game is finished" << std::endl;
			//	//break;
			//	m_selected = false;
			//	m_selected_piece = NULL;
			//}

			//m_window.clear();
			draw(m_window);

			// first choice, nothing is already highlighted
			if (m_selected)
			{
				m_selected_piece = NULL;

				// getting coords of the click after highlighting selected piece, ignore clicks outside
				std::tuple<int, int> coordinates = get_coordinates();
				int x = std::get<0>(coordinates);
				int y = std::get<1>(coordinates);

				if (x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1)
					continue;

				//
				if (!m_to_delete_list.empty() && (*m_board)[x][y] != m_moving_piece)
				{
#ifdef _DEBUG
					if ((*m_board)[x][y])
						std::cout << "Multicapture: this isn't the moving piece." << std::endl;
					else
						std::cout << "Multicapture: this is an empty space." << std::endl;
#endif
					m_selected = false;
					m_selected_piece = NULL;
					continue;
				}

				// check if the correspoding field contains a piece
				if ((*m_board)[x][y] != NULL)
				{
#ifdef _DEBUG
					std::cout << "x: " << x << "; y: " << y << "; piece: " << (*m_board)[x][y] << std::endl;
#endif
					// check if player owns this piece
					if ((*m_board)[x][y]->get_owner() == m_current_player)
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
									if (dynamic_cast<available_capture*>(a))
									{
										found_capture = true;
										return false;
									}
									return true;
								});
#ifdef _DEBUG
							for_each((*m_board)[x][y]->get_av_list()->begin(), (*m_board)[x][y]->get_av_list()->end(), [this](available_move* a)
								{
									m_os << "available: x: " << a->get_x() << "; y: " << a->get_y();
									if (dynamic_cast<available_capture*>(a))
										m_os << "; max captures: " << dynamic_cast<available_capture*>(a)->get_max_score();
									m_os << std::endl;
								});
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
				m_selected = false;
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
		print_results(m_os);
	}

	bool game::evaluate(std::list<piece*>* list, std::vector<std::vector<piece*>>* board, int* counter, base_player* player, int last_capture_direction, std::list<piece*>* dead_list, piece* moving_piece) // add moving piece
	{
		bool av_capture = false;
		clear_list(list);

		// is there a multicapture
		if (moving_piece)
		{
			if (dynamic_cast<king*>(moving_piece))
			{
				av_capture = evaluate_piece(dynamic_cast<king*>(moving_piece), list, board, counter, player, last_capture_direction, dead_list);
			}
			else
			{
				av_capture = evaluate_piece(moving_piece, list, board, counter, player);
			}
		}
		else
		{
			assert(dead_list);
			if (list->empty())
			{
#ifdef _DEBUG
				m_os << "Evaluating empty list" << std::endl;
#endif
			}
			else
			{
				for_each(list->begin(), list->end(), [this, &board, &list, &counter, &av_capture, &player, &last_capture_direction, &dead_list](piece* p)
					{
						bool local_available = false;
						if (dynamic_cast<king*>(p))
						{
							local_available = evaluate_piece(dynamic_cast<king*>(p), list, board, counter, player, last_capture_direction, dead_list);
							if (local_available)
								av_capture = true;
						}
						else
						{
							local_available = evaluate_piece(p, list, board, counter, player);
							if (local_available)
								av_capture = true;
						}
					});

				// check, when there wasn't multicapture, but there are two pieces that can do captures
				int pieces_with_captures = 0;
				all_of(list->begin(), list->end(), [&pieces_with_captures](piece* p)
					{
						all_of(p->get_av_list()->begin(), p->get_av_list()->end(), [&pieces_with_captures](available_move* a)
							{
								// if at least one of available moves is a capture, increment and go to the next piece
								if (dynamic_cast<available_capture*>(a))
								{
									pieces_with_captures++;
									return false;
								}
								return true;
							});
						// stop checking, if there is at least two pieces containing separate captures
						if (pieces_with_captures >= 2)
							return false;
						return true;
					});
				if (pieces_with_captures >= 2)
				{
					// find maximal capture counter over all captures
					int max_captures_overall = 0;
					for_each(list->begin(), list->end(), [&max_captures_overall](piece* p)
						{
							for_each(p->get_av_list()->begin(), p->get_av_list()->end(), [&max_captures_overall](available_move* a)
								{
									if (dynamic_cast<available_capture*>(a))
									{
										int captures = dynamic_cast<available_capture*>(a)->get_max_score();
										if (captures > max_captures_overall)
											max_captures_overall = captures;
									}
								});
						});
#ifdef _DEBUG
					m_os << "max captures overall: " << max_captures_overall << std::endl;
#endif
					// delete possible captures with score lower than max
					for_each(list->begin(), list->end(), [this, &max_captures_overall](piece* p)
						{
							std::list<available_move*> to_delete;
							for_each(p->get_av_list()->begin(), p->get_av_list()->end(), [this, &to_delete, &max_captures_overall, &p](available_move* a)
								{
									if (dynamic_cast<available_capture*>(a))
									{
										int captures = dynamic_cast<available_capture*>(a)->get_max_score();
										if (captures < max_captures_overall)
										{
											to_delete.push_back(a);
#ifdef _DEBUG
											m_os << "deleting capture: x: " << a->get_x() << "; y: " << a->get_y() << "; captures: " << captures << std::endl;
#endif
										}
									}
								});
							while (!to_delete.empty())
							{
								available_move* a = to_delete.front();
								p->get_av_list()->remove(a);
								to_delete.pop_front();
							}
						});
				}
			}
		}
#ifdef _DEBUG
		std::cout << "Evaluation returns: ";
		av_capture ? (std::cout << "true") : (std::cout << "false");
		std::cout << std::endl;
#endif
		return av_capture;
	}

	bool game::evaluate_piece(piece* p, std::list<piece*>* list, std::vector<std::vector<piece*>>* board, int* counter, base_player* player)
	{
		assert(dynamic_cast<piece*>(p));

		bool av_capture = false;

		// x coordinate of evaluated piece
		int x = p->get_x();
		// y coordinate of evaluated piece
		int y = p->get_y();

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
		if (x + 2 <= s_size - 1 && y - 2 >= 0 && (*board)[x + 1][y - 1] != NULL && (*board)[x + 1][y - 1]->get_owner() == player->get_next_player() && (*board)[x + 2][y - 2] == NULL)
			possible_capture_top_right = true;

		// capture top left (1)
		if (x - 2 >= 0 && y - 2 >= 0 && (*board)[x - 1][y - 1] != NULL && (*board)[x - 1][y - 1]->get_owner() == player->get_next_player() && (*board)[x - 2][y - 2] == NULL)
			possible_capture_top_left = true;

		// capture bottom right (2)
		if (x + 2 <= s_size - 1 && y + 2 <= s_size - 1 && (*board)[x + 1][y + 1] != NULL && (*board)[x + 1][y + 1]->get_owner() == player->get_next_player() && (*board)[x + 2][y + 2] == NULL)
			possible_capture_bottom_right = true;

		// capture bottom left (3)
		if (x - 2 >= 0 && y + 2 <= s_size - 1 && (*board)[x - 1][y + 1] != NULL && (*board)[x - 1][y + 1]->get_owner() == player->get_next_player() && (*board)[x - 2][y + 2] == NULL)
			possible_capture_bottow_left = true;


		if (possible_capture_top_left || possible_capture_top_right || possible_capture_bottow_left || possible_capture_bottom_right)
		{
			av_capture = true;

			// evaluate copy of the board recursively in every direction and find highest number of captures to add to base moves list
			int capture_counter[4] = { 0, 0, 0, 0 }; // 0 - top right, 1 - top left, 2 - bottom right, 3 - bottom left

			int last_capture_direction;

			if (possible_capture_top_right)
			{
				//(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
				capture_counter[0] = 1; // change here to get from counter, then increment?

				// copy the board and make empty list for moved piece
				std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
				std::list<piece*> copy_of_list;

				for (int i = 0; i < s_size; ++i)
				{
					for (int j = 0; j < s_size; ++j)
					{
						piece* p = (*board)[i][j];
						if (p)
						{
							if (dynamic_cast<king*>(p))
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
							else
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
						}
					}
				}

				// make planned move
				piece* moving_piece = (*copy_of_board)[x][y];
				(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
				moving_piece->set_x(x + 2);
				moving_piece->set_y(y - 2);
				(*copy_of_board)[x + 2][y - 2] = moving_piece;
				copy_of_list.push_back(moving_piece);
				(*copy_of_board)[x + 1][y - 1] = NULL;
				moving_piece = (*copy_of_board)[x + 2][y - 2];
				last_capture_direction = 0;
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
					evaluate(&copy_of_list, copy_of_board, &moves, player, last_capture_direction, NULL, moving_piece);
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
					evaluate(&copy_of_list, copy_of_board, counter, player, last_capture_direction, NULL, moving_piece);
				}
			}

			if (possible_capture_top_left)
			{
				//(*p).get_av_list()->push_back(new available_capture(x - 2, y - 2, x - 1, y - 1));
				capture_counter[1] = 1;

				// copy the board and make empty list for moved piece
				std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
				std::list<piece*> copy_of_list;

				for (int i = 0; i < s_size; ++i)
				{
					for (int j = 0; j < s_size; ++j)
					{
						piece* p = (*board)[i][j];
						if (p)
						{
							if (dynamic_cast<king*>(p))
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
							else
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
						}
					}
				}

				// make planned move
				piece* moving_piece = (*copy_of_board)[x][y];
				(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
				moving_piece->set_x(x - 2);
				moving_piece->set_y(y - 2);
				(*copy_of_board)[x - 2][y - 2] = moving_piece;
				copy_of_list.push_back(moving_piece);
				(*copy_of_board)[x - 1][y - 1] = NULL;
				moving_piece = (*copy_of_board)[x - 2][y - 2];
				last_capture_direction = 1;
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
					evaluate(&copy_of_list, copy_of_board, &moves, player, last_capture_direction, NULL, moving_piece);
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
					evaluate(&copy_of_list, copy_of_board, counter, player, last_capture_direction, NULL, moving_piece);
				}
			}

			if (possible_capture_bottom_right)
			{
				//(*p).get_av_list()->push_back(new available_capture(x + 2, y + 2, x + 1, y + 1));
				capture_counter[2] = 1;

				// copy the board and make empty list for moved piece
				std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
				std::list<piece*> copy_of_list;

				for (int i = 0; i < s_size; ++i)
				{
					for (int j = 0; j < s_size; ++j)
					{
						piece* p = (*board)[i][j];
						if (p)
						{
							if (dynamic_cast<king*>(p))
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
							else
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
						}
					}
				}

				// make planned move
				piece* moving_piece = (*copy_of_board)[x][y];
				(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
				moving_piece->set_x(x + 2);
				moving_piece->set_y(y + 2);
				(*copy_of_board)[x + 2][y + 2] = moving_piece;
				copy_of_list.push_back(moving_piece);
				(*copy_of_board)[x + 1][y + 1] = NULL;
				moving_piece = (*copy_of_board)[x + 2][y + 2];
				last_capture_direction = 2;
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
					evaluate(&copy_of_list, copy_of_board, &moves, player, last_capture_direction, NULL, moving_piece);
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
					evaluate(&copy_of_list, copy_of_board, counter, player, last_capture_direction, NULL, moving_piece);
				}
			}

			if (possible_capture_bottow_left)
			{
				//(*p).get_av_list()->push_back(new available_capture(x - 2, y + 2, x - 1, y + 1));
				capture_counter[3] = 1;

				// copy the board and make empty list for moved piece
				std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
				std::list<piece*> copy_of_list;

				for (int i = 0; i < s_size; ++i)
				{
					for (int j = 0; j < s_size; ++j)
					{
						piece* p = (*board)[i][j];
						if (p)
						{
							if (dynamic_cast<king*>(p))
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
							else
							{
								if (p->get_owner() == player)
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
								else if (p->get_owner() == player->get_next_player())
									(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
								else
									throw std::runtime_error("King evaluation: wrong membership");
							}
						}
					}
				}

				// make planned move
				piece* moving_piece = (*copy_of_board)[x][y];
				(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
				moving_piece->set_x(x - 2);
				moving_piece->set_y(y + 2);
				(*copy_of_board)[x - 2][y + 2] = moving_piece;
				copy_of_list.push_back(moving_piece);
				(*copy_of_board)[x - 1][y + 1] = NULL;
				moving_piece = (*copy_of_board)[x - 2][y + 2];
				last_capture_direction = 3;
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
					evaluate(&copy_of_list, copy_of_board, &moves, player, last_capture_direction, NULL, moving_piece);
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
					evaluate(&copy_of_list, copy_of_board, counter, player, last_capture_direction, NULL, moving_piece);
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
						(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1, max));
						break;
					}
					case 1:
					{
#ifdef _DEBUG
						std::cout << "top left direction: " << capture_counter[1] << std::endl;
#endif
						(*p).get_av_list()->push_back(new available_capture(x - 2, y - 2, x - 1, y - 1, max));
						break;
					}
					case 2:
					{
#ifdef _DEBUG
						std::cout << "bottom right direction: " << capture_counter[2] << std::endl;
#endif
						(*p).get_av_list()->push_back(new available_capture(x + 2, y + 2, x + 1, y + 1, max));
						break;
					}
					case 3:
					{
#ifdef _DEBUG
						std::cout << "bottom left direction: " << capture_counter[3] << std::endl;
#endif
						(*p).get_av_list()->push_back(new available_capture(x - 2, y + 2, x - 1, y + 1, max));
						break;
					}
					default:
						throw std::exception("Evaluation error on piece " + p->get_sign());
					}
				}

			// test code
			if (*counter == NULL)
				*counter = max;
		}
		else
		{
			if (*counter == NULL)
			{
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
#ifdef _DEBUG
		std::cout << "piece evaluation returns: ";
		av_capture ? (std::cout << "true") : (std::cout << "false");
		std::cout << std::endl;
#endif
		return av_capture;
	}

	bool game::evaluate_piece(king* p, std::list<piece*>* list, std::vector<std::vector<piece*>>* board, int* counter, base_player* player, int last_capture_direction, std::list<piece*>* dead_list)
	{
		assert(dynamic_cast<king*>(p));

		bool av_capture = false;

		// x coordinate of evaluated piece
		int x = p->get_x();
		// y coordinate of evaluated piece
		int y = p->get_y();

#ifdef _DEBUG
		std::cout << "evaluating the king" << std::endl;
		std::cout << "x: " << x << "; y: " << y << std::endl;

		if ((*board)[x][y] != NULL)
			std::cout << (*board)[x][y] << std::endl;
#endif  
		// flags blocking opposite captures in multicapture
		bool possible_top_right = true;
		bool possible_top_left = true;
		bool possible_bottom_right = true;
		bool possible_bottom_left = true;

		bool multicapture = !(*dead_list).empty();

		if (multicapture) // cannot capture in one direction and then in opposite
		{
			switch (last_capture_direction)
			{
			case 0: // top right
				possible_bottom_left = false;
				break;
			case 1: // top left
				possible_bottom_right = false;
				break;
			case 2: // bottom right
				possible_top_left = false;
				break;
			case 3: // bottom left
				possible_top_right = false;
				break;
			default:
				throw std::runtime_error("King evaluation: wrong member: last capture direction");
			}
		}
#ifdef _DEBUG
		if (!possible_top_right)
			m_os << "/ top right capture won't be available" << std::endl;
		else if (!possible_top_left)
			m_os << "/ top left capture won't be available" << std::endl;
		else if (!possible_bottom_right)
			m_os << "/ bottom right capture won't be available" << std::endl;
		else if (!possible_bottom_left)
			m_os << "/ bottom left capture won't be available" << std::endl;
#endif

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
		std::vector<available_move> local_moves_top_right;
		std::vector<available_move> local_moves_top_left;
		std::vector<available_move> local_moves_bottom_right;
		std::vector<available_move> local_moves_bottom_left;

		// lists (vectors) of captures: one capture in each direction, every in possible location
		std::vector<available_capture> local_captures_top_right;
		std::vector<available_capture> local_captures_top_left;
		std::vector<available_capture> local_captures_bottom_right;
		std::vector<available_capture> local_captures_bottom_left;

		// temporary board made from dead list
		std::vector<std::vector<piece*>> dead_board(s_size, std::vector<piece*>(s_size, NULL));
		for_each(dead_list->begin(), dead_list->end(), [&dead_board, this, player](piece* p)
			{
				int x = p->get_x();
				int y = p->get_y();
				if (dead_board[x][y] == NULL)
				{
					if (dynamic_cast<king*>(p))
						dead_board[x][y] = new king(p->get_sign(), x, y, p->is_alive(), p->get_owner());
					else
						dead_board[x][y] = new piece(p->get_sign(), x, y, p->is_alive(), p->get_owner());
				}
				else
					throw std::runtime_error("Copying to delete list: wrong piece data: overwriting another piece");
			});
#ifdef _DEBUG
		m_os << "Dead board:" << std::endl;
		m_os << &dead_board << std::endl;
#endif
		int i;
		if (possible_top_right)
		{
			// capture top right (0) + -
#ifdef _DEBUG
			std::cout << "top right checking" << std::endl;
#endif
			i = 1;
			while (x + i + 1 <= s_size - 1 && y - i - 1 >= 0)
			{
#ifdef _DEBUG
				std::cout << "checking: x: " << x + i << ", y: " << y - i << " and place to go: x: " << x + i + 1 << ", y: " << y - i - 1 << std::endl;
#endif
				// searching for own piece (cannot jump across them)
				if ((*board)[x + i][y - i] != NULL && (*board)[x + i][y - i]->get_owner() == player)
				{
#ifdef _DEBUG
					std::cout << "*found own piece and breaking" << std::endl;
#endif
					possible_capture_top_right.push_back(false);
					break;
				}
				// searching for dead piece (cannot jump across them)
				else if (dead_board[x + i][y - i] != NULL && dead_board[x + i][y - i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found dead piece and breaking" << std::endl;
#endif
					possible_capture_top_right.push_back(false);
					break;
				}
				else if ((*board)[x + i][y - i] != NULL && (*board)[x + i][y - i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
					if ((*board)[x + i + 1][y - i - 1] == NULL && dead_board[x + i + 1][y - i - 1] == NULL)
					{
#ifdef _DEBUG
						std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
						possible_capture_top_right.push_back(true);
						at_least_one_capture_top_right = true;

						// adding all possible capture options
						int j = 0;
						while (x + i + 1 + j <= s_size - 1 && y - i - 1 - j >= 0 && (*board)[x + i + 1 + j][y - i - 1 - j] == NULL && dead_board[x + i + 1 + j][y - i - 1 - j] == NULL)
						{
#ifdef _DEBUG
							std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x + i + 1 + j << ", y: " << y - i - 1 - j << std::endl;
#endif
							local_captures_top_right.push_back(available_capture(x + i + 1 + j, y - i - 1 - j, x + i, y - i, 1));
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
					local_moves_top_right.push_back(available_move(x + i, y - i));
					possible_capture_top_right.push_back(false);
				}

				++i;
			}
			if (x + i <= s_size - 1 && y - i >= 0 && (*board)[x + i][y - i] == NULL && !at_least_one_capture_top_right) // last, not checked field (checking looks for next which is outside the boundaries)
			{
#ifdef _DEBUG
				std::cout << "last but not least" << std::endl;
#endif
				local_moves_top_right.push_back(available_move(x + i, y - i));
			}
		}

		if (possible_top_left)
		{
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
				if ((*board)[x - i][y - i] != NULL && (*board)[x - i][y - i]->get_owner() == player)
				{
#ifdef _DEBUG
					std::cout << "*found own piece and breaking" << std::endl;
#endif
					possible_capture_top_left.push_back(false);
					break;
				}
				// searching for dead piece (cannot jump across them)
				else if (dead_board[x - i][y - i] != NULL && dead_board[x - i][y - i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found dead piece and breaking" << std::endl;
#endif
					possible_capture_top_right.push_back(false);
					break;
				}
				else if ((*board)[x - i][y - i] != NULL && (*board)[x - i][y - i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
					if ((*board)[x - i - 1][y - i - 1] == NULL && dead_board[x - i - 1][y - i - 1] == NULL)
					{
#ifdef _DEBUG
						std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
						possible_capture_top_left.push_back(true);
						at_least_one_capture_top_left = true;

						// adding all possible capture options
						int j = 0;
						while (x - i - 1 - j >= 0 && y - i - 1 - j >= 0 && (*board)[x - i - 1 - j][y - i - 1 - j] == NULL && dead_board[x - i - 1 - j][y - i - 1 - j] == NULL)
						{
#ifdef _DEBUG
							std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x - i - 1 - j << ", y: " << y - i - 1 - j << std::endl;
#endif
							local_captures_top_left.push_back(available_capture(x - i - 1 - j, y - i - 1 - j, x - i, y - i, 1));
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
						if (at_least_one_capture_top_left)
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
					local_moves_top_left.push_back(available_move(x - i, y - i));
					possible_capture_top_left.push_back(false);
				}
				++i;
			}
			if (x - i >= 0 && y - i >= 0 && (*board)[x - i][y - i] == NULL && !at_least_one_capture_top_left) // last, not checked field (checking looks for next which is outside the boundaries)
			{
#ifdef _DEBUG
				std::cout << "last but not least" << std::endl;
#endif
				local_moves_top_left.push_back(available_move(x - i, y - i));
			}
		}


		if (possible_bottom_right)
		{
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
				if ((*board)[x + i][y + i] != NULL && (*board)[x + i][y + i]->get_owner() == player)
				{
#ifdef _DEBUG
					std::cout << "*found own piece and breaking" << std::endl;
#endif
					possible_capture_bottom_right.push_back(false);
					break;
				}
				// searching for dead piece (cannot jump across them)
				else if (dead_board[x + i][y + i] != NULL && dead_board[x + i][y + i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found dead piece and breaking" << std::endl;
#endif
					possible_capture_top_right.push_back(false);
					break;
				}
				else if ((*board)[x + i][y + i] != NULL && (*board)[x + i][y + i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
					if ((*board)[x + i + 1][y + i + 1] == NULL && dead_board[x + i + 1][y + i + 1] == NULL)
					{
#ifdef _DEBUG
						std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
						possible_capture_bottom_right.push_back(true);
						at_least_one_capture_bottom_right = true;

						// adding all possible capture options
						int j = 0;
						while (x + i + 1 + j <= s_size - 1 && y + i + 1 + j <= s_size - 1 && (*board)[x + i + 1 + j][y + i + 1 + j] == NULL && dead_board[x + i + 1 + j][y + i + 1 + j] == NULL)
						{
#ifdef _DEBUG
							std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x + i + 1 + j << ", y: " << y + i + 1 + j << std::endl;
#endif
							local_captures_bottom_right.push_back(available_capture(x + i + 1 + j, y + i + 1 + j, x + i, y + i, 1));
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
						if (at_least_one_capture_bottom_right)
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
					local_moves_bottom_right.push_back(available_move(x + i, y + i));
					possible_capture_bottom_right.push_back(false);
				}

				++i;
			}
			if (x + i <= s_size - 1 && y + i <= s_size - 1 && (*board)[x + i][y + i] == NULL && !at_least_one_capture_bottom_right) // last, not checked field (checking looks for next which is outside the boundaries)
			{
#ifdef _DEBUG
				std::cout << "last but not least" << std::endl;
#endif
				local_moves_bottom_right.push_back(available_move(x + i, y + i));
			}
		}

		if (possible_bottom_left)
		{
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
				if ((*board)[x - i][y + i] != NULL && (*board)[x - i][y + i]->get_owner() == player)
				{
#ifdef _DEBUG
					std::cout << "*found own piece and breaking" << std::endl;
#endif
					possible_capture_bottom_left.push_back(false);
					break;
				}
				// searching for dead piece (cannot jump across them)
				else if (dead_board[x - i][y + i] != NULL && dead_board[x - i][y + i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found dead piece and breaking" << std::endl;
#endif
					possible_capture_top_right.push_back(false);
					break;
				}
				else if ((*board)[x - i][y + i] != NULL && (*board)[x - i][y + i]->get_owner() == player->get_next_player())
				{
#ifdef _DEBUG
					std::cout << "*found opponent's piece and checking for next fields" << std::endl;
#endif
					if ((*board)[x - i - 1][y + i + 1] == NULL && dead_board[x - i - 1][y + i + 1] == NULL)
					{
#ifdef _DEBUG
						std::cout << "*found opponent's piece that can be captured, searching for next" << std::endl;
#endif
						possible_capture_bottom_left.push_back(true);
						at_least_one_capture_bottom_left = true;

						// adding all possible capture options
						int j = 0;
						while (x - i - 1 - j >= 0 && y + i + 1 + j <= s_size - 1 && (*board)[x - i - 1 - j][y + i + 1 + j] == NULL && dead_board[x - i - 1 - j][y + i + 1 + j] == NULL)
						{
#ifdef _DEBUG
							std::cout << "\%piece at x: " << x << ", y: " << y << " can capture on coords: x: " << x - i - 1 - j << ", y: " << y + i + 1 + j << std::endl;
#endif
							local_captures_bottom_left.push_back(available_capture(x - i - 1 - j, y + i + 1 + j, x - i, y + i, 1));
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
						if (at_least_one_capture_bottom_left)
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
					local_moves_bottom_left.push_back(available_move(x - i, y + i));
					possible_capture_bottom_left.push_back(false);
				}

				++i;
			}
			if (x - i >= 0 && y + i <= s_size - 1 && (*board)[x - i][y + i] == NULL && !at_least_one_capture_bottom_left) // last, not checked field (checking looks for next which is outside the boundaries)
			{
#ifdef _DEBUG
				std::cout << "Added last possible field" << std::endl;
#endif
				local_moves_bottom_left.push_back(available_move(x - i, y + i));
			}
		}

#ifdef _DEBUG
		// list top right moves and captures
		std::cout << "king top right moves" << std::endl;
		for_each(possible_capture_top_right.begin(), possible_capture_top_right.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

		std::cout << "local moves" << std::endl;
		for_each(local_moves_top_right.begin(), local_moves_top_right.end(), [](available_move& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << std::endl; });

		std::cout << "local captures" << std::endl;
		for_each(local_captures_top_right.begin(), local_captures_top_right.end(), [](available_capture& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << "; x_d: " << a.get_x_d() << "; y_d: " << a.get_y_d() << std::endl; });

		// list top left moves and captures
		std::cout << "king top left moves" << std::endl;
		for_each(possible_capture_top_left.begin(), possible_capture_top_left.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

		std::cout << "local moves" << std::endl;
		for_each(local_moves_top_left.begin(), local_moves_top_left.end(), [](available_move& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << std::endl; });

		std::cout << "local captures" << std::endl;
		for_each(local_captures_top_left.begin(), local_captures_top_left.end(), [](available_capture& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << "; x_d: " << a.get_x_d() << "; y_d: " << a.get_y_d() << std::endl; });

		// list bottom right moves and captures
		std::cout << "king bottom right moves" << std::endl;
		for_each(possible_capture_bottom_right.begin(), possible_capture_bottom_right.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

		std::cout << "local moves" << std::endl;
		for_each(local_moves_bottom_right.begin(), local_moves_bottom_right.end(), [](available_move& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << std::endl; });

		std::cout << "local captures" << std::endl;
		for_each(local_captures_bottom_right.begin(), local_captures_bottom_right.end(), [](available_capture& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << "; x_d: " << a.get_x_d() << "; y_d: " << a.get_y_d() << std::endl; });

		// list bottom left moves and captures
		std::cout << "king bottom left moves" << std::endl;
		for_each(possible_capture_bottom_left.begin(), possible_capture_bottom_left.end(), [i = 1](bool b) mutable { std::cout << i++ << ": " << b << std::endl; });

		std::cout << "local moves" << std::endl;
		for_each(local_moves_bottom_left.begin(), local_moves_bottom_left.end(), [](available_move& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << std::endl; });

		std::cout << "local captures" << std::endl;
		for_each(local_captures_bottom_left.begin(), local_captures_bottom_left.end(), [](available_capture& a) { std::cout << "-x: " << a.get_x() << "; y: " << a.get_y() << "; x_d: " << a.get_x_d() << "; y_d: " << a.get_y_d() << std::endl; });
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

			if (*counter != NULL)
				(*counter)++;

			// top right recursive evaluation
			if (at_least_one_capture_top_right)
			{
				for_each(local_captures_top_right.begin(), local_captures_top_right.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y, &dead_list](available_capture& a) mutable
					{
						// todo: save last capture direction locally after the move, copy dead piece list

						capture_counter[0][i] = 1; // change here to get from counter, then increment?

						// copy coords
						int x_to_go = a.get_x();
						int y_to_go = a.get_y();
						int x_to_delete = a.get_x_d();
						int y_to_delete = a.get_y_d();

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
						std::list<piece*> copy_of_list;
						std::list<piece*> copy_of_dead;

						for (int i = 0; i < s_size; ++i)
						{
							for (int j = 0; j < s_size; ++j)
							{
								piece* p = (*board)[i][j];
								if (p)
								{
									if (dynamic_cast<king*>(p))
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
									else
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
								}
							}
						}
						for_each(dead_list->begin(), dead_list->end(), [&copy_of_dead](piece* p) { copy_of_dead.push_back(new piece(p->get_sign(), p->get_x(), p->get_y(), false, p->get_owner())); });

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y]; // consider changing to king
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x_to_go);
						moving_piece->set_y(y_to_go);
						(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
						copy_of_list.push_back(moving_piece);
						copy_of_dead.push_back((*copy_of_board)[x_to_delete][y_to_delete]);
						(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
							evaluate(&copy_of_list, copy_of_board, &(capture_counter[0][i]), player, 0, &copy_of_dead, moving_piece);
#ifdef _DEBUG
							std::cout << "moves counter (top right): " << capture_counter[0][i] << std::endl;
#endif
						}
						else
						{
#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter equal to: " << (*counter) << std::endl;
#endif

							evaluate(&copy_of_list, copy_of_board, counter, player, 0, &copy_of_dead, moving_piece);
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
				for_each(local_captures_top_left.begin(), local_captures_top_left.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y, &dead_list](available_capture& a) mutable
					{
						capture_counter[1][i] = 1; // change here to get from counter, then increment?

						// copy coords
						int x_to_go = a.get_x();
						int y_to_go = a.get_y();
						int x_to_delete = a.get_x_d();
						int y_to_delete = a.get_y_d();

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
						std::list<piece*> copy_of_list;
						std::list<piece*> copy_of_dead;

						for (int i = 0; i < s_size; ++i)
						{
							for (int j = 0; j < s_size; ++j)
							{
								piece* p = (*board)[i][j];
								if (p)
								{
									if (dynamic_cast<king*>(p))
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
									else
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
								}
							}
						}
						for_each(dead_list->begin(), dead_list->end(), [&copy_of_dead](piece* p) { copy_of_dead.push_back(new piece(p->get_sign(), p->get_x(), p->get_y(), false, p->get_owner())); });

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y];
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x_to_go);
						moving_piece->set_y(y_to_go);
						(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
						copy_of_list.push_back(moving_piece);
						copy_of_dead.push_back((*copy_of_board)[x_to_delete][y_to_delete]);
						(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
							evaluate(&copy_of_list, copy_of_board, &(capture_counter[1][i]), player, 1, &copy_of_dead, moving_piece);
#ifdef _DEBUG
							std::cout << "moves counter (top left): " << capture_counter[1][i] << std::endl;
#endif
						}
						else
						{
#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter equal to: " << (*counter) << std::endl;
#endif
							evaluate(&copy_of_list, copy_of_board, counter, player, 1, &copy_of_dead, moving_piece);
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
				for_each(local_captures_bottom_right.begin(), local_captures_bottom_right.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y, &dead_list](available_capture& a) mutable
					{
						//(*p).get_av_list()->push_back(new available_capture(x + 2, y - 2, x + 1, y - 1));
						capture_counter[2][i] = 1; // change here to get from counter, then increment?

						// copy coords
						int x_to_go = a.get_x();
						int y_to_go = a.get_y();
						int x_to_delete = a.get_x_d();
						int y_to_delete = a.get_y_d();

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
						std::list<piece*> copy_of_list;
						std::list<piece*> copy_of_dead;

						for (int i = 0; i < s_size; ++i)
						{
							for (int j = 0; j < s_size; ++j)
							{
								piece* p = (*board)[i][j];
								if (p)
								{
									if (dynamic_cast<king*>(p))
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
									else
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
								}
							}
						}
						for_each(dead_list->begin(), dead_list->end(), [&copy_of_dead](piece* p) { copy_of_dead.push_back(new piece(p->get_sign(), p->get_x(), p->get_y(), false, p->get_owner())); });

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y];
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x_to_go);
						moving_piece->set_y(y_to_go);
						(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
						copy_of_list.push_back(moving_piece);
						copy_of_dead.push_back((*copy_of_board)[x_to_delete][y_to_delete]);
						(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
							evaluate(&copy_of_list, copy_of_board, &(capture_counter[2][i]), player, 2, &copy_of_dead, moving_piece);
#ifdef _DEBUG
							std::cout << "moves counter (bottom right): " << capture_counter[2][i] << std::endl;
#endif
						}
						else
						{
#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter equal to: " << (*counter) << std::endl;
#endif
							evaluate(&copy_of_list, copy_of_board, counter, player, 2, &copy_of_dead, moving_piece);
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
				for_each(local_captures_bottom_left.begin(), local_captures_bottom_left.end(), [i = 0, this, &capture_counter, &counter, &board, &player, &x, &y, &dead_list](available_capture& a) mutable
					{
						capture_counter[3][i] = 1; // change here to get from counter, then increment?

						// copy coords
						int x_to_go = a.get_x();
						int y_to_go = a.get_y();
						int x_to_delete = a.get_x_d();
						int y_to_delete = a.get_y_d();

						// copy the board and make empty list for moved piece
						std::vector<std::vector<piece*>>* copy_of_board = new std::vector<std::vector<piece*>>(s_size, std::vector<piece*>(s_size, 0));
						std::list<piece*> copy_of_list;
						std::list<piece*> copy_of_dead;

						for (int i = 0; i < s_size; ++i)
						{
							for (int j = 0; j < s_size; ++j)
							{
								piece* p = (*board)[i][j];
								if (p)
								{
									if (dynamic_cast<king*>(p))
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new king(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
									else
									{
										if (p->get_owner() == player)
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player);
										else if (p->get_owner() == player->get_next_player())
											(*copy_of_board)[i][j] = new piece(p->get_sign(), p->get_x(), p->get_y(), p->is_alive(), player->get_next_player());
										else
											throw std::runtime_error("King evaluation: wrong membership");
									}
								}
							}
						}
						for_each(dead_list->begin(), dead_list->end(), [&copy_of_dead](piece* p)
							{
								copy_of_dead.push_back(new piece(p->get_sign(), p->get_x(), p->get_y(), false, p->get_owner()));
							});

						// make planned move
						piece* moving_piece = (*copy_of_board)[x][y];
						(*copy_of_board)[moving_piece->get_x()][moving_piece->get_y()] = NULL;
						moving_piece->set_x(x_to_go);
						moving_piece->set_y(y_to_go);
						(*copy_of_board)[x_to_go][y_to_go] = moving_piece;
						copy_of_list.push_back(moving_piece);
						copy_of_dead.push_back((*copy_of_board)[x_to_delete][y_to_delete]);
						(*copy_of_board)[x_to_delete][y_to_delete] = NULL;
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
							evaluate(&copy_of_list, copy_of_board, &(capture_counter[3][i]), player, 3, &copy_of_dead, moving_piece);
#ifdef _DEBUG
							std::cout << "moves counter (bottom left): " << capture_counter[3][i] << std::endl;
#endif
						}
						else
						{
#ifdef _DEBUG
							std::cout << "---------------------------------------------------------------" << std::endl;
							std::cout << "counter equal to: " << (*counter) << std::endl;
#endif
							evaluate(&copy_of_list, copy_of_board, counter, player, 3, &copy_of_dead, moving_piece);
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

			// save how many places have max captures
			int elements_with_max = 0;

			for (int i = 0; i < 4; ++i)
				for_each(capture_counter[i].begin(), capture_counter[i].end(), [&max_captures, &elements_with_max](int c)
					{
						if (c == max_captures)
							++elements_with_max;
						else if (c > max_captures)
						{
							max_captures = c;
							elements_with_max = 1;
						}
					});
#ifdef _DEBUG
			std::cout << "found max captures: " << max_captures << std::endl;
			std::cout << "elements with max captures: " << elements_with_max << std::endl;
#endif
			// actual adding captures to the piece
			for (int i = 0; i < 4; ++i)
				for_each(capture_counter[i].begin(), capture_counter[i].end(), [&max_captures, &i, &p, j = 0, &multicapture, &possible_top_right, &possible_top_left, &possible_bottom_right, &possible_bottom_left, &local_captures_top_right, &local_captures_top_left, &local_captures_bottom_right, &local_captures_bottom_left](int c) mutable
					{
						if (c == max_captures)
						{
							switch (i)
							{
							case 0:
								if (multicapture && possible_top_right || !multicapture)
									p->get_av_list()->push_back(new available_capture(local_captures_top_right[j].get_x(), local_captures_top_right[j].get_y(), local_captures_top_right[j].get_x_d(), local_captures_top_right[j].get_y_d(), max_captures));
								break;
							case 1:
								if (multicapture && possible_top_left || !multicapture)
									p->get_av_list()->push_back(new available_capture(local_captures_top_left[j].get_x(), local_captures_top_left[j].get_y(), local_captures_top_left[j].get_x_d(), local_captures_top_left[j].get_y_d(), max_captures));
								break;
							case 2:
								if (multicapture && possible_bottom_right || !multicapture)
									p->get_av_list()->push_back(new available_capture(local_captures_bottom_right[j].get_x(), local_captures_bottom_right[j].get_y(), local_captures_bottom_right[j].get_x_d(), local_captures_bottom_right[j].get_y_d(), max_captures));
								break;
							case 3:
								if (multicapture && possible_bottom_left || !multicapture)
									p->get_av_list()->push_back(new available_capture(local_captures_bottom_left[j].get_x(), local_captures_bottom_left[j].get_y(), local_captures_bottom_left[j].get_x_d(), local_captures_bottom_left[j].get_y_d(), max_captures));
								break;
							}
						}
						++j;
					});
			//if (*counter == NULL)
			//	*counter = max_captures;
		}
		else // only moves
		{
			if (multicapture && possible_top_right || !multicapture)
				for_each(local_moves_top_right.begin(), local_moves_top_right.end(), [&p](available_move& a)
					{
						p->get_av_list()->push_back(new available_move(a.get_x(), a.get_y()));
					});

			if (multicapture && possible_top_left || !multicapture)
				for_each(local_moves_top_left.begin(), local_moves_top_left.end(), [&p](available_move& a)
					{
						p->get_av_list()->push_back(new available_move(a.get_x(), a.get_y()));
					});

			if (multicapture && possible_bottom_right || !multicapture)
				for_each(local_moves_bottom_right.begin(), local_moves_bottom_right.end(), [&p](available_move& a)
					{
						p->get_av_list()->push_back(new available_move(a.get_x(), a.get_y()));
					});

			if (multicapture && possible_bottom_left || !multicapture)
				for_each(local_moves_bottom_left.begin(), local_moves_bottom_left.end(), [&p](available_move& a)
					{
						p->get_av_list()->push_back(new available_move(a.get_x(), a.get_y()));
					});
		}
#ifdef _DEBUG
		std::cout << "king evaluation returns: ";
		av_capture ? (std::cout << "true") : (std::cout << "false");
		std::cout << std::endl;
#endif
		return av_capture;
	}

	void game::clear_list(std::list<piece*>* list) { for_each(list->begin(), list->end(), [this](piece* p) { p->get_av_list()->clear(); }); }

	void game::print_pieces(std::list<piece*>* list)
	{
		std::for_each(list->begin(), list->end(), [i = 1, this](piece* p) mutable
			{
				m_os << i++ << "; sign: " << p << "; x: " << p->get_x() << "; y: " << p->get_y();
				if (dynamic_cast<king*>(p))
					m_os << "; king";
				m_os << std::endl;
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

	void game::debug_info(void)
	{
		m_os << "Board:" << std::endl;
		m_os << m_board << std::endl;

		m_os << "console game: ";
		m_console_game ? m_os << "true" << std::endl : m_os << "false" << std::endl;

		m_player_1 ? m_os << "player 1: " << m_player_1 : m_os << "not set" << std::endl;

		m_player_2 ? m_os << "player 2: " << m_player_2 : m_os << "not set" << std::endl;

		m_current_player ? m_os << "current player: " << m_current_player : m_os << "not set" << std::endl;

		m_os << "available capture: ";
		m_available_capture ? m_os << "true" << std::endl : m_os << "false" << std::endl;

		m_os << "last capture direction: ";
		switch (m_last_capture_direction)
		{
		case 0:
			m_os << "top right" << std::endl;
			break;
		case 1:
			m_os << "top left" << std::endl;
			break;
		case 2:
			m_os << "bottom right" << std::endl;
			break;
		case 3:
			m_os << "bottom left" << std::endl;
			break;
		default:
			m_os << "not set" << std::endl;
		}

		m_os << "finished game: ";
		m_is_finished ? m_os << "true" << std::endl : m_os << "false" << std::endl;

		m_os << "first won: ";
		m_first_won ? m_os << "true" << std::endl : m_os << "false" << std::endl;

		m_os << "second won: ";
		m_second_won ? m_os << "true" << std::endl : m_os << "false" << std::endl;

		m_os << "selected: ";
		m_selected ? m_os << "true" << std::endl : m_os << "false" << std::endl;

		m_os << "selected piece: ";
		if (m_selected_piece)
		{
			m_os << "sign: " << m_selected_piece->get_sign() << "; x: " << m_selected_piece->get_x() << "; y: " << m_selected_piece->get_y() << "; list of available moves: " << m_selected_piece->get_av_list()->size();
			m_os << "; is king: ";
			dynamic_cast<king*>(m_selected_piece) ? m_os << "true" << std::endl : m_os << "false" << std::endl;
		}
		else
		{
			m_os << "null" << std::endl;
		}

		m_os << "moving piece: ";
		if (m_moving_piece)
		{
			m_os << "sign: " << m_moving_piece->get_sign() << "; x: " << m_moving_piece->get_x() << "; y: " << m_moving_piece->get_y() << "; list of available moves: " << m_moving_piece->get_av_list()->size();
			m_os << "; is king: ";
			dynamic_cast<king*>(m_moving_piece) ? m_os << "true" << std::endl : m_os << "false" << std::endl;
		}
		else
		{
			m_os << "null" << std::endl;
		}

		std::tuple<int, int> coords = get_click_coordinates();
		int x = std::get<0>(coords);
		int y = std::get<1>(coords);
		if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1))
		{
			piece* hovered_piece = (*m_board)[x][y];
			m_os << "hovered piece: ";
			if (hovered_piece)
			{
				m_os << "sign: " << hovered_piece->get_sign() << "; x: " << hovered_piece->get_x() << "; y: " << hovered_piece->get_y() << "; list of available moves: " << hovered_piece->get_av_list()->size();
				m_os << "; is king: ";
				dynamic_cast<king*>(hovered_piece) ? m_os << "true" << std::endl : m_os << "false" << std::endl;
			}
			else
				m_os << "null" << std::endl;

		}

		m_os << "fps: " << m_fps << std::endl;
	}
}