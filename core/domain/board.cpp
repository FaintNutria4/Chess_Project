#include "board.hpp"

namespace chess {

void Board::clear() {
	cells_.fill(std::nullopt);
}

void Board::set_piece(const Square &square, const Piece &piece) {
	if (!square.valid()) {
		return;
	}
	cells_[square.index()] = piece;
}

void Board::remove_piece(const Square &square) {
	if (!square.valid()) {
		return;
	}
	cells_[square.index()] = std::nullopt;
}

std::optional<Piece> Board::at(const Square &square) const {
	if (!square.valid()) {
		return std::nullopt;
	}
	return cells_[square.index()];
}

} // namespace chess
