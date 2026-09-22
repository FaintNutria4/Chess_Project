#include "game_state.hpp"

#include <cstdlib>

#include "fen.hpp"
#include "rules.hpp"

namespace chess {

void GameState::reset() {
	load_fen(START_FEN);
}

bool GameState::is_in_check(Color color) const {
	std::optional<Square> king = rules::find_king(board_, color);
	return king && rules::square_attacked(board_, *king, opposite(color));
}

std::vector<Move> GameState::legal_moves_from(const Square &from) const {
	return rules::legal_moves(*this, from);
}

StateSnapshot GameState::snapshot() const {
	return StateSnapshot{ board_, turn_, rights_, ep_, halfmove_, fullmove_ };
}

void GameState::restore(const StateSnapshot &snapshot) {
	board_ = snapshot.board;
	turn_ = snapshot.turn;
	rights_ = snapshot.rights;
	ep_ = snapshot.en_passant;
	halfmove_ = snapshot.halfmove_clock;
	fullmove_ = snapshot.fullmove_number;
}

bool GameState::undo() {
	if (history_.empty()) {
		return false;
	}
	restore(history_.back());
	history_.pop_back();
	return true;
}

MoveResult GameState::try_move(const Move &move) {
	MoveResult result;
	if (!move.from.valid() || !move.to.valid() || move.from == move.to) {
		return result;
	}
	auto mover_opt = board_.at(move.from);
	if (!mover_opt || mover_opt->color != turn_) {
		return result;
	}

	const Move *chosen = nullptr;
	for (const Move &candidate : rules::legal_moves(*this, move.from)) {
		if (candidate.from != move.from || candidate.to != move.to) {
			continue;
		}
		if (candidate.is_promotion) {
			if (move.is_promotion && candidate.promotion == move.promotion) {
				chosen = &candidate;
				break;
			}
			if (!move.is_promotion && candidate.promotion == PieceType::Queen) {
				chosen = &candidate;
				break;
			}
		} else {
			chosen = &candidate;
			break;
		}
	}
	if (!chosen) {
		return result;
	}

	Piece mover = *mover_opt;
	auto captured = board_.at(move.to);
	bool en_passant_capture = mover.type == PieceType::Pawn && ep_ && move.to == *ep_ && !captured;

	history_.push_back(snapshot());
	rules::apply_move(board_, *chosen, ep_, mover);

	result.legal = true;
	result.captured = captured.has_value() || en_passant_capture;
	result.en_passant = en_passant_capture;
	result.castled = mover.type == PieceType::King && std::abs(int(move.to.file) - int(move.from.file)) == 2;
	result.promoted = chosen->is_promotion;

	ep_ = std::nullopt;
	if (mover.type == PieceType::Pawn && std::abs(int(move.to.rank) - int(move.from.rank)) == 2) {
		ep_ = Square{ move.from.file, int8_t((int(move.from.rank) + int(move.to.rank)) / 2) };
	}

	if (mover.type == PieceType::King) {
		if (turn_ == Color::White) {
			rights_.white_king_side = false;
			rights_.white_queen_side = false;
		} else {
			rights_.black_king_side = false;
			rights_.black_queen_side = false;
		}
	}
	auto clear_corner = [&](const Square &square) {
		if (square == Square{ 0, 0 }) {
			rights_.white_queen_side = false;
		}
		if (square == Square{ 7, 0 }) {
			rights_.white_king_side = false;
		}
		if (square == Square{ 0, 7 }) {
			rights_.black_queen_side = false;
		}
		if (square == Square{ 7, 7 }) {
			rights_.black_king_side = false;
		}
	};
	clear_corner(move.from);
	clear_corner(move.to);

	halfmove_ = (mover.type == PieceType::Pawn || result.captured) ? 0 : halfmove_ + 1;
	if (turn_ == Color::Black) {
		fullmove_++;
	}
	turn_ = opposite(turn_);

	result.check = is_in_check(turn_);
	bool opponent_has_moves = rules::has_legal_move(*this, turn_);
	result.checkmate = result.check && !opponent_has_moves;
	result.stalemate = !result.check && !opponent_has_moves;
	return result;
}

} // namespace chess
