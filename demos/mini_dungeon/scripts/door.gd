## Door stays locked while can_open/2 is false (key + requires/2).
extends StaticBody2D

var opened: bool = false
var dungeon: Node = null

@onready var _visual: ProceduralActor = $Visual
@onready var _collision: CollisionShape2D = $CollisionShape2D


func _ready() -> void:
	if _collision.shape == null:
		var rect := RectangleShape2D.new()
		rect.size = Vector2(28, 48)
		_collision.shape = rect
	z_index = 4
	_visual.kind = ProceduralActor.Kind.DOOR


## Query Prolog; if true, disable collision and assert opened/1.
func try_open() -> bool:
	if opened or dungeon == null:
		return opened
	if dungeon.game.query_can_open(dungeon.player, self):
		open()
		return true
	return false


## Side effects after can_open/2 succeeded: visual, collision off, opened/1.
func open() -> void:
	opened = true
	_visual.opened = true
	_visual.highlight = true
	_collision.set_deferred("disabled", true)
	dungeon.game.set_opened(self)
	dungeon.spawn_burst(global_position, DungeonTheme.SUCCESS)


## Green outline when query_can_open is true (player holds the right key).
func set_can_open_glow(on: bool) -> void:
	if not opened:
		_visual.highlight = on
