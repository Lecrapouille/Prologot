## Player. Keys call dungeon.player_*; Prolog allows or refuses.
extends CharacterBody2D

const SPEED := 190.0
const ATTACK_COOLDOWN := 0.45

var hp: int = 100
var max_hp: int = 100
var facing: Vector2 = Vector2.DOWN
var attack_cd: float = 0.0
var arrows: int = 0  ## Godot ammo counter; Prolog still needs has(Player, Arrow) for can_shoot
var dungeon: Node = null

@onready var _visual: ProceduralActor = $Visual


func _ready() -> void:
	var cs := $CollisionShape2D
	if cs.shape == null:
		var circle := CircleShape2D.new()
		circle.radius = 12
		cs.shape = circle
	z_index = 5
	_visual.kind = ProceduralActor.Kind.PLAYER


func _physics_process(delta: float) -> void:
	if dungeon and dungeon.game_over:
		velocity = Vector2.ZERO
		move_and_slide()
		return
	var dir := _move_dir()
	if dir != Vector2.ZERO:
		facing = dir
		_visual.facing = dir
	velocity = dir * SPEED
	move_and_slide()
	attack_cd = maxf(0.0, attack_cd - delta)
	# Space / Q / G: the dungeon queries Prolog before acting.
	if Input.is_physical_key_pressed(KEY_SPACE) and attack_cd <= 0.0:
		attack_cd = ATTACK_COOLDOWN
		if dungeon:
			dungeon.player_attack()
	if Input.is_physical_key_pressed(KEY_Q) and attack_cd <= 0.0:
		attack_cd = 0.2
		if dungeon:
			dungeon.player_drink()
	if Input.is_physical_key_pressed(KEY_G) and attack_cd <= 0.0:
		attack_cd = 0.25
		if dungeon:
			dungeon.player_drop_sword()


## Armor: wearing_armor/1 reduces damage (~45%). Death → set_alive(false).
func take_hit(amount: int) -> void:
	if dungeon and dungeon.game.query_true("wearing_armor", [self]):
		amount = maxi(1, int(amount * 0.55))
	hp = maxi(0, hp - amount)
	modulate = DungeonTheme.ACCENT
	var tween := create_tween()
	tween.tween_property(self, "modulate", Color.WHITE, 0.2)
	if dungeon:
		dungeon.spawn_burst(global_position, DungeonTheme.ACCENT)
	if hp <= 0 and dungeon:
		dungeon.on_player_dead()


func set_held_weapon(weapon: String) -> void:
	_visual.held_weapon = weapon
	_visual.queue_redraw()


func play_weapon_use(weapon: String) -> void:
	set_held_weapon(weapon)
	_visual.play_use()


## Called on a new run after $Keep is cleared.
func reset_stats() -> void:
	hp = max_hp
	arrows = 0
	modulate = Color.WHITE
	set_held_weapon("")


func _move_dir() -> Vector2:
	var d := Vector2.ZERO
	if Input.is_physical_key_pressed(KEY_A) or Input.is_physical_key_pressed(KEY_LEFT):
		d.x -= 1
	if Input.is_physical_key_pressed(KEY_D) or Input.is_physical_key_pressed(KEY_RIGHT):
		d.x += 1
	if Input.is_physical_key_pressed(KEY_W) or Input.is_physical_key_pressed(KEY_UP):
		d.y -= 1
	if Input.is_physical_key_pressed(KEY_S) or Input.is_physical_key_pressed(KEY_DOWN):
		d.y += 1
	return d.normalized()
