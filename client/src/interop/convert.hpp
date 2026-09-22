#ifndef CHESS_INTEROP_CONVERT_H
#define CHESS_INTEROP_CONVERT_H

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "domain/types.hpp"

namespace chess::interop {

chess::Square to_square(const godot::Vector2i &coords);
godot::Vector2i from_square(const chess::Square &square);
godot::String color_name(chess::Color color);
godot::String piece_type_name(chess::PieceType type);
chess::PieceType piece_type_from_name(const godot::String &name);
godot::Dictionary result_to_dict(const chess::MoveResult &result, const godot::String &fen, chess::Color next_turn);

} // namespace chess::interop

#endif // CHESS_INTEROP_CONVERT_H
