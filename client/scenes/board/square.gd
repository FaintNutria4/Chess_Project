extends Area2D

signal clicked(square)

var coords: Vector2i = Vector2i.ZERO


func _ready() -> void:
	input_event.connect(_on_input_event)


func _on_input_event(_viewport: Node, event: InputEvent, _shape_idx: int) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		clicked.emit(self)


func set_base_color(color: Color) -> void:
	$Base.color = color


func set_highlight(enabled: bool) -> void:
	$Highlight.visible = enabled
