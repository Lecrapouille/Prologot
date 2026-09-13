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
		draw_circle(Vector2.ZERO, 8, Color(1, 0.35, 0.1, 0.35))
		draw_circle(Vector2.ZERO, 5, Color("#ff6b35"))
		draw_circle(Vector2.ZERO, 2.5, Color("#ffe66d"))
	else:
		draw_circle(Vector2.ZERO, 4, DungeonTheme.GOLD)
		draw_circle(Vector2.ZERO, 2, Color.WHITE)


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
