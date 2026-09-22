#ifndef CHESS_TYPES_HPP
#define CHESS_TYPES_HPP

#include <cstdint>

namespace chess {

enum class Color : uint8_t { White = 0, Black = 1 };

enum class PieceType : uint8_t { Pawn, Knight, Bishop, Rook, Queen, King };

inline Color opposite(Color color) {
	return color == Color::White ? Color::Black : Color::White;
}

struct Piece {
	Color color = Color::White;
	PieceType type = PieceType::Pawn;
	bool operator==(const Piece &other) const { return color == other.color && type == other.type; }
	bool operator!=(const Piece &other) const { return !(*this == other); }
};

struct Square {
	int8_t file = 0;
	int8_t rank = 0;

	bool valid() const { return file >= 0 && file < 8 && rank >= 0 && rank < 8; }
	int index() const { return rank * 8 + file; }
	static Square from_index(int index) { return Square{ int8_t(index % 8), int8_t(index / 8) }; }
	bool operator==(const Square &other) const { return file == other.file && rank == other.rank; }
	bool operator!=(const Square &other) const { return !(*this == other); }
};

struct Move {
	Square from;
	Square to;
	bool is_promotion = false;
	PieceType promotion = PieceType::Queen;
	bool operator==(const Move &other) const {
		return from == other.from && to == other.to && is_promotion == other.is_promotion &&
				promotion == other.promotion;
	}
	bool operator!=(const Move &other) const { return !(*this == other); }
};

struct MoveResult {
	bool legal = false;
	bool captured = false;
	bool en_passant = false;
	bool castled = false;
	bool promoted = false;
	bool check = false;
	bool checkmate = false;
	bool stalemate = false;
};

} // namespace chess

#endif // CHESS_TYPES_HPP
