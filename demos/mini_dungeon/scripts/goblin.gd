## Monster (goblin or wizard). Prolog picks the action; this script executes it.
extends CharacterBody2D

const SPEED := 115.0
const WIZARD_SPEED := 90.0
const ATTACK_COOLDOWN := 0.7
const CAST_COOLDOWN := 1.1
const THINK_INTERVAL := 0.25  ## How often we query best_action/3

## "goblin" or "wizard" — set at spawn, before add_child.
var role: String = "goblin"
var hp: int = 50
var max_hp: int = 50
var wounded_threshold: int = 20  ## HP at or below → assert wounded/1
var facing: Vector2 = Vector2.LEFT
var current_action: String = "wander"
var current_target: Node2D = null
var think_timer: float = 0.0
var attack_cd: float = 0.0
var wander_dir: Vector2 = Vector2.LEFT
var dungeon: Node = null
## Godot-side potion carry; Prolog sees has(Goblin, Potion) via pickup.
var has_potion: bool = false
var potion_node: Node2D = null

@onready var _visual: ProceduralActor = $Visual
@onready var _bubble: Label = $Bubble


func _ready() -> void:
	var cs := $CollisionShape2D
	if cs.shape == null:
		var circle := CircleShape2D.new()
		circle.radius = 12
		cs.shape = circle
	z_index = 5
	_apply_role_visual()
	_visual.facing = facing
	_bubble.text = "..."


## Called from dungeon._spawn_level after role is set (before or after add_child).
func _apply_role_visual() -> void:
	if role == "wizard":
		_visual.kind = ProceduralActor.Kind.WIZARD
	else:
		_visual.kind = ProceduralActor.Kind.GOBLIN


func set_held_weapon(weapon: String) -> void:
	_visual.held_weapon = weapon
	_visual.queue_redraw()


func _physics_process(delta: float) -> void:
	if dungeon == null or dungeon.game_over or hp <= 0:
		velocity = Vector2.ZERO
		move_and_slide()
		return

	think_timer -= delta
	attack_cd = maxf(0.0, attack_cd - delta)
	if think_timer <= 0.0:
		think()
		think_timer = THINK_INTERVAL

	_execute(delta)
	move_and_slide()


## Sync spatial, then best_action/3. The bubble shows the Prolog atom (FLEE, CAST…).
func think() -> void:
	dungeon.game.sync_spatial()
	var result: Dictionary = dungeon.game.query_best_action(self)
	current_action = str(result.get("action", "wander"))
	current_target = result.get("target", self)
	_bubble.text = current_action.to_upper()
	dungeon.on_goblin_decision(current_action, current_target)


## Godot owns HP. Below the threshold → wounded/1 (Prolog will flee).
func take_hit(amount: int) -> void:
	hp = maxi(0, hp - amount)
	modulate = DungeonTheme.ACCENT
	var tween := create_tween()
	tween.tween_property(self, "modulate", Color.WHITE, 0.2)
	dungeon.spawn_burst(global_position, DungeonTheme.GOLD)
	if hp > 0 and hp <= wounded_threshold:
		dungeon.game.set_wounded(self, true)
		_visual.highlight = true
	if hp <= 0:
		dungeon.game.set_alive(self, false)
		_bubble.text = "DEAD"
		_visual.modulate = Color(0.4, 0.4, 0.4)
		collision_layer = 0
		collision_mask = 0
		dungeon.on_goblin_dead()


## Prolog `drink`: heal, then retract has/2 and wounded/1.
func drink() -> void:
	if not has_potion:
		return
	hp = max_hp
	has_potion = false
	dungeon.game.set_wounded(self, false)
	if potion_node:
		dungeon.game.set_has(self, potion_node, false)
		dungeon.consume_potion(potion_node)
		potion_node = null
	_visual.highlight = false
	_bubble.text = "DRINK"
	dungeon.spawn_burst(global_position, DungeonTheme.SUCCESS)


## Interpret the atom returned by best_action (chase, flee, cast, seek_weapon…).
func _execute(_delta: float) -> void:
	match current_action:
		# armed + near → strike; otherwise close in (chase).
		"chase", "attack":
			if current_action == "attack" and dungeon.player.global_position.distance_to(global_position) <= PrologGame.NEAR_DIST + 6.0:
				_face(dungeon.player)
				velocity = Vector2.ZERO
				if attack_cd <= 0.0:
					attack_cd = ATTACK_COOLDOWN
					_visual.play_use()
					dungeon.goblin_attack(self)
			else:
				_move_towards(dungeon.player.global_position)
		# Target = Item from seek_weapon/2 (visible on_floor weapon).
		"seek_weapon":
			if current_target:
				_move_towards(current_target.global_position)
			else:
				velocity = Vector2.ZERO
		# unarmed / wounded / wizard too close → move away from the player.
		"flee", "keep_distance":
			_move_towards(_away_from(dungeon.player))
		# Wizard: back off if glued to the player, otherwise stop and shoot.
		"cast":
			_face(dungeon.player)
			var too_close: bool = dungeon.player.global_position.distance_to(global_position) < 90.0
			if too_close:
				_move_towards(_away_from(dungeon.player))
			else:
				velocity = Vector2.ZERO
			if attack_cd <= 0.0:
				attack_cd = CAST_COOLDOWN
				_visual.play_use()
				dungeon.wizard_cast(self)
		"drink":
			velocity = Vector2.ZERO
			drink()
		_:
			if randf() < 0.04:
				wander_dir = Vector2.from_angle(randf() * TAU)
			_move_towards(global_position + wander_dir * 40.0)


## Shared locomotion for chase, seek_weapon, flee, and wander.
func _move_towards(dest: Vector2) -> void:
	var dir := dest - global_position
	if dir.length() < 6.0:
		velocity = Vector2.ZERO
		return
	facing = dir.normalized()
	_visual.facing = facing
	var spd := WIZARD_SPEED if role == "wizard" else SPEED
	velocity = facing * spd


func _face(node: Node2D) -> void:
	if node == null:
		return
	facing = (node.global_position - global_position).normalized()
	_visual.facing = facing


func _away_from(node: Node2D) -> Vector2:
	return global_position + (global_position - node.global_position).normalized() * 80.0
