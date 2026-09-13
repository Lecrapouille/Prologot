## Procedural checkerboard floor (no PNGs).
extends Node2D

const TILE := 32


func _ready() -> void:
	queue_redraw()


func _draw() -> void:
	var size := get_viewport_rect().size
	for x in range(0, int(size.x), TILE):
		for y in range(0, int(size.y), TILE):
			var alt := ((x / TILE) + (y / TILE)) % 2 == 0
			var col := DungeonTheme.FLOOR_ALT if alt else DungeonTheme.FLOOR
			draw_rect(Rect2(x + 1, y + 1, TILE - 2, TILE - 2), col)
