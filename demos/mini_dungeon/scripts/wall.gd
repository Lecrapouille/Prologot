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
	var paint := Node2D.new()
	paint.draw.connect(func():
		var r := Rect2(-rect.size * 0.5, rect.size)
		paint.draw_rect(r, DungeonTheme.STONE.darkened(0.22))
		var brick_w := 18.0
		var brick_h := 10.0
		var row := 0
		var y := 0.0
		while y < r.size.y:
			var shift := 0.0 if row % 2 == 0 else brick_w * 0.5
			var x := -shift
			while x < r.size.x:
				var brick := Rect2(r.position + Vector2(x + 1, y + 1), Vector2(brick_w - 2, brick_h - 2))
				var clipped: Rect2 = brick.intersection(r.grow(-1))
				if clipped.size.x > 1.0 and clipped.size.y > 1.0:
					var n: int = absi(row * 17 + int(x) * 13) % 5
					var col: Color = DungeonTheme.STONE_HI if n > 2 else DungeonTheme.STONE
					if n == 0:
						col = DungeonTheme.STONE.lightened(0.06)
					paint.draw_rect(clipped, col)
					if clipped.size.x > 4.0:
						paint.draw_rect(Rect2(clipped.position, Vector2(clipped.size.x, 1.5)), col.lightened(0.1))
				x += brick_w
			y += brick_h
			row += 1
		paint.draw_rect(Rect2(r.position, Vector2(r.size.x, 4)), DungeonTheme.STONE_HI.lightened(0.08))
		paint.draw_rect(Rect2(r.position + Vector2(0, r.size.y - 5), Vector2(r.size.x, 5)), DungeonTheme.STONE_SHADOW)
		paint.draw_rect(r.grow(-1), Color(DungeonTheme.STONE_HI, 0.35), false, 1.0)
	)
	add_child(paint)
	paint.queue_redraw()
