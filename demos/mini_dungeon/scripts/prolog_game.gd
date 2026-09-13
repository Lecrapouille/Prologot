## Godot ↔ Prolog bridge.
## Positions stay in Godot. Prolog only sees facts
## (player/1, has/2, near/2, visible/2…). Nodes are PrologObjects.
extends RefCounted
class_name PrologGame

const NEAR_DIST := 52.0      ## Melee threshold → near/2
const VISIBLE_DIST := 380.0  ## Sight / loot range → visible/2

var prolog: Prologot
var _world: Node2D
## Nodes already register()'d; used to recompute near/visible.
var _actors: Array[Node2D] = []


## Bundled SWI-Prolog path per OS (see demos/mini_dungeon/bin/).
static func swipl_home() -> String:
	var os_map := {"Linux": "linux", "Windows": "windows", "macOS": "macos"}
	return "res://bin/" + os_map.get(OS.get_name(), OS.get_name().to_lower()) + "/swipl"


## Start SWI-Prolog and consult dungeon / rules / combat / ai.
func start(world: Node2D) -> bool:
	_world = world
	prolog = Prologot.new()
	var options := {}
	var home := swipl_home()
	if DirAccess.dir_exists_absolute(home) or DirAccess.dir_exists_absolute(ProjectSettings.globalize_path(home)):
		options["home"] = home
	if not prolog.initialize(options):
		push_error("Prologot: " + prolog.get_last_error())
		return false
	for path in [
		"res://prolog/dungeon.pl",
		"res://prolog/rules.pl",
		"res://prolog/combat.pl",
		"res://prolog/ai.pl",
	]:
		if not prolog.consult_file(path):
			push_error("Failed to consult %s: %s" % [path, prolog.get_last_error()])
			return false
	return true


## Assert functor(Node). Player / goblin / wizard also get alive + healthy.
func register(node: Node2D, functor: String) -> void:
	if node == null or prolog == null:
		return
	if node not in _actors:
		_actors.append(node)
	prolog.assert_fact(prolog.predicate(functor).call(node))
	if functor in ["player", "goblin", "wizard"]:
		prolog.assert_fact(prolog.predicate("alive").call(node))
		prolog.assert_fact(prolog.predicate("healthy").call(node))


## Put a node back into _actors after retract_level_world (e.g. the player).
func keep_actor(node: Node2D) -> void:
	if node != null and node not in _actors:
		_actors.append(node)


## enemy(Attacker, Target) — required by can_attack/2 and can_cast/2.
func set_enemy(attacker: Node2D, target: Node2D) -> void:
	prolog.assert_fact(prolog.predicate("enemy").call(attacker, target))


## requires(Door, Key) — links the door to the chest key on this floor.
func set_requires(door: Node2D, key: Node2D) -> void:
	prolog.assert_fact(prolog.predicate("requires").call(door, key))


## Prolog inventory: has(Owner, Item). A held item is no longer on_floor/1.
func set_has(owner: Node2D, item: Node2D, on: bool) -> void:
	var goal = prolog.predicate("has").call(owner, item)
	if on:
		prolog.assert_fact(goal)
		set_on_floor(item, false)
	else:
		prolog.retract_fact(goal)


## Loot visible to seek_weapon/2. The floor_loot group is used by sync_spatial.
func set_on_floor(item: Node2D, on: bool) -> void:
	if item == null or prolog == null:
		return
	var goal = prolog.predicate("on_floor").call(item)
	if on:
		prolog.assert_fact(goal)
		if not item.is_in_group("floor_loot"):
			item.add_to_group("floor_loot")
	else:
		prolog.retract_all(goal)
		if item.is_in_group("floor_loot"):
			item.remove_from_group("floor_loot")


## Toggle healthy/1 ↔ wounded/1 (goblin.gd sets wounded below HP threshold).
func set_wounded(node: Node2D, wounded: bool) -> void:
	if wounded:
		prolog.retract_all(prolog.predicate("healthy").call(node))
		prolog.assert_fact(prolog.predicate("wounded").call(node))
	else:
		prolog.retract_all(prolog.predicate("wounded").call(node))
		prolog.assert_fact(prolog.predicate("healthy").call(node))


## Toggle alive/1 ↔ dead/1 when a combatant dies or respawns on a new floor.
func set_alive(node: Node2D, alive: bool) -> void:
	if alive:
		prolog.retract_all(prolog.predicate("dead").call(node))
		prolog.assert_fact(prolog.predicate("alive").call(node))
	else:
		prolog.retract_all(prolog.predicate("alive").call(node))
		prolog.assert_fact(prolog.predicate("dead").call(node))


## Assert opened/1 after door.gd disables collision (completed open_door).
func set_opened(door: Node2D) -> void:
	prolog.assert_fact(prolog.predicate("opened").call(door))


## at(Who, Place) — used by completed(reach_exit / reach_surface).
func set_at(who: Node2D, place: Node2D, on: bool) -> void:
	var goal = prolog.predicate("at").call(who, place)
	if on:
		prolog.assert_fact(goal)
	else:
		prolog.retract_all(goal)


## Recompute near/2 and visible/2 between combatants, and combatant↔loot.
func sync_spatial() -> void:
	prolog.retract_all(prolog.predicate("near").call(prolog.anonymous(), prolog.anonymous()))
	prolog.retract_all(prolog.predicate("visible").call(prolog.anonymous(), prolog.anonymous()))
	var living: Array[Node2D] = []
	for actor in _actors:
		if is_instance_valid(actor) and actor.is_inside_tree():
			living.append(actor)
	_actors = living
	for i in _actors.size():
		for j in _actors.size():
			if i == j:
				continue
			var a := _actors[i]
			var b := _actors[j]
			var dist := a.global_position.distance_to(b.global_position)
			if not _should_relate(a, b):
				continue
			if dist <= NEAR_DIST:
				prolog.assert_fact(prolog.predicate("near").call(a, b))
			if dist <= VISIBLE_DIST:
				prolog.assert_fact(prolog.predicate("visible").call(a, b))


## Single best_action/3 solution (Prolog cut). Default: wander.
func query_best_action(goblin: Node2D) -> Dictionary:
	var action := prolog.variable("Action")
	var target := prolog.variable("Target")
	var sol: PrologSolution = prolog.solve(prolog.predicate("best_action").call(goblin, action, target)).first()
	if sol == null:
		return {"action": "wander", "target": goblin}
	return {"action": str(sol.get(action)), "target": _as_node(sol.get(target), goblin)}


## True when the player holds the key that requires/2 this door.
func query_can_open(player: Node2D, door: Node2D) -> bool:
	return prolog.solve(prolog.predicate("can_open").call(player, door)).has_solution()


## Goal atoms: find_key, open_door, find_treasure, reach_exit, reach_surface.
func query_completed(player: Node2D, goal_name: String) -> bool:
	return prolog.solve(prolog.predicate("completed").call(player, goal_name)).has_solution()


## Generic has_solution/0 check — e.g. query_true("can_melee", [player]).
func query_true(functor: String, args: Array) -> bool:
	return prolog.solve(prolog.predicate(functor).callv(args)).has_solution()


## Forget this floor's facts. Preserve + kept_item group (inventory) stay.
## Without preserve:[player], the player's near/visible facts vanish (deaf goblins).
func retract_level_world(preserve: Array[Node2D] = []) -> void:
	if prolog == null:
		return
	for pred in ["goblin", "wizard", "potion", "door", "chest", "stairs_down", "stairs_up", "key", "opened", "dead", "on_floor"]:
		prolog.retract_all(prolog.predicate(pred).call(prolog.anonymous()))
	for pred in ["enemy", "near", "visible", "requires", "at"]:
		prolog.retract_all(prolog.predicate(pred).call(prolog.anonymous(), prolog.anonymous()))
	var kept: Array[Node2D] = []
	for actor in _actors:
		if not is_instance_valid(actor):
			continue
		if actor in preserve or actor.is_in_group("kept_item"):
			kept.append(actor)
	_actors = kept


func cleanup() -> void:
	if prolog:
		prolog.cleanup()
		prolog = null


## Unwrap a PrologObject target from best_action/3 back into a Node2D.
func _as_node(value, fallback: Node2D) -> Node2D:
	if value is PrologObject:
		var obj = value.get_object()
		if obj is Node2D:
			return obj
	if value is Node2D:
		return value
	return fallback


func _is_combatant(node: Node2D) -> bool:
	return node is CharacterBody2D


## Combatant↔combatant (AI) and combatant↔loot (seek_weapon). Not loot↔loot.
func _should_relate(a: Node2D, b: Node2D) -> bool:
	if a is CharacterBody2D and b is CharacterBody2D:
		return true
	if a is CharacterBody2D and b.is_in_group("floor_loot"):
		return true
	if b is CharacterBody2D and a.is_in_group("floor_loot"):
		return true
	return false


## Raycast LOS helper (unused: visible/2 uses distance only in this demo).
func _has_los(a: Node2D, b: Node2D) -> bool:
	if _world == null:
		return true
	var space := _world.get_world_2d().direct_space_state
	var q := PhysicsRayQueryParameters2D.create(a.global_position, b.global_position)
	var exclude: Array[RID] = []
	if a is CollisionObject2D:
		exclude.append((a as CollisionObject2D).get_rid())
	if b is CollisionObject2D:
		exclude.append((b as CollisionObject2D).get_rid())
	q.exclude = exclude
	var hit := space.intersect_ray(q)
	return hit.is_empty()
