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
var pending_move := {}


func _ready() -> void:
	chess = ChessGame.new()
	$Promotion.piece_chosen.connect(_on_promotion_chosen)
	_build_squares()
	_sync_pieces()


func square_position(coords: Vector2i) -> Vector2:
	return (Vector2(coords.x, 7 - coords.y) + Vector2(0.5, 0.5)) * SQUARE_SIZE


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_cancel"):
		_cancel_pending()
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_RIGHT:
		_cancel_pending()


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
	if not pending_move.is_empty():
		if square.coords == pending_move["from"] or square.coords == pending_move["to"]:
			return
		_cancel_pending()
	var response: Dictionary = chess.select_square(square.coords)
	_clear_highlights()
	var move: Dictionary = response.get("move", {})
	if not move.is_empty():
		if move["promotion"]:
			pending_move = move
			$Promotion.open(chess.get_turn(), square_position(move["to"]))
		else:
			_submit_move(move, "")
	elif response.get("selected", false):
		for entry in response["moves"]:
			var to: Vector2i = entry["to"]
			if squares.has(to):
				squares[to].set_highlight(true)


func _submit_move(move: Dictionary, piece_name: String) -> void:
	var result: Dictionary = chess.try_move(move["from"], move["to"], piece_name)
	_cancel_pending()
	if result["legal"]:
		_sync_pieces()
		move_played.emit(result)


func _on_promotion_chosen(piece_name: String) -> void:
	if pending_move.is_empty():
		return
	_submit_move(pending_move, piece_name)


func _cancel_pending() -> void:
	pending_move = {}
	$Promotion.close()
	_clear_highlights()
	chess.deselect()


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
