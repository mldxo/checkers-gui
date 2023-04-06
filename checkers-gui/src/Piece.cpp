#include "Piece.h"

namespace Checkers
{
    Piece::Piece(char s, int x1, int y1) : sign(s), x(x1), y(y1), is_captured(false), is_king(false), av_list(new std::list<AvailableMove*>)
    {
        //// test
        //av_list->push_back(AvailableMove(1, 1));
    }

    Piece::~Piece() {}

    int Piece::get_x(void) { return x; }

    int Piece::get_y(void) { return y; }

    int Piece::set_x(int x1) { return x = x1; }

    int Piece::set_y(int y1) { return y = y1; }

    char Piece::get_sign(void) { return sign; }

    bool Piece::set_captured(bool t) { return is_captured = t; }

    bool Piece::get_is_captured(void) { return is_captured; }

    std::ostream& operator<<(std::ostream& os, const Piece* piece)
    {
        if (piece == NULL)
            return os << " ";
        else
            return os << piece->sign;
    }

    void Piece::draw(sf::RenderWindow& window)
    {
        float radius = 30;

        sf::CircleShape shape(radius);
        if (!is_captured)
        {
            if (sign == 'W')
                shape.setFillColor(sf::Color(217, 216, 216, 255));
            else // 'B'
                shape.setFillColor(sf::Color(26, 23, 22, 255));
        }
        else // during multicapture
        {
            if (sign == 'W')
                shape.setFillColor(sf::Color(197, 196, 196, 255));
            else // 'B'
                shape.setFillColor(sf::Color(59, 59, 59, 255));
        }
        shape.setPosition(sf::Vector2f(x * 75 + (75 - radius * 2) / 2, y * 75 + (75 - 2 * radius) / 2));
        window.draw(shape);
    }

    std::list<AvailableMove*>* Piece::get_av_list(void)
    {
        return av_list;
    }
}