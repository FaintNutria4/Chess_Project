#ifndef CHESS_RULES_HPP
#define CHESS_RULES_HPP

#include <cstdint>
#include <optional>
#include <vector>

#include "board.hpp"
#include "types.hpp"

namespace chess {

class GameState;

}

namespace chess::rules {

std::vector<Move> pseudo_moves_from(const GameState &state, const Square &from);
std::vector<Move> legal_moves(const GameState &state, const Square &from);
std::vector<Move> all_legal_moves(const GameState &state, Color color);
bool has_legal_move(const GameState &state, Color color);
bool square_attacked(const Board &board, const Square &target, Color by);
std::optional<Square> find_king(const Board &board, Color color);
void apply_move(Board &board, const Move &move, const std::optional<Square> &en_passant, const Piece &mover);

} // namespace chess::rules

#endif // CHESS_RULES_HPP
