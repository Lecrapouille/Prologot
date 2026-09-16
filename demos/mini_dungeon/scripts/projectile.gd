## Player arrow or wizard fireball (`style`). Damage is applied here.
extends Area2D

var velocity: Vector2 = Vector2.ZERO
var dungeon: Node = null
var damage: int = 11
## true = wizard spell (hits the player); false = arrow (hits a hostile).
var hits_player: bool = false
var style: String = "arrow"  ## "arrow" | "fireball"
var _life: float = 0.7  ## Auto-despawn if nothing is hit


func _ready() -> void:
	z_index = 6
	collision_layer = 0
	collision_mask = 1
	var cs := CollisionShape2D.new()
	var circle := CircleShape2D.new()
	circle.radius = 6 if style == "fireball" else 5
	cs.shape = circle
	add_child(cs)
	if not body_entered.is_connected(_on_body_entered):
		body_entered.connect(_on_body_entered)


func _draw() -> void:
	if style == "fireball":
		var tail := -velocity.normalized() * 11.0 if velocity.length() > 1.0 else Vector2(-8, 0)
		draw_circle(tail * 1.3, 6, Color(1, 0.2, 0.05, 0.18))
		draw_circle(tail, 5, Color(1, 0.35, 0.08, 0.28))
		draw_circle(Vector2.ZERO, 10, Color(1, 0.35, 0.1, 0.22))
		draw_circle(Vector2.ZERO, 6.2, Color("#ff6b35"))
		draw_circle(Vector2.ZERO, 3.4, Color("#ffe66d"))
		draw_circle(Vector2(-1.4, -1.4), 1.4, Color(1, 1, 1, 0.6))
	else:
		var dir := velocity.normalized() if velocity.length() > 1.0 else Vector2.RIGHT
		var tip := dir * 11.0
		var tail := -dir * 10.0
		var perp := dir.rotated(PI * 0.5)
		draw_line(tail, tip, Color("#8d6e63"), 2.6)
		draw_line(tail, tip, Color("#d9c27a"), 1.4)
		draw_colored_polygon(PackedVector2Array([
			tip + dir * 3.0, tip + perp * 3.2 - dir * 2.0, tip - perp * 3.2 - dir * 2.0
		]), DungeonTheme.GLOW)
		draw_line(tail, tail + perp * 3.4 - dir * 3.0, Color("#c9783a"), 1.6)
		draw_line(tail, tail - perp * 3.4 - dir * 3.0, Color("#c9783a"), 1.6)


func _process(delta: float) -> void:
	position += velocity * delta
	queue_redraw()
	_life -= delta
	if _life <= 0.0:
		queue_free()


func _on_body_entered(body: Node) -> void:
	if dungeon == null:
		return
	if hits_player:
		if body == dungeon.player:
			body.take_hit(damage)
			dungeon.spawn_burst(global_position, Color("#ff6b35"))
			queue_free()
		return
	if body is CharacterBody2D and dungeon.is_hostile(body):
		body.take_hit(damage)
		dungeon.spawn_burst(global_position, DungeonTheme.GOLD)
		queue_free()
