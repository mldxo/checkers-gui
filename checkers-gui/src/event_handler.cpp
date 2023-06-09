#include "include/event_handler.h"
#include "include/game.h"
#include "include/piece.h"
#include "include/king.h"

namespace checkers
{
	event_handler::event_handler(game* game_pointer) : m_game_pointer(game_pointer) {}

	event_handler::~event_handler() {}

	void event_handler::handle_events(void)
	{
		std::ostream& os = m_game_pointer->m_os;
		std::ostream& log = m_game_pointer->m_log;
		sf::Window& window = m_game_pointer->m_gui->get_window();
		sf::Event& event = m_game_pointer->m_gui->get_event();
		while (window.pollEvent(event) || (bool)dynamic_cast<bot*>(m_game_pointer->m_current_player))
		{
			if (event.type == sf::Event::Closed || event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
			{
				window.close();
#ifdef _DEBUG
				os << "Closing the game" << std::endl;
				//save_pieces_to_file();
#endif
				break;
			}
#ifdef _DEBUG
			if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R)
			{
				os << "Reseted the game" << std::endl;
				m_game_pointer->m_first_won = false;
				m_game_pointer->m_second_won = false;
				m_game_pointer->m_player_1->set_captured_pieces(0);
				m_game_pointer->m_player_2->set_captured_pieces(0);
				break;
			}
#endif
			if ((m_game_pointer->m_player_1->get_pieces() == 0 || m_game_pointer->m_player_2->get_pieces() == 0) && (m_game_pointer->m_player_1->get_captured_pieces() > 0 || m_game_pointer->m_player_2->get_captured_pieces() > 0))
			{
				//m_signaled_bot = false;
				if (event.type == sf::Event::KeyPressed || event.type == sf::Event::MouseButtonPressed)
				{
					os << "Game was finished. Click Escape!" << std::endl;
				}
				break;
			}
			else if (m_game_pointer->m_game_freeze)
			{
				if (event.type == sf::Event::KeyPressed || event.type == sf::Event::MouseButtonPressed)
				{
					if (event.key.code == sf::Keyboard::X)
					{
						os << "Unfrozen the game." << std::endl;
						m_game_pointer->m_game_freeze = false;
					}
					else
						os << "Game is frozen" << std::endl;
					m_game_pointer->debug_info(os);
				}
				break;
			}
			else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left || (bool)dynamic_cast<bot*>(m_game_pointer->m_current_player) || m_game_pointer->m_console_game)
			{
				//m_signaled_bot = false;
				if (!m_game_pointer->m_selected)
				{
					m_game_pointer->select_piece();
					break;
				}
				if (m_game_pointer->m_selected_piece)
				{
					m_game_pointer->move_selected_piece();
					break;
				}
			}
#ifdef _DEBUG
			if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right)
			{
				// display debug info
				m_game_pointer->debug_info(os);
				break;
			}
			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::E)
				{
					// re-evaluate the game
					log << "Manually re-evaluating the game" << std::endl;
					m_game_pointer->m_selected_piece = nullptr;
					m_game_pointer->m_selected = false;
					int dummy = 0;
					m_game_pointer->m_available_capture = m_game_pointer->evaluate(m_game_pointer->m_current_player->get_list(), m_game_pointer->m_board, &dummy, dummy, m_game_pointer->m_current_player, m_game_pointer->m_last_capture_direction, &m_game_pointer->m_to_delete_list, m_game_pointer->m_moving_piece);
					os << "Game re-evaluated" << std::endl;
					break;
				}
				else if (event.key.code == sf::Keyboard::T)
				{
					// write a separator
					os << "--------------------------------------------------------------" << std::endl;
					break;
				}
				else if (event.key.code == sf::Keyboard::U && ((sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) || (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))))
				{
					// add new piece to the first player
					std::pair<int, int> coords = m_game_pointer->get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_game_pointer->m_board)[x][y] == nullptr)
					{
						log << "Manually adding king to the first player at coordinates: x: " << x << "; " << y << std::endl;
						m_game_pointer->add_new_piece(&m_game_pointer->m_p_list_1, m_game_pointer->m_board, m_game_pointer->m_player_1, new king(m_game_pointer->m_player_1->get_sign(), x, y, true, m_game_pointer->m_player_1));
					}
					break;
				}
				else if (event.key.code == sf::Keyboard::I && ((sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) || (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))))
				{
					// add new piece to the second player
					std::pair<int, int> coords = m_game_pointer->get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_game_pointer->m_board)[x][y] == nullptr)
					{
						log << "Manually adding king to the second player at coordinates: x: " << x << "; " << y << std::endl;
						m_game_pointer->add_new_piece(&m_game_pointer->m_p_list_2, m_game_pointer->m_board, m_game_pointer->m_player_2, new king(m_game_pointer->m_player_2->get_sign(), x, y, true, m_game_pointer->m_player_2));
						m_game_pointer->m_any_changes = true;
					}
					break;
				}
				else if (event.key.code == sf::Keyboard::U)
				{
					// add new piece to the first player
					std::pair<int, int> coords = m_game_pointer->get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_game_pointer->m_board)[x][y] == nullptr)
					{
						log << "Manually adding piece to the first player at coordinates: x: " << x << "; " << y << std::endl;
						m_game_pointer->add_new_piece(&m_game_pointer->m_p_list_1, m_game_pointer->m_board, m_game_pointer->m_player_1, x, y, true);
						m_game_pointer->m_any_changes = true;
					}
					break;
				}
				else if (event.key.code == sf::Keyboard::I)
				{
					// add new piece to the second player
					std::pair<int, int> coords = m_game_pointer->get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_game_pointer->m_board)[x][y] == nullptr)
					{
						log << "Manually adding piece to the second player at coordinates: x: " << x << "; " << y << std::endl;
						m_game_pointer->add_new_piece(&m_game_pointer->m_p_list_2, m_game_pointer->m_board, m_game_pointer->m_player_2, x, y, true);
						m_game_pointer->m_any_changes = true;
					}
					break;
				}
				else if (event.key.code == sf::Keyboard::D)
				{
					// add new piece to the second player
					std::pair<int, int> coords = m_game_pointer->get_click_coordinates();
					int x = std::get<0>(coords);
					int y = std::get<1>(coords);
					std::cout << "x: " << x << "; y: " << y << std::endl;
					if (!(x < 0 || x > s_size - 1 || y < 0 || y > s_size - 1) && (x % 2 == 0 && y % 2 != 0 || x % 2 != 0 && y % 2 == 0) && (*m_game_pointer->m_board)[x][y] != nullptr)
					{
						piece* p = (*m_game_pointer->m_board)[x][y];
						if (p)
						{
							log << "Manually deleting piece at coordinates: x: " << x << "; " << y << std::endl;
							m_game_pointer->delete_piece(p, m_game_pointer->m_board, p->get_owner());
							m_game_pointer->m_any_changes = true;
						}
					}
					break;
				}
				else if (event.key.code == sf::Keyboard::L)
				{
					log << "Manually populating the board with normal setup" << std::endl;
					m_game_pointer->populate_board(s_size / 2 - 1);
					m_game_pointer->m_any_changes = true;
					break;
				}
				else if (event.key.code == sf::Keyboard::F)
				{
					// load pieces from file
					log << "Manually loading gamestate from file" << std::endl;
					m_game_pointer->load_pieces_from_file("pieces.txt");
					m_game_pointer->m_any_changes = true;
					break;
				}
				else if (event.key.code == sf::Keyboard::J)
				{
					// save pieces to file
					log << "Manually saving gamestate to file" << std::endl;
					m_game_pointer->save_to_file("pieces.txt");
					break;
				}
				else if (event.key.code == sf::Keyboard::S)
				{
					log << "Manually switching game turn" << std::endl;
					m_game_pointer->switch_turn();
					break;
				}
				else if (event.key.code == sf::Keyboard::R)
				{
					// refresh the board
					os << "Refreshing the board." << std::endl;
					m_game_pointer->m_any_changes = true;
					break;
				}
				else if (event.key.code == sf::Keyboard::X)
				{
					os << "Frozen the game." << std::endl;
					m_game_pointer->m_game_freeze = true;
					break;
				}
				else if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift)
				{
					break;
				}
				else
				{
					os << "Key code: \'" << event.key.code << "\' is not programmed." << std::endl;
					break;
				}
			}
#endif
			if (m_game_pointer->m_first_won || m_game_pointer->m_second_won)
			{
				os << "Game is finished. Click Escape to display the results" << std::endl;
				m_game_pointer->m_selected = false;
				m_game_pointer->m_selected_piece = nullptr;
				m_game_pointer->m_game_freeze = true;
				m_game_pointer->m_gui->draw_board(*m_game_pointer->m_player_1->get_list(), *m_game_pointer->m_player_2->get_list(), m_game_pointer->m_to_delete_list, m_game_pointer->m_selected_piece);
				break;
			}
		}
	}
}