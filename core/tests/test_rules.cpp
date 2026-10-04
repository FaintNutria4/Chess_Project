#include <iostream>
#include <string>
#include <vector>

#include "domain/board.hpp"
#include "domain/fen.hpp"
#include "domain/game_state.hpp"
#include "domain/rules.hpp"
#include "domain/types.hpp"

static int checks = 0;
static int failures = 0;

#define CHECK(cond) \
	do { \
		++checks; \
		if (!(cond)) { \
			++failures; \
			std::cout << "FAIL line " << __LINE__ << ": " #cond "\n"; \
		} \
	} while (0)

using namespace chess;

static bool has_move(const std::vector<Move> &moves, int from_file, int from_rank, int to_file, int to_rank) {
	Move wanted{ Square{ int8_t(from_file), int8_t(from_rank) }, Square{ int8_t(to_file), int8_t(to_rank) } };
	for (const Move &move : moves) {
		if (move.from == wanted.from && move.to == wanted.to) {
			return true;
		}
	}
	return false;
}

int main() {
	{
		GameState state;
		CHECK(rules::all_legal_moves(state, Color::White).size() == 20);
	}
	{
		GameState state;
		MoveResult result = state.try_move(Move{ Square{ 4, 1 }, Square{ 4, 3 } });
		CHECK(result.legal);
		CHECK(state.to_fen() == "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
		CHECK((state.en_passant_target() == Square{ 4, 2 }));
		CHECK(rules::all_legal_moves(state, Color::Black).size() == 20);
	}
	{
		GameState state;
		MoveResult result = state.try_move(Move{ Square{ 4, 1 }, Square{ 4, 6 } });
		CHECK(!result.legal);
		CHECK(state.to_fen() == START_FEN);
	}
	{
		GameState state;
		CHECK(state.load_fen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1"));
		std::vector<Move> moves = state.legal_moves_from(Square{ 4, 4 });
		CHECK(has_move(moves, 4, 4, 3, 5));
		MoveResult result = state.try_move(Move{ Square{ 4, 4 }, Square{ 3, 5 } });
		CHECK(result.legal && result.en_passant && result.captured);
		CHECK((!state.board().at(Square{ 3, 4 }).has_value()));
	}
	{
		GameState state;
		CHECK(state.load_fen("4k3/8/8/8/8/8/8/4K2R w K - 0 1"));
		std::vector<Move> moves = state.legal_moves_from(Square{ 4, 0 });
		CHECK(has_move(moves, 4, 0, 6, 0));
		MoveResult result = state.try_move(Move{ Square{ 4, 0 }, Square{ 6, 0 } });
		CHECK(result.legal && result.castled);
		CHECK((state.board().at(Square{ 5, 0 })->type == PieceType::Rook));
		CHECK((state.board().at(Square{ 6, 0 })->type == PieceType::King));
	}
	{
		GameState state;
		CHECK(state.load_fen("4k3/8/8/8/1b6/8/3N4/4K3 w - - 0 1"));
		CHECK(state.legal_moves_from(Square{ 3, 1 }).empty());
	}
	{
		GameState state;
		CHECK(state.load_fen("rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3"));
		CHECK(state.is_in_check(Color::White));
		CHECK(!rules::has_legal_move(state, Color::White));
	}
	{
		GameState state;
		CHECK(state.load_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1"));
		CHECK(!state.is_in_check(Color::Black));
		CHECK(!rules::has_legal_move(state, Color::Black));
	}
	{
		GameState state;
		CHECK((state.try_move(Move{ Square{ 4, 1 }, Square{ 4, 3 } }).legal));
		CHECK((state.try_move(Move{ Square{ 4, 6 }, Square{ 4, 4 } }).legal));
		CHECK(state.undo());
		CHECK(state.to_fen() == "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
	}
	{
		GameState state;
		std::string fen = "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4";
		CHECK(state.load_fen(fen));
		CHECK(state.to_fen() == fen);
	}
	{
		GameState state;
		std::string fen = "4k3/P7/8/8/8/8/8/4K3 w - - 0 1";
		CHECK(state.load_fen(fen));
		CHECK(state.to_fen() == fen);
		CHECK(!state.castling_rights().white_king_side);
		CHECK(!state.castling_rights().white_queen_side);
		CHECK(!state.castling_rights().black_king_side);
		CHECK(!state.castling_rights().black_queen_side);
	}
	{
		GameState state;
		CHECK(state.load_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"));
		std::vector<Move> moves = state.legal_moves_from(Square{ 0, 6 });
		int promotions = 0;
		bool saw_queen = false;
		bool saw_rook = false;
		bool saw_bishop = false;
		bool saw_knight = false;
		for (const Move &move : moves) {
			if (move.from == Square{ 0, 6 } && move.to == Square{ 0, 7 } && move.is_promotion) {
				++promotions;
				saw_queen = saw_queen || move.promotion == PieceType::Queen;
				saw_rook = saw_rook || move.promotion == PieceType::Rook;
				saw_bishop = saw_bishop || move.promotion == PieceType::Bishop;
				saw_knight = saw_knight || move.promotion == PieceType::Knight;
			}
		}
		CHECK(promotions == 4);
		CHECK(saw_queen && saw_rook && saw_bishop && saw_knight);
	}
	{
		GameState state;
		CHECK(state.load_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"));
		Move promotion;
		promotion.from = Square{ 0, 6 };
		promotion.to = Square{ 0, 7 };
		promotion.is_promotion = true;
		promotion.promotion = PieceType::Bishop;
		MoveResult result = state.try_move(promotion);
		CHECK(result.legal && result.promoted);
		CHECK((state.board().at(Square{ 0, 7 })->type == PieceType::Bishop));
	}
	{
		GameState state;
		CHECK(state.load_fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1"));
		MoveResult result = state.try_move(Move{ Square{ 0, 6 }, Square{ 0, 7 } });
		CHECK(result.legal && result.promoted);
		CHECK((state.board().at(Square{ 0, 7 })->type == PieceType::Queen));
	}

	std::cout << checks << " checks, " << failures << " failures\n";
	return failures == 0 ? 0 : 1;
}
