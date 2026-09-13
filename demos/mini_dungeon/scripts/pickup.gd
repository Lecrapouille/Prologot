## Loot / chest / stairs. Each Kind maps to a Prolog functor
## (sword/1, arrow/1, stairs_up/1…). Overlap calls dungeon.player_* / monster_*.
extends Area2D
class_name Pickup

## EXIT = stairs down visually; registered in Prolog as stairs_down/1.
enum Kind { POTION, CHEST, EXIT, KEY, SWORD, BOW, ARMOR, TREASURE, ARROW, STAIRS_UP }

@export var kind: Kind = Kind.POTION

var taken: bool = false
var opened: bool = false
var dungeon: Node = null
## true only on the floor-4 chest (while not climbing back up).
var contains_treasure: bool = false
## Prevents picking the sword back up immediately after G (drop).
var pickup_lock: float = 0.0

@onready var _visual: ProceduralActor = $Visual


func _ready() -> void:
	z_index = 3
	var cs := $CollisionShape2D
	if cs.shape == null:
		var circle := CircleShape2D.new()
		circle.radius = 16
		cs.shape = circle
	_apply_visual()
	if not body_entered.is_connected(_on_body_entered):
		body_entered.connect(_on_body_entered)


## After drop (pickup_lock), re-fire overlap for bodies still standing on the item.
func _process(delta: float) -> void:
	if pickup_lock > 0.0:
		pickup_lock = maxf(0.0, pickup_lock - delta)
		if pickup_lock == 0.0:
			for body in get_overlapping_bodies():
				_on_body_entered(body)


func _apply_visual() -> void:
	if _visual == null:
		return
	var map := {
		Kind.POTION: ProceduralActor.Kind.POTION,
		Kind.CHEST: ProceduralActor.Kind.CHEST,
		Kind.EXIT: ProceduralActor.Kind.EXIT,
		Kind.STAIRS_UP: ProceduralActor.Kind.STAIRS_UP,
		Kind.KEY: ProceduralActor.Kind.KEY,
		Kind.SWORD: ProceduralActor.Kind.SWORD,
		Kind.BOW: ProceduralActor.Kind.BOW,
		Kind.ARMOR: ProceduralActor.Kind.ARMOR,
		Kind.TREASURE: ProceduralActor.Kind.TREASURE,
		Kind.ARROW: ProceduralActor.Kind.ARROW,
	}
	_visual.kind = map[kind]
	if kind == Kind.POTION:
		var glow := CPUParticles2D.new()
		glow.emitting = true
		glow.amount = 8
		glow.lifetime = 1.2
		glow.emission_shape = CPUParticles2D.EMISSION_SHAPE_SPHERE
		glow.emission_sphere_radius = 8.0
		glow.direction = Vector2(0, -1)
		glow.spread = 180.0
		glow.gravity = Vector2(0, -12)
		glow.scale_amount_min = 1.5
		glow.scale_amount_max = 3.0
		glow.color = DungeonTheme.SUCCESS
		add_child(glow)


## Dispatch by Kind. Sword/bow: an unarmed goblin may pick them up too.
func _on_body_entered(body: Node) -> void:
	if dungeon == null or pickup_lock > 0.0:
		return
	match kind:
		Kind.POTION:
			if taken:
				return
			if dungeon.is_goblin(body):
				taken = true
				dungeon.goblin_pick_potion(body, self)
			elif body == dungeon.player:
				taken = true
				dungeon.player_pick_item(self, "potion")
		Kind.CHEST:
			if opened:
				return
			if body == dungeon.player:
				opened = true
				_visual.opened = true
				dungeon.player_open_chest(self)
		Kind.EXIT:
			if body == dungeon.player:
				dungeon.player_go_down()
		Kind.STAIRS_UP:
			if body == dungeon.player:
				dungeon.player_go_up()
		Kind.KEY:
			if taken:
				return
			if body == dungeon.player:
				taken = true
				dungeon.player_pick_key(self)
		Kind.SWORD, Kind.BOW:
			if taken:
				return
			var functor := "sword" if kind == Kind.SWORD else "bow"
			if dungeon.is_goblin(body):
				if dungeon.monster_pick_weapon(body, self, functor):
					taken = true
			elif body == dungeon.player:
				taken = true
				dungeon.player_pick_item(self, functor)
		Kind.ARMOR:
			if taken:
				return
			if body == dungeon.player:
				taken = true
				dungeon.player_pick_item(self, "armor")
		Kind.TREASURE:
			if taken:
				return
			if body == dungeon.player:
				taken = true
				dungeon.player_pick_item(self, "treasure")
		Kind.ARROW:
			if taken:
				return
			if body == dungeon.player:
				taken = true
				dungeon.player_pick_item(self, "arrow")
