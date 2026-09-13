## Dungeon orchestrator: floors, loot, combat, Prolog queries.
## Descend (key + door) to floor 4, then climb back after the treasure.
extends Node2D

const PrologGameScript = preload("res://scripts/prolog_game.gd")
const WallScript = preload("res://scripts/wall.gd")
const ProjectileScript = preload("res://scripts/projectile.gd")
const PLAYER_DAMAGE := 16
const GOBLIN_DAMAGE := 8
const FIREBALL_DAMAGE := 14
const MAX_FLOOR := 4
const GOBLIN_SCENE := preload("res://scenes/goblin.tscn")
const DOOR_SCENE := preload("res://scenes/door.tscn")
const ITEM_SCENE := preload("res://scenes/item.tscn")
const PickupScript = preload("res://scripts/pickup.gd")

var game: PrologGame
var game_over: bool = false
var player_has_key: bool = false
var last_decision: String = "Thinking..."
var floor_index: int = 1
## true once the player picks up the treasure: only UP stairs matter.
var ascending: bool = false
var goblins: Array[Node2D] = []
## Goblins + wizards still in _actors for combat.
var hostiles: Array[Node2D] = []
var potions: Array[Area2D] = []
## Maps logical slots ("sword", "bow", "potion_inv"…) to Area2D / token nodes.
var loot_items: Dictionary = {}
var door: StaticBody2D = null
## DOWN stairs node (Prolog functor stairs_down/1); name kept for history.
var dungeon_exit: Area2D = null
var stairs_up: Area2D = null
var chest: Area2D = null
var key: Area2D = null
var _walls: Array[Node] = []

@onready var player: CharacterBody2D = $Player
@onready var world: Node2D = $World
## Off-screen parent for inventory nodes that survive floor changes.
@onready var keep: Node2D = $Keep
@onready var camera: Camera2D = $Camera2D
@onready var hp_bar: ProgressBar = $HUD/Panel/VBox/HpBar
@onready var inventory: Label = $HUD/Panel/VBox/Inventory
@onready var decision: Label = $HUD/Panel/VBox/Decision
@onready var goals: Label = $HUD/Panel/VBox/Goals
@onready var banner: Label = $HUD/Banner
@onready var game_over_layer: CanvasLayer = $GameOver
@onready var game_over_label: Label = $GameOver/Center/VBox/Title
@onready var game_over_hint: Label = $GameOver/Center/VBox/Hint
@onready var game_over_dim: ColorRect = $GameOver/Dim
@onready var title_label: Label = $HUD/Panel/VBox/Title


## Boot Prolog, register the player, load floor 1.
func _ready() -> void:
	DungeonTheme.apply($HUD/Panel)
	player.dungeon = self
	game_over_layer.visible = false
	game_over_label.text = "GAME OVER"
	game_over_label.add_theme_color_override("font_color", DungeonTheme.ACCENT)

	game = PrologGameScript.new()
	if not game.start(self):
		banner.text = "Prologot failed to start."
		_show_game_over()
		return

	game.register(player, "player")
	_load_floor(1)
	banner.text = "Unarmed. Find a sword or bow. Unarmed goblins flee and hunt weapons."


## Door glow + auto-open when the player stands on the door tile.
func _process(_delta: float) -> void:
	if game_over:
		if Input.is_physical_key_pressed(KEY_R):
			get_tree().reload_current_scene()
		return
	if game == null:
		return
	if player_has_key and door:
		door.set_can_open_glow(game.query_can_open(player, door))
	if door and player.global_position.distance_to(door.global_position) < 40.0:
		door.try_open()
	_refresh_hud()


## Pickup uses this so only goblins (not wizards) grab floor weapons.
func is_goblin(body: Node) -> bool:
	return body in goblins and body.get("role") == "goblin"


## Projectile.gd: player arrows hit living hostiles only.
func is_hostile(body: Node) -> bool:
	return body in hostiles and body.get("hp") > 0


## Space: Prolog decides can_melee / can_shoot / must_drop_sword; Godot applies it.
func player_attack() -> void:
	if game.query_true("can_melee", [player]):
		player.play_weapon_use("sword")
		var target := _nearest_living_hostile()
		if target and player.global_position.distance_to(target.global_position) <= PrologGame.NEAR_DIST + 10.0:
			target.take_hit(PLAYER_DAMAGE)
			_shake()
			return
		banner.text = "Too far for the sword. Get closer — or drop it (G) to use a bow."
		return
	if game.query_true("can_shoot", [player]) and player.arrows > 0:
		player.play_weapon_use("bow")
		_shoot_arrow()
		return
	if game.query_true("must_drop_sword", [player]):
		banner.text = "Hands full. Drop the sword (G) — Prolog: can_shoot :- \\+ holding_sword."
		return
	banner.text = "No weapon. Find a sword or a bow."


## Q: requires has(player, Potion). Heal 30 HP then retract has/2.
func player_drink() -> void:
	var potion: Area2D = loot_items.get("potion_inv", null)
	if potion == null or not game.query_true("has", [player, potion]):
		banner.text = "No potion in inventory (Q to drink)."
		return
	player.hp = mini(player.max_hp, player.hp + 30)
	game.set_has(player, potion, false)
	loot_items.erase("potion_inv")
	spawn_burst(player.global_position, DungeonTheme.SUCCESS)
	banner.text = "You drank a potion."


## G: retract has(player, sword) so can_shoot can succeed (\\+ holding_sword).
func player_drop_sword() -> void:
	if not game.query_true("holding_sword", [player]):
		banner.text = "You are not holding a sword."
		return
	var sword: Area2D = loot_items.get("sword", null)
	if sword == null or not is_instance_valid(sword):
		for child in keep.get_children():
			if child is Area2D and child.get("kind") == PickupScript.Kind.SWORD:
				sword = child
				break
	if sword == null:
		banner.text = "Cannot find the sword node to drop."
		return
	game.set_has(player, sword, false)
	loot_items.erase("sword")
	_drop_on_floor(sword)
	if game.query_true("can_shoot", [player]):
		player.set_held_weapon("bow")
		banner.text = "Sword dropped ahead. Walk to it to pick it up. Bow is free."
	else:
		player.set_held_weapon("")
		banner.text = "Sword dropped."
	spawn_burst(sword.global_position, DungeonTheme.GLOW)


## Drop the item in front of the player + on_floor/1 (unarmed goblins can see it).
func _drop_on_floor(item: Area2D) -> void:
	if item.is_in_group("kept_item"):
		item.remove_from_group("kept_item")
	item.reparent(world)
	var look: Vector2 = player.facing
	if look.length() > 0.1:
		look = look.normalized()
	else:
		look = Vector2.DOWN
	var dest: Vector2 = player.global_position + look * 96.0
	dest.x = clampf(dest.x, 64.0, 1216.0)
	dest.y = clampf(dest.y, 64.0, 656.0)
	item.global_position = dest
	item.visible = true
	item.taken = false
	item.pickup_lock = 0.45
	item.monitoring = false
	item.set_deferred("monitoring", true)
	game.set_on_floor(item, true)


## Decrement Godot arrow count; retract has(player, Arrow) when empty.
func _shoot_arrow() -> void:
	player.arrows -= 1
	if player.arrows <= 0:
		var arrow_item: Node2D = loot_items.get("arrow", null)
		if arrow_item:
			game.set_has(player, arrow_item, false)
	_spawn_bolt(player.global_position, player.facing, false, "arrow", 11, 420.0)
	_shake()


## Prolog `cast` action: fireball toward the player.
func wizard_cast(wizard: Node2D) -> void:
	var dir: Vector2 = player.global_position - wizard.global_position
	_spawn_bolt(wizard.global_position, dir, true, "fireball", FIREBALL_DAMAGE, 280.0)


## Shared spawner for player arrows and wizard fireballs.
func _spawn_bolt(from: Vector2, dir: Vector2, hits_player: bool, style: String, dmg: int, speed: float) -> void:
	var bolt := Area2D.new()
	bolt.set_script(ProjectileScript)
	bolt.position = from
	var look := dir.normalized() if dir.length() > 0.1 else Vector2.RIGHT
	bolt.velocity = look * speed
	bolt.dungeon = self
	bolt.hits_player = hits_player
	bolt.style = style
	bolt.damage = dmg
	add_child(bolt)


## Godot distance check after Prolog chose attack/2 (can_attack already passed).
func goblin_attack(goblin: Node2D) -> void:
	if player.global_position.distance_to(goblin.global_position) <= PrologGame.NEAR_DIST + 8.0:
		player.take_hit(GOBLIN_DAMAGE)
		_shake()


## Goblin grabbed a potion → has(Goblin, Potion) for can_drink/2 in ai.pl.
func goblin_pick_potion(goblin: Node2D, potion: Area2D) -> void:
	goblin.has_potion = true
	goblin.potion_node = potion
	game.set_has(goblin, potion, true)
	potion.visible = false
	potion.monitoring = false
	spawn_burst(potion.global_position, DungeonTheme.SUCCESS)


## Goblin pickup: only if unarmed/1. Assert has/2 → armed/1.
func monster_pick_weapon(monster: Node2D, item: Area2D, functor: String) -> bool:
	if not game.query_true("unarmed", [monster]):
		return false
	game.set_has(monster, item, true)
	item.visible = false
	item.monitoring = false
	monster.set_held_weapon(functor)
	spawn_burst(item.global_position, DungeonTheme.GOLD)
	banner.text = "A goblin armed itself — Prolog: armed/1."
	return true


func consume_potion(potion: Node2D) -> void:
	if potion:
		potion.visible = false
		if potion is Area2D:
			potion.taken = true
			potion.monitoring = false
	potions.erase(potion)


## Player pickup. Treasure sets `ascending` (must climb back up).
func player_pick_item(item: Area2D, functor: String) -> void:
	_stash(item)
	if functor == "bow":
		game.set_has(player, item, true)
		player.arrows += 5
		_ensure_arrow_token()
		loot_items["bow"] = item
		if game.query_true("must_drop_sword", [player]):
			banner.text = "Bow found, but Prolog blocks can_shoot/1 while holding_sword. Press G to drop the sword."
		else:
			player.set_held_weapon("bow")
			banner.text = "Bow + arrows. Space to shoot."
	elif functor == "arrow":
		player.arrows += 4
		_ensure_arrow_token()
		banner.text = "Arrows on the ground: +4 (total %d)." % player.arrows
	elif functor == "potion":
		loot_items["potion_inv"] = item
		game.set_has(player, item, true)
		banner.text = "Potion stored. Press Q to drink."
	else:
		game.set_has(player, item, true)
		match functor:
			"sword":
				loot_items["sword"] = item
				player.set_held_weapon("sword")
				if game.query_true("must_drop_sword", [player]):
					banner.text = "Sword in hand: can_melee/1. Drop it (G) to use the bow."
				else:
					banner.text = "Sword found. Prolog: can_melee/1. Space to strike."
			"armor":
				banner.text = "Armor on. Incoming damage is reduced."
			"treasure":
				ascending = true
				banner.text = "The treasure is yours. Climb the UP stairs, floor by floor, to the surface."
	spawn_burst(player.global_position, DungeonTheme.GOLD)


## Hidden token in $Keep: Prolog needs an arrow/1 node for can_shoot.
func _ensure_arrow_token() -> void:
	var quiver: Node2D = loot_items.get("arrow", null)
	if quiver == null or not is_instance_valid(quiver):
		quiver = _hidden_token("arrow")
		loot_items["arrow"] = quiver
	game.set_has(player, quiver, true)


## Floors 1–3: key pops out. Floor 4: treasure (starts ascending phase).
func player_open_chest(opened_chest: Area2D) -> void:
	if opened_chest.contains_treasure:
		var treasure := _spawn_item(PickupScript.Kind.TREASURE, opened_chest.global_position + Vector2(0, -28), "treasure")
		treasure.dungeon = self
		banner.text = "The chest holds the dungeon treasure."
	else:
		key.global_position = opened_chest.global_position + Vector2(0, -28)
		key.visible = true
		key.monitoring = true
		banner.text = "A key! Pick it up so can_open/2 becomes true."
	spawn_burst(opened_chest.global_position, DungeonTheme.GOLD)


func player_pick_key(key_item: Area2D) -> void:
	player_has_key = true
	_stash(key_item)
	game.set_has(player, key_item, true)
	if door:
		door.set_can_open_glow(true)
	banner.text = "can_open(Player, Door) is now true."
	spawn_burst(player.global_position, DungeonTheme.GOLD)


## DOWN stairs. Blocked if already climbing, the door is shut, or this is floor 4.
func player_go_down() -> void:
	if ascending:
		banner.text = "You have the treasure — use the UP stairs to climb out."
		return
	if door and not door.opened:
		banner.text = "The way down is sealed until Prolog says can_open/2."
		return
	if floor_index >= MAX_FLOOR:
		banner.text = "This is the bottom. Find the treasure, then climb up."
		return
	game.set_at(player, dungeon_exit, true)
	_load_floor(floor_index + 1)
	banner.text = "Down to floor %d." % floor_index


## UP stairs. Requires completed(find_treasure). Floor 1 → SUCCESS (reach_surface).
func player_go_up() -> void:
	if not game.query_true("completed", [player, "find_treasure"]):
		banner.text = "The up stairs wait until you hold the treasure."
		return
	if floor_index <= 1:
		game.set_at(player, stairs_up, true)
		_show_success()
		return
	game.set_at(player, stairs_up, true)
	_load_floor(floor_index - 1)
	banner.text = "Climbing… floor %d. Keep going up to the surface." % floor_index


func on_player_dead() -> void:
	game.set_alive(player, false)
	_show_game_over()


func on_goblin_dead() -> void:
	if _living_hostile_count() == 0:
		banner.text = "Room is clear. Grab loot and use the stairs."


## Feeds the HUD "Prolog: FLEE → Player" line from goblin.think().
func on_goblin_decision(action: String, target: Node2D) -> void:
	var target_name: String = str(target.name) if target else "?"
	last_decision = "%s → %s" % [action.to_upper(), target_name]


func spawn_burst(at: Vector2, color: Color) -> void:
	var particles := CPUParticles2D.new()
	particles.one_shot = true
	particles.emitting = true
	particles.amount = 16
	particles.lifetime = 0.45
	particles.explosiveness = 0.95
	particles.spread = 180.0
	particles.gravity = Vector2(0, 240)
	particles.initial_velocity_min = 60.0
	particles.initial_velocity_max = 140.0
	particles.scale_amount_min = 2.0
	particles.scale_amount_max = 4.0
	particles.color = color
	particles.position = at
	add_child(particles)
	get_tree().create_timer(0.6).timeout.connect(particles.queue_free)


## Change floor: forget the Prolog world except player + inventory (kept_item).
func _load_floor(index: int) -> void:
	floor_index = index
	player_has_key = false
	_clear_world()
	var preserve: Array[Node2D] = [player]
	game.retract_level_world(preserve)
	game.keep_actor(player)
	game.set_alive(player, true)
	player.global_position = Vector2(160, 560)
	_build_walls(index)
	_spawn_level(index)
	game.sync_spatial()
	_refresh_hud()


## New run: empty $Keep, retract has/2 and persisted item types.
func _new_run() -> void:
	for child in keep.get_children():
		child.queue_free()
	loot_items.clear()
	ascending = false
	player.reset_stats()
	if game.prolog:
		game.prolog.retract_all(game.prolog.predicate("has").call(game.prolog.anonymous(), game.prolog.anonymous()))
		for pred in ["sword", "bow", "arrow", "armor", "treasure", "key"]:
			game.prolog.retract_all(game.prolog.predicate(pred).call(game.prolog.anonymous()))
	_load_floor(1)
	banner.text = "New run. Unarmed again — find a weapon."


## Free scene nodes; Prolog facts are cleared separately in retract_level_world.
func _clear_world() -> void:
	for wall in _walls:
		if is_instance_valid(wall):
			wall.queue_free()
	_walls.clear()
	for child in world.get_children():
		child.queue_free()
	goblins.clear()
	hostiles.clear()
	potions.clear()
	door = null
	dungeon_exit = null
	stairs_up = null
	chest = null
	key = null


## Prolog roster (goblin/wizard, armed), floor loot, door/key, stairs.
func _spawn_level(index: int) -> void:
	var spots: Array[Vector2] = [
		Vector2(960, 240), Vector2(720, 180), Vector2(880, 480), Vector2(400, 220), Vector2(560, 300)
	]
	var roster: Array[Dictionary] = _floor_roster(index)
	for i in roster.size():
		var spec: Dictionary = roster[i]
		var unit: CharacterBody2D = GOBLIN_SCENE.instantiate()
		unit.role = str(spec["role"])
		unit.position = spots[i % spots.size()] + Vector2(i * 10, (i % 2) * 12)
		unit.dungeon = self
		unit.hp = int(spec["hp"])
		unit.max_hp = unit.hp
		world.add_child(unit)
		unit._apply_role_visual()
		hostiles.append(unit)
		if unit.role == "wizard":
			game.register(unit, "wizard")
		else:
			goblins.append(unit)
			game.register(unit, "goblin")
		game.set_enemy(unit, player)
		if spec.get("armed", false):
			var blade := _hidden_token("sword")
			game.set_has(unit, blade, true)
			unit.set_held_weapon("sword")

	# Floor loot — register() + set_on_floor() so seek_weapon/2 can see weapons.
	_spawn_item(PickupScript.Kind.POTION, Vector2(200, 150), "potion")
	_spawn_item(PickupScript.Kind.ARROW, Vector2(420, 500), "arrow")
	_spawn_item(PickupScript.Kind.ARROW, Vector2(780, 360), "arrow")

	if index == 1:
		_spawn_item(PickupScript.Kind.SWORD, Vector2(300, 500), "sword")
	if index == 2:
		_spawn_item(PickupScript.Kind.BOW, Vector2(360, 420), "bow")
		_spawn_item(PickupScript.Kind.SWORD, Vector2(520, 140), "sword")
	if index == 3:
		_spawn_item(PickupScript.Kind.ARMOR, Vector2(340, 500), "armor")
		_spawn_item(PickupScript.Kind.POTION, Vector2(480, 140), "potion")
		_spawn_item(PickupScript.Kind.SWORD, Vector2(900, 560), "sword")
	if index == 4:
		_spawn_item(PickupScript.Kind.ARROW, Vector2(240, 300), "arrow")

	chest = _spawn_item(PickupScript.Kind.CHEST, Vector2(640, 580), "chest")
	chest.contains_treasure = index >= MAX_FLOOR and not ascending

	# No door while climbing back — only UP stairs matter on the ascent.
	var need_key_door := index < MAX_FLOOR and not ascending
	if need_key_door:
		door = DOOR_SCENE.instantiate()
		door.position = Vector2(1120, 400)
		door.dungeon = self
		world.add_child(door)
		game.register(door, "door")
		key = _spawn_item(PickupScript.Kind.KEY, Vector2(640, 540), "key")
		key.visible = false
		key.monitoring = false
		game.set_requires(door, key)
		dungeon_exit = _spawn_item(PickupScript.Kind.EXIT, Vector2(1210, 400), "stairs_down")

	if index > 1 or ascending:
		stairs_up = _spawn_item(PickupScript.Kind.STAIRS_UP, Vector2(80, 400), "stairs_up")
		if stairs_up.get_node_or_null("Visual"):
			stairs_up.get_node("Visual").highlight = ascending


## Per-floor lineup. `armed` → hidden sword token + has/2 at spawn.
func _floor_roster(index: int) -> Array[Dictionary]:
	match index:
		1:
			return [
				{"role": "goblin", "armed": false, "hp": 40},
				{"role": "goblin", "armed": true, "hp": 52},
			]
		2:
			return [
				{"role": "goblin", "armed": false, "hp": 42},
				{"role": "goblin", "armed": true, "hp": 56},
				{"role": "wizard", "armed": false, "hp": 38},
			]
		3:
			return [
				{"role": "goblin", "armed": false, "hp": 44},
				{"role": "goblin", "armed": true, "hp": 60},
				{"role": "goblin", "armed": true, "hp": 58},
				{"role": "wizard", "armed": false, "hp": 42},
			]
		_:
			return [
				{"role": "goblin", "armed": true, "hp": 64},
				{"role": "goblin", "armed": true, "hp": 64},
				{"role": "wizard", "armed": false, "hp": 48},
				{"role": "wizard", "armed": false, "hp": 48},
			]


## Instantiate item.tscn; functor must match a :- dynamic in dungeon.pl.
func _spawn_item(kind: int, pos: Vector2, functor: String) -> Area2D:
	var item: Area2D = ITEM_SCENE.instantiate()
	item.kind = kind
	item.position = pos
	item.dungeon = self
	world.add_child(item)
	game.register(item, functor)
	if kind in [PickupScript.Kind.SWORD, PickupScript.Kind.BOW, PickupScript.Kind.ARROW, PickupScript.Kind.POTION, PickupScript.Kind.TREASURE]:
		game.set_on_floor(item, true)
	if kind == PickupScript.Kind.POTION:
		potions.append(item)
	return item


## Sprite-less node in $Keep: a weapon already owned on the Prolog side.
func _hidden_token(functor: String) -> Node2D:
	var token := Node2D.new()
	token.name = functor.capitalize()
	token.add_to_group("kept_item")
	keep.add_child(token)
	game.register(token, functor)
	return token


## Hide a picked-up loot and move it into $Keep so it survives a floor change.
func _stash(item: Area2D) -> void:
	item.visible = false
	item.monitoring = false
	item.add_to_group("kept_item")
	game.set_on_floor(item, false)
	item.reparent(keep)


func _nearest_living_hostile() -> Node2D:
	var best: Node2D = null
	var best_d := INF
	for g in hostiles:
		if g == null or not is_instance_valid(g) or g.hp <= 0:
			continue
		var d := player.global_position.distance_to(g.global_position)
		if d < best_d:
			best_d = d
			best = g
	return best


func _living_hostile_count() -> int:
	var n := 0
	for g in hostiles:
		if g and is_instance_valid(g) and g.hp > 0:
			n += 1
	return n


## Inventory line mirrors Prolog predicates (can_melee, wearing_armor, completed…).
func _refresh_hud() -> void:
	var dir := "↑ climb" if ascending else "↓ delve"
	title_label.text = "Mini Dungeon  ·  Floor %d / %d  ·  %s" % [floor_index, MAX_FLOOR, dir]
	hp_bar.max_value = player.max_hp
	hp_bar.value = player.hp
	var bits: PackedStringArray = []
	if game.query_true("can_melee", [player]):
		bits.append("Sword")
	if player.arrows > 0:
		bits.append("Arrows (%d)" % player.arrows)
	if game.query_true("can_shoot", [player]):
		bits.append("Bow")
	if game.query_true("wearing_armor", [player]):
		bits.append("Armor")
	if loot_items.has("potion_inv"):
		bits.append("Potion")
	if player_has_key:
		bits.append("Key")
	if game.query_true("completed", [player, "find_treasure"]):
		bits.append("Treasure")
	inventory.text = "Gear: " + (", ".join(bits) if bits.size() else "none — find a weapon")
	decision.text = "Prolog: " + last_decision
	if game and not game_over:
		if ascending:
			goals.text = "Goal: climb up to the surface"
		else:
			var g1 := "weapon" if game.query_true("can_fight", [player]) else "…"
			var g2 := "down" if floor_index < MAX_FLOOR else "treasure"
			goals.text = "Goals: %s / reach %s" % [g1, g2]


func _show_game_over() -> void:
	game_over = true
	game_over_dim.color = Color(0.35, 0.02, 0.06, 0.82)
	game_over_label.text = "GAME OVER"
	game_over_label.add_theme_color_override("font_color", DungeonTheme.ACCENT)
	game_over_hint.text = "Press R to restart"
	game_over_layer.visible = true
	banner.text = ""


## Victory: completed(reach_surface) — treasure + floor-1 UP stairs.
func _show_success() -> void:
	game_over = true
	game_over_dim.color = Color(0.02, 0.28, 0.18, 0.82)
	game_over_label.text = "SUCCESS"
	game_over_label.add_theme_color_override("font_color", DungeonTheme.SUCCESS)
	game_over_hint.text = "You reached the surface with the treasure.\nPress R for a new run."
	game_over_layer.visible = true
	banner.text = ""


func _shake() -> void:
	var origin := camera.offset
	var tween := create_tween()
	tween.tween_property(camera, "offset", origin + Vector2(4, -3), 0.04)
	tween.tween_property(camera, "offset", origin, 0.08)


## Procedural layout grows with depth (extra rects on floors 2–4).
func _build_walls(index: int) -> void:
	var size := get_viewport_rect().size
	var t := 28.0
	var rects: Array[Rect2] = [
		Rect2(0, 0, size.x, t),
		Rect2(0, size.y - t, size.x, t),
		Rect2(0, 0, t, size.y),
		Rect2(size.x - t, 0, t, size.y),
		Rect2(280, 200, 140, 36),
		Rect2(520, 360, 36, 180),
		Rect2(820, 80, 160, 36),
	]
	if index >= 2:
		rects.append(Rect2(180, 380, 120, 28))
	if index >= 3:
		rects.append(Rect2(700, 300, 28, 140))
	if index >= 4:
		rects.append(Rect2(420, 80, 28, 160))
	for r in rects:
		var wall := StaticBody2D.new()
		wall.set_script(WallScript)
		world.add_child(wall)
		wall.setup(r)
		_walls.append(wall)


func _exit_tree() -> void:
	if game:
		game.cleanup()
