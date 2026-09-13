## Collision wall + procedural draw. Created by dungeon._build_walls.
extends StaticBody2D


## rect is in world space; collision + draw child are centered on it.
func setup(rect: Rect2) -> void:
	position = rect.position + rect.size * 0.5
	var cs := CollisionShape2D.new()
	var shape := RectangleShape2D.new()
	shape.size = rect.size
	cs.shape = shape
	add_child(cs)
	var draw := Node2D.new()
	draw.draw.connect(func():
		var r := Rect2(-rect.size * 0.5, rect.size)
		draw.draw_rect(r, DungeonTheme.STONE)
		draw.draw_rect(Rect2(r.position, Vector2(r.size.x, 4)), DungeonTheme.STONE_HI)
		draw.draw_rect(Rect2(r.position + Vector2(0, r.size.y - 5), Vector2(r.size.x, 5)), DungeonTheme.STONE_SHADOW)
		draw.draw_rect(r.grow(-2), DungeonTheme.STONE_HI.darkened(0.15), false, 1.0)
	)
	add_child(draw)
	draw.queue_redraw()
