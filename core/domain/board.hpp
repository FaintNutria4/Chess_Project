#ifndef CHESS_BOARD_HPP
#define CHESS_BOARD_HPP

#include <array>
#include <cstdint>
#include <optional>

#include "types.hpp"

namespace chess {

class Board {
public:
	void clear();
	void set_piece(const Square &square, const Piece &piece);
	void remove_piece(const Square &square);
	std::optional<Piece> at(const Square &square) const;

private:
	std::array<std::optional<Piece>, 64> cells_{};
};

} // namespace chess

#endif // CHESS_BOARD_HPP
