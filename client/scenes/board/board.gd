extends Node2D

signal move_played(result: Dictionary)

const SquareScene := preload("res://scenes/board/square.tscn")
const PieceScene := preload("res://scenes/board/piece.tscn")

const SQUARE_SIZE := 64.0
const LIGHT_COLOR := Color(0.87, 0.82, 0.7)
const DARK_COLOR := Color(0.52, 0.38, 0.24)

var chess: ChessGame
var squares := {}
var pieces := {}


func _ready() -> void:
	chess = ChessGame.new()
	_build_squares()
	_sync_pieces()


func square_position(coords: Vector2i) -> Vector2:
	return (Vector2(coords.x, 7 - coords.y) + Vector2(0.5, 0.5)) * SQUARE_SIZE


func _build_squares() -> void:
	for file in range(8):
		for rank in range(8):
			var coords := Vector2i(file, rank)
			var square := SquareScene.instantiate()
			square.coords = coords
			square.position = square_position(coords)
			square.set_base_color(LIGHT_COLOR if (file + rank) % 2 == 0 else DARK_COLOR)
			square.clicked.connect(_on_square_clicked)
			$Squares.add_child(square)
			squares[coords] = square


func _on_square_clicked(square: Area2D) -> void:
	var response: Dictionary = chess.select_square(square.coords)
	_clear_highlights()
	if response.get("moved", false):
		_sync_pieces()
		move_played.emit(response["result"])
	elif response.get("selected", false):
		var moves: Array = response["moves"]
		for coords in moves:
			if squares.has(coords):
				squares[coords].set_highlight(true)


func _clear_highlights() -> void:
	for square in squares.values():
		square.set_highlight(false)


func _sync_pieces() -> void:
	for piece in $Pieces.get_children():
		piece.queue_free()
	pieces.clear()
	for entry in chess.get_pieces():
		var piece := PieceScene.instantiate()
		piece.setup(entry["color"], entry["type"])
		piece.position = square_position(entry["square"])
		$Pieces.add_child(piece)
		pieces[entry["square"]] = piece
