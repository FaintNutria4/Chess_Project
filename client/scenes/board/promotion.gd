extends CanvasLayer

signal piece_chosen(piece_name: String)

const PieceScript := preload("res://scenes/board/piece.gd")
const OPTIONS := ["queen", "rook", "bishop", "knight"]
const ICON_SIZE := 64

var _icons := {}


func _ready() -> void:
	for color in ["white", "black"]:
		for option in OPTIONS:
			var image: Image = PieceScript.TEXTURES[color + "-" + option].get_image()
			image.resize(ICON_SIZE, ICON_SIZE, Image.INTERPOLATE_LANCZOS)
			_icons[color + "-" + option] = ImageTexture.create_from_image(image)


func open(color: String, screen_pos: Vector2) -> void:
	for option in OPTIONS:
		var button := $Panel/Pieces.get_node(option.capitalize()) as Button
		button.icon = _icons[color + "-" + option]
	visible = true
	await get_tree().process_frame
	if not visible:
		return
	$Panel.position = screen_pos - $Panel.size * 0.5
	$Panel.position = $Panel.position.clamp(Vector2.ZERO, get_viewport().get_visible_rect().size - $Panel.size)


func close() -> void:
	visible = false


func _on_piece_pressed(piece_name: String) -> void:
	piece_chosen.emit(piece_name)
	close()
