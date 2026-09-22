#include "rules.hpp"

#include <cstdlib>

#include "game_state.hpp"

namespace chess::rules {

namespace {

constexpr int8_t KNIGHT_OFFSETS[8][2] = {
	{ 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 }, { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 }
};
constexpr int8_t KING_OFFSETS[8][2] = {
	{ 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
};
constexpr int8_t BISHOP_DIRS[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };
constexpr int8_t ROOK_DIRS[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

bool in_bounds(int file, int rank) {
	return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

bool is_enemy(const Board &board, const Square &square, Color my_color) {
	auto piece = board.at(square);
	return piece && piece->color != my_color;
}

void add_step_moves(std::vector<Move> &out, const Board &board, const Square &from, const int8_t (&offsets)[8][2]) {
	Piece mover = *board.at(from);
	for (const auto &offset : offsets) {
		int file = from.file + offset[0];
		int rank = from.rank + offset[1];
		if (!in_bounds(file, rank)) {
			continue;
		}
		Square to{ int8_t(file), int8_t(rank) };
		if (!board.at(to) || is_enemy(board, to, mover.color)) {
			out.push_back(Move{ from, to });
		}
	}
}

void add_ray_moves(std::vector<Move> &out, const Board &board, const Square &from, const int8_t (&dirs)[4][2]) {
	Piece mover = *board.at(from);
	for (const auto &dir : dirs) {
		int file = from.file + dir[0];
		int rank = from.rank + dir[1];
		while (in_bounds(file, rank)) {
			Square to{ int8_t(file), int8_t(rank) };
			auto target = board.at(to);
			if (!target) {
				out.push_back(Move{ from, to });
			} else {
				if (target->color != mover.color) {
					out.push_back(Move{ from, to });
				}
				break;
			}
			file += dir[0];
			rank += dir[1];
		}
	}
}

void add_promotion_moves(std::vector<Move> &out, const Square &from, const Square &to) {
	const PieceType types[4] = { PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight };
	for (PieceType type : types) {
		Move move{};
		move.from = from;
		move.to = to;
		move.is_promotion = true;
		move.promotion = type;
		out.push_back(move);
	}
}

void add_pawn_moves(std::vector<Move> &out, const GameState &state, const Square &from, const Piece &mover) {
	const Board &board = state.board();
	int dir = mover.color == Color::White ? 1 : -1;
	int start_rank = mover.color == Color::White ? 1 : 6;
	int promotion_rank = mover.color == Color::White ? 7 : 0;

	Square one{ from.file, int8_t(from.rank + dir) };
	if (one.valid() && !board.at(one)) {
		if (one.rank == promotion_rank) {
			add_promotion_moves(out, from, one);
		} else {
			out.push_back(Move{ from, one });
			if (from.rank == start_rank) {
				Square two{ from.file, int8_t(from.rank + 2 * dir) };
				if (two.valid() && !board.at(two)) {
					out.push_back(Move{ from, two });
				}
			}
		}
	}
	for (int df = -1; df <= 1; df += 2) {
		Square to{ int8_t(from.file + df), int8_t(from.rank + dir) };
		if (!to.valid()) {
			continue;
		}
		auto target = board.at(to);
		if (target && target->color != mover.color) {
			if (to.rank == promotion_rank) {
				add_promotion_moves(out, from, to);
			} else {
				out.push_back(Move{ from, to });
			}
		} else if (!target && state.en_passant_target() && to == *state.en_passant_target()) {
			out.push_back(Move{ from, to });
		}
	}
}

void add_castling_moves(std::vector<Move> &out, const GameState &state, const Square &from, const Piece &mover) {
	const Board &board = state.board();
	int8_t home_rank = mover.color == Color::White ? 0 : 7;
	if (from.file != 4 || from.rank != home_rank) {
		return;
	}
	const CastlingRights &rights = state.castling_rights();
	Color enemy = opposite(mover.color);
	Square king_square{ 4, home_rank };
	bool king_side = mover.color == Color::White ? rights.white_king_side : rights.black_king_side;
	bool queen_side = mover.color == Color::White ? rights.white_queen_side : rights.black_queen_side;

	if (king_side) {
		Square f{ 5, home_rank };
		Square g{ 6, home_rank };
		Square h{ 7, home_rank };
		auto rook = board.at(h);
		if (rook && rook->type == PieceType::Rook && rook->color == mover.color && !board.at(f) && !board.at(g) &&
				!square_attacked(board, king_square, enemy) && !square_attacked(board, f, enemy) &&
				!square_attacked(board, g, enemy)) {
			out.push_back(Move{ from, g });
		}
	}
	if (queen_side) {
		Square b{ 1, home_rank };
		Square c{ 2, home_rank };
		Square d{ 3, home_rank };
		Square a{ 0, home_rank };
		auto rook = board.at(a);
		if (rook && rook->type == PieceType::Rook && rook->color == mover.color && !board.at(b) && !board.at(c) &&
				!board.at(d) && !square_attacked(board, king_square, enemy) && !square_attacked(board, c, enemy) &&
				!square_attacked(board, d, enemy)) {
			out.push_back(Move{ from, c });
		}
	}
}

} // namespace

std::vector<Move> pseudo_moves_from(const GameState &state, const Square &from) {
	std::vector<Move> out;
	const Board &board = state.board();
	auto mover_opt = board.at(from);
	if (!mover_opt) {
		return out;
	}
	Piece mover = *mover_opt;

	switch (mover.type) {
		case PieceType::Pawn:
			add_pawn_moves(out, state, from, mover);
			break;
		case PieceType::Knight:
			add_step_moves(out, board, from, KNIGHT_OFFSETS);
			break;
		case PieceType::Bishop:
			add_ray_moves(out, board, from, BISHOP_DIRS);
			break;
		case PieceType::Rook:
			add_ray_moves(out, board, from, ROOK_DIRS);
			break;
		case PieceType::Queen:
			add_ray_moves(out, board, from, BISHOP_DIRS);
			add_ray_moves(out, board, from, ROOK_DIRS);
			break;
		case PieceType::King:
			add_step_moves(out, board, from, KING_OFFSETS);
			add_castling_moves(out, state, from, mover);
			break;
	}
	return out;
}

bool square_attacked(const Board &board, const Square &target, Color by) {
	int8_t pawn_rank = by == Color::White ? int8_t(target.rank - 1) : int8_t(target.rank + 1);
	for (int df = -1; df <= 1; df += 2) {
		Square square{ int8_t(target.file + df), pawn_rank };
		if (square.valid()) {
			auto piece = board.at(square);
			if (piece && piece->color == by && piece->type == PieceType::Pawn) {
				return true;
			}
		}
	}
	for (const auto &offset : KNIGHT_OFFSETS) {
		Square square{ int8_t(target.file + offset[0]), int8_t(target.rank + offset[1]) };
		if (square.valid()) {
			auto piece = board.at(square);
			if (piece && piece->color == by && piece->type == PieceType::Knight) {
				return true;
			}
		}
	}
	for (const auto &offset : KING_OFFSETS) {
		Square square{ int8_t(target.file + offset[0]), int8_t(target.rank + offset[1]) };
		if (square.valid()) {
			auto piece = board.at(square);
			if (piece && piece->color == by && piece->type == PieceType::King) {
				return true;
			}
		}
	}
	for (const auto &dir : BISHOP_DIRS) {
		int file = target.file + dir[0];
		int rank = target.rank + dir[1];
		while (in_bounds(file, rank)) {
			auto piece = board.at(Square{ int8_t(file), int8_t(rank) });
			if (piece) {
				if (piece->color == by && (piece->type == PieceType::Bishop || piece->type == PieceType::Queen)) {
					return true;
				}
				break;
			}
			file += dir[0];
			rank += dir[1];
		}
	}
	for (const auto &dir : ROOK_DIRS) {
		int file = target.file + dir[0];
		int rank = target.rank + dir[1];
		while (in_bounds(file, rank)) {
			auto piece = board.at(Square{ int8_t(file), int8_t(rank) });
			if (piece) {
				if (piece->color == by && (piece->type == PieceType::Rook || piece->type == PieceType::Queen)) {
					return true;
				}
				break;
			}
			file += dir[0];
			rank += dir[1];
		}
	}
	return false;
}

std::optional<Square> find_king(const Board &board, Color color) {
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			Square square{ int8_t(file), int8_t(rank) };
			auto piece = board.at(square);
			if (piece && piece->color == color && piece->type == PieceType::King) {
				return square;
			}
		}
	}
	return std::nullopt;
}

void apply_move(Board &board, const Move &move, const std::optional<Square> &en_passant, const Piece &mover) {
	board.remove_piece(move.from);
	if (mover.type == PieceType::King && std::abs(int(move.to.file) - int(move.from.file)) == 2) {
		int8_t rank = move.from.rank;
		if (move.to.file == 6) {
			auto rook = board.at(Square{ 7, rank });
			if (rook) {
				board.remove_piece(Square{ 7, rank });
				board.set_piece(Square{ 5, rank }, *rook);
			}
		} else {
			auto rook = board.at(Square{ 0, rank });
			if (rook) {
				board.remove_piece(Square{ 0, rank });
				board.set_piece(Square{ 3, rank }, *rook);
			}
		}
	}
	if (mover.type == PieceType::Pawn && en_passant && move.to == *en_passant && !board.at(move.to)) {
		board.remove_piece(Square{ move.to.file, move.from.rank });
	}
	Piece placed = mover;
	if (move.is_promotion) {
		placed.type = move.promotion;
	}
	board.set_piece(move.to, placed);
}

std::vector<Move> legal_moves(const GameState &state, const Square &from) {
	std::vector<Move> out;
	const Board &board = state.board();
	auto mover_opt = board.at(from);
	if (!mover_opt) {
		return out;
	}
	Color my_color = mover_opt->color;
	Color enemy = opposite(my_color);
	for (const Move &move : pseudo_moves_from(state, from)) {
		Board copy = board;
		apply_move(copy, move, state.en_passant_target(), *mover_opt);
		std::optional<Square> king = mover_opt->type == PieceType::King
				? std::optional<Square>(move.to)
				: find_king(copy, my_color);
		if (!king || !square_attacked(copy, *king, enemy)) {
			out.push_back(move);
		}
	}
	return out;
}

std::vector<Move> all_legal_moves(const GameState &state, Color color) {
	std::vector<Move> out;
	const Board &board = state.board();
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			Square square{ int8_t(file), int8_t(rank) };
			auto piece = board.at(square);
			if (piece && piece->color == color) {
				std::vector<Move> moves = legal_moves(state, square);
				out.insert(out.end(), moves.begin(), moves.end());
			}
		}
	}
	return out;
}

bool has_legal_move(const GameState &state, Color color) {
	const Board &board = state.board();
	for (int rank = 0; rank < 8; ++rank) {
		for (int file = 0; file < 8; ++file) {
			Square square{ int8_t(file), int8_t(rank) };
			auto piece = board.at(square);
			if (piece && piece->color == color && !legal_moves(state, square).empty()) {
				return true;
			}
		}
	}
	return false;
}

} // namespace chess::rules
