#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "domain/game_state.hpp"

namespace godot {

class ChessGame : public RefCounted {
	GDCLASS(ChessGame, RefCounted)

private:
	chess::GameState state;
	chess::Square selected;
	bool has_selected = false;
	String player_name;

protected:
	static void _bind_methods();

public:
	void reset();
	String get_fen() const;
	bool load_fen(const String &fen);
	Dictionary try_move(const Vector2i &from, const Vector2i &to, const String &promotion);
	Array legal_moves_for(const Vector2i &square) const;
	Dictionary select_square(const Vector2i &square);
	Array get_pieces() const;
	String get_turn() const;
	bool is_in_check() const;

	String hello() const;

	void set_player_name(const String p_name);
	String get_player_name() const;

	ChessGame();
	~ChessGame();
};

} // namespace godot

#endif // CHESS_GAME_H
