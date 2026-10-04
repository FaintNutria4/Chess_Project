#include "interop/convert.hpp"

#include <cstdint>

namespace chess::interop {

chess::Square to_square(const godot::Vector2i &coords) {
	return chess::Square{ int8_t(coords.x), int8_t(coords.y) };
}

godot::Vector2i from_square(const chess::Square &square) {
	return godot::Vector2i(square.file, square.rank);
}

godot::String color_name(chess::Color color) {
	return color == chess::Color::White ? "white" : "black";
}

godot::String piece_type_name(chess::PieceType type) {
	switch (type) {
		case chess::PieceType::Pawn:
			return "pawn";
		case chess::PieceType::Knight:
			return "knight";
		case chess::PieceType::Bishop:
			return "bishop";
		case chess::PieceType::Rook:
			return "rook";
		case chess::PieceType::Queen:
			return "queen";
		case chess::PieceType::King:
			return "king";
	}
	return "pawn";
}

chess::PieceType piece_type_from_name(const godot::String &name) {
	if (name == "rook") {
		return chess::PieceType::Rook;
	}
	if (name == "bishop") {
		return chess::PieceType::Bishop;
	}
	if (name == "knight") {
		return chess::PieceType::Knight;
	}
	return chess::PieceType::Queen;
}

godot::Dictionary move_to_dict(const chess::Move &move) {
	godot::Dictionary out;
	out["from"] = from_square(move.from);
	out["to"] = from_square(move.to);
	out["promotion"] = move.is_promotion;
	return out;
}

godot::Dictionary result_to_dict(const chess::MoveResult &result, const godot::String &fen, chess::Color next_turn) {
	godot::Dictionary out;
	out["legal"] = result.legal;
	out["captured"] = result.captured;
	out["en_passant"] = result.en_passant;
	out["castled"] = result.castled;
	out["promoted"] = result.promoted;
	out["check"] = result.check;
	out["checkmate"] = result.checkmate;
	out["stalemate"] = result.stalemate;
	out["fen"] = fen;
	out["turn"] = color_name(next_turn);
	return out;
}

} // namespace chess::interop
