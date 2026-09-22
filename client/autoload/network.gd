extends Node

signal connected
signal disconnected
signal move_received(from: Vector2i, to: Vector2i)

var active := false


func connect_to_server(_address: String, _port: int) -> void:
	active = false


func send_move(_from: Vector2i, _to: Vector2i) -> void:
	pass
