#include "chess_game.h"

#include "interop/convert.hpp"

namespace godot {

void ChessGame::_bind_methods() {
	ClassDB::bind_method(D_METHOD("reset"), &ChessGame::reset);
	ClassDB::bind_method(D_METHOD("get_fen"), &ChessGame::get_fen);
	ClassDB::bind_method(D_METHOD("load_fen", "fen"), &ChessGame::load_fen);
	ClassDB::bind_method(D_METHOD("try_move", "from", "to", "promotion"), &ChessGame::try_move);
	ClassDB::bind_method(D_METHOD("legal_moves_for", "square"), &ChessGame::legal_moves_for);
	ClassDB::bind_method(D_METHOD("select_square", "square"), &ChessGame::select_square);
	ClassDB::bind_method(D_METHOD("get_pieces"), &ChessGame::get_pieces);
	ClassDB::bind_method(D_METHOD("get_turn"), &ChessGame::get_turn);
	ClassDB::bind_method(D_METHOD("is_in_check"), &ChessGame::is_in_check);

	ClassDB::bind_method(D_METHOD("hello"), &ChessGame::hello);

	ClassDB::bind_method(D_METHOD("set_player_name", "player_name"), &ChessGame::set_player_name);
	ClassDB::bind_method(D_METHOD("get_player_name"), &ChessGame::get_player_name);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "player_name"), "set_player_name", "get_player_name");
}

void ChessGame::reset() {
	state.reset();
	has_selected = false;
}

String ChessGame::get_fen() const {
	return String(state.to_fen().c_str());
}

bool ChessGame::load_fen(const String &fen) {
	bool ok = state.load_fen(std::string(fen.utf8().get_data()));
	if (ok) {
		has_selected = false;
	}
	return ok;
}

Dictionary ChessGame::try_move(const Vector2i &from, const Vector2i &to, const String &promotion) {
	chess::Move move;
	move.from = chess::interop::to_square(from);
	move.to = chess::interop::to_square(to);
	move.is_promotion = true;
	move.promotion = chess::interop::piece_type_from_name(promotion);
	chess::MoveResult result = state.try_move(move);
	return chess::interop::result_to_dict(result, String(state.to_fen().c_str()), state.turn());
}

Array ChessGame::legal_moves_for(const Vector2i &square) const {
	Array out;
	chess::Square sq = chess::interop::to_square(square);
	if (!sq.valid()) {
		return out;
	}
	for (const chess::Move &move : state.legal_moves_from(sq)) {
		out.push_back(chess::interop::from_square(move.to));
	}
	return out;
}

Dictionary ChessGame::select_square(const Vector2i &coords) {
	Dictionary response;
	response["selected"] = false;
	response["moved"] = false;
	response["moves"] = Array();
	response["result"] = Dictionary();

	chess::Square sq = chess::interop::to_square(coords);
	if (!sq.valid()) {
		return response;
	}

	if (has_selected) {
		chess::Square from = selected;
		has_selected = false;
		if (!(sq == from)) {
			bool destination_reachable = false;
			for (const chess::Move &move : state.legal_moves_from(from)) {
				if (move.to == sq) {
					destination_reachable = true;
					break;
				}
			}
			if (destination_reachable) {
				chess::Move move;
				move.from = from;
				move.to = sq;
				chess::MoveResult result = state.try_move(move);
				response["moved"] = true;
				response["result"] = chess::interop::result_to_dict(
						result, String(state.to_fen().c_str()), state.turn());
				return response;
			}
		}
	}

	auto piece = state.board().at(sq);
	if (piece && piece->color == state.turn()) {
		selected = sq;
		has_selected = true;
		response["selected"] = true;
		Array moves;
		for (const chess::Move &move : state.legal_moves_from(sq)) {
			moves.push_back(chess::interop::from_square(move.to));
		}
		response["moves"] = moves;
	}
	return response;
}

Array ChessGame::get_pieces() const {
	Array out;
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			chess::Square sq{ int8_t(file), int8_t(rank) };
			auto piece = state.board().at(sq);
			if (!piece) {
				continue;
			}
			Dictionary entry;
			entry["square"] = chess::interop::from_square(sq);
			entry["color"] = chess::interop::color_name(piece->color);
			entry["type"] = chess::interop::piece_type_name(piece->type);
			out.push_back(entry);
		}
	}
	return out;
}

String ChessGame::get_turn() const {
	return chess::interop::color_name(state.turn());
}

bool ChessGame::is_in_check() const {
	return state.is_in_check(state.turn());
}

String ChessGame::hello() const {
	return String("Hello from C++! Player: ") + player_name;
}

void ChessGame::set_player_name(const String p_name) {
	player_name = p_name;
}

String ChessGame::get_player_name() const {
	return player_name;
}

ChessGame::ChessGame() {
}

ChessGame::~ChessGame() {
}

} // namespace godot
