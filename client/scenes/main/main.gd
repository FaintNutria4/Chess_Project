extends Node2D


func _ready() -> void:
	$Board.move_played.connect(_on_move_played)


func _on_move_played(result: Dictionary) -> void:
	var text := ""
	if result["checkmate"]:
		text = "Checkmate! %s wins" % ("White" if result["turn"] == "black" else "Black")
	elif result["stalemate"]:
		text = "Stalemate! Draw"
	elif result["check"]:
		text = "Check!"
	$UI.set_status(text)
