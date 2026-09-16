## Procedural checkerboard floor (no PNGs).
extends Node2D

const TILE := 32


func _ready() -> void:
	queue_redraw()


func _draw() -> void:
	var size := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, size), DungeonTheme.STONE_SHADOW)
	for x in range(0, int(size.x), TILE):
		for y in range(0, int(size.y), TILE):
			var tx := int(x / float(TILE))
			var ty := int(y / float(TILE))
			var alt := (tx + ty) % 2 == 0
			var col := DungeonTheme.FLOOR_ALT if alt else DungeonTheme.FLOOR
			var n := ((tx * 13 + ty * 7) % 5) * 0.015
			col = col.lightened(n)
			draw_rect(Rect2(x + 1, y + 1, TILE - 2, TILE - 2), col)
			draw_rect(Rect2(x + 2, y + 2, TILE - 6, 2), col.lightened(0.07))
			draw_rect(Rect2(x + 2, y + TILE - 5, TILE - 6, 1), col.darkened(0.08))
			if (tx * 3 + ty * 5) % 11 == 0:
				draw_circle(Vector2(x + 11, y + 19), 2.4, col.darkened(0.14))
			elif (tx * 5 + ty * 2) % 13 == 0:
				draw_circle(Vector2(x + 21, y + 10), 1.6, col.lightened(0.06))
