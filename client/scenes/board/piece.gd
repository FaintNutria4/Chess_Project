extends Node2D

const TEXTURES := {
	"white-pawn": preload("res://assets/sprites/white-pawn.png"),
	"white-rook": preload("res://assets/sprites/white-rook.png"),
	"white-knight": preload("res://assets/sprites/white-knight.png"),
	"white-bishop": preload("res://assets/sprites/white-bishop.png"),
	"white-queen": preload("res://assets/sprites/white-queen.png"),
	"white-king": preload("res://assets/sprites/white-king.png"),
	"black-pawn": preload("res://assets/sprites/black-pawn.png"),
	"black-rook": preload("res://assets/sprites/black-rook.png"),
	"black-knight": preload("res://assets/sprites/black-knight.png"),
	"black-bishop": preload("res://assets/sprites/black-bishop.png"),
	"black-queen": preload("res://assets/sprites/black-queen.png"),
	"black-king": preload("res://assets/sprites/black-king.png"),
}


var piece_color: String = "white"
var piece_type: String = "pawn"


func setup(color_name: String, type_name: String) -> void:
	piece_color = color_name
	piece_type = type_name
	$Visual.texture = TEXTURES.get(color_name + "-" + type_name)
