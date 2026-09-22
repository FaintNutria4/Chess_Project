extends Node2D

var SHAPES = {
	"pawn": PackedVector2Array([
		Vector2(-10, -8), Vector2(0, -16), Vector2(10, -8), Vector2(13, 4),
		Vector2(7, 14), Vector2(0, 17), Vector2(-7, 14), Vector2(-13, 4),
	]),
	"knight": PackedVector2Array([
		Vector2(-11, -14), Vector2(4, -16), Vector2(12, -6), Vector2(9, 2),
		Vector2(2, 4), Vector2(6, 10), Vector2(10, 16), Vector2(-12, 16),
		Vector2(-10, 8), Vector2(-14, 0),
	]),
	"bishop": PackedVector2Array([
		Vector2(0, -18), Vector2(7, -6), Vector2(10, 6), Vector2(5, 15),
		Vector2(-5, 15), Vector2(-10, 6), Vector2(-7, -6),
	]),
	"rook": PackedVector2Array([
		Vector2(-12, -16), Vector2(-6, -10), Vector2(6, -10), Vector2(12, -16),
		Vector2(12, 16), Vector2(7, 10), Vector2(-7, 10), Vector2(-12, 16),
	]),
	"queen": PackedVector2Array([
		Vector2(0, -20), Vector2(8, -8), Vector2(15, 8), Vector2(8, 16),
		Vector2(-8, 16), Vector2(-15, 8), Vector2(-8, -8),
	]),
	"king": PackedVector2Array([
		Vector2(-3, -22), Vector2(3, -22), Vector2(3, -16), Vector2(12, -14),
		Vector2(5, -5), Vector2(13, 8), Vector2(7, 16), Vector2(-7, 16),
		Vector2(-13, 8), Vector2(-5, -5), Vector2(-12, -14), Vector2(-3, -16),
	]),
}

var piece_color: String = "white"
var piece_type: String = "pawn"


func setup(color_name: String, type_name: String) -> void:
	piece_color = color_name
	piece_type = type_name
	$Visual.polygon = SHAPES.get(type_name, SHAPES["pawn"])
	$Visual.color = Color(0.95, 0.95, 0.9) if color_name == "white" else Color(0.15, 0.15, 0.18)
