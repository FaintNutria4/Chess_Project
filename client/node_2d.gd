extends Node2D


func _ready() -> void:
	var g = ChessGame.new()
	g.player_name = "Alvaro"
	print(g.hello())
