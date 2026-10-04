#ifndef CHESS_GAME_STATE_HPP
#define CHESS_GAME_STATE_HPP

#include <optional>
#include <string>
#include <vector>

#include "board.hpp"
#include "types.hpp"

namespace chess {

struct CastlingRights {
	bool white_king_side = false;
	bool white_queen_side = false;
	bool black_king_side = false;
	bool black_queen_side = false;
};

struct StateSnapshot {
	Board board;
	Color turn = Color::White;
	CastlingRights rights;
	std::optional<Square> en_passant;
	int halfmove_clock = 0;
	int fullmove_number = 1;
};

class GameState {
public:
	GameState() {
		reset();
	}

	void reset();

	const Board &board() const { return board_; }
	Color turn() const { return turn_; }
	const CastlingRights &castling_rights() const { return rights_; }
	const std::optional<Square> &en_passant_target() const { return ep_; }

	bool is_in_check(Color color) const;
	std::vector<Move> legal_moves_from(const Square &from) const;
	MoveResult try_move(const Move &move);
	bool undo();

	std::string to_fen() const;
	bool load_fen(const std::string &fen);

private:
	StateSnapshot snapshot() const;
	void restore(const StateSnapshot &snapshot);

	Board board_;
	Color turn_ = Color::White;
	CastlingRights rights_;
	std::optional<Square> ep_;
	int halfmove_ = 0;
	int fullmove_ = 1;
	std::vector<StateSnapshot> history_;
};

} // namespace chess

#endif // CHESS_GAME_STATE_HPP
