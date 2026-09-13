## Procedural top-down drawing (0 PNGs). Kind follows Prolog functors
## (player, goblin, wizard, sword, stairs_up…).
extends Node2D
class_name ProceduralActor

enum Kind { PLAYER, GOBLIN, WIZARD, POTION, DOOR, CHEST, EXIT, STAIRS_UP, KEY, SWORD, BOW, ARMOR, TREASURE, ARROW }

@export var kind: Kind = Kind.PLAYER
@export var facing: Vector2 = Vector2.DOWN
@export var highlight: bool = false  ## wounded goblin, openable door, ascending stairs
@export var opened: bool = false
@export var held_weapon: String = ""  ## "sword" | "bow" | "" — mirrors Prolog armed state
@export var use_pose: float = 0.0     ## swing / draw animation from play_use()

var _bounce: float = 0.0


func _ready() -> void:
	var tween := create_tween().set_loops()
	tween.tween_property(self, "_bounce", -3.0, 0.45).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(self, "_bounce", 0.0, 0.45).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)


func _process(_delta: float) -> void:
	queue_redraw()


func _draw() -> void:
	match kind:
		Kind.PLAYER:
			_draw_creature(DungeonTheme.PLAYER, DungeonTheme.GLOW)
		Kind.GOBLIN:
			_draw_creature(DungeonTheme.GOBLIN, Color("#2d3436"))
		Kind.WIZARD:
			_draw_creature(Color("#9b5de5"), Color("#f72585"))
			_draw_wizard_hat()
		Kind.POTION:
			_draw_potion()
		Kind.DOOR:
			_draw_door()
		Kind.CHEST:
			_draw_chest()
		Kind.EXIT:
			_draw_exit()
		Kind.STAIRS_UP:
			_draw_stairs_up()
		Kind.KEY:
			_draw_key()
		Kind.SWORD:
			_draw_sword()
		Kind.BOW:
			_draw_bow()
		Kind.ARMOR:
			_draw_armor()
		Kind.TREASURE:
			_draw_treasure()
		Kind.ARROW:
			_draw_arrow_loot()


func _draw_creature(body: Color, accent: Color) -> void:
	var bob := Vector2(0, _bounce)
	draw_circle(Vector2(0, 10), 14, Color(0, 0, 0, 0.28))
	draw_circle(bob, 16, body)
	draw_circle(bob + Vector2(0, -2), 12, body.lightened(0.12))
	var look := facing.normalized() if facing.length() > 0.01 else Vector2.DOWN
	var eye := bob + look * 5
	draw_circle(eye + Vector2(-4, -2), 3.2, Color.WHITE)
	draw_circle(eye + Vector2(4, -2), 3.2, Color.WHITE)
	draw_circle(eye + Vector2(-4, -2) + look * 1.2, 1.6, accent)
	draw_circle(eye + Vector2(4, -2) + look * 1.2, 1.6, accent)
	if highlight:
		draw_arc(bob, 22, 0, TAU, 40, DungeonTheme.GLOW, 2.5)
	if held_weapon == "sword" or held_weapon == "bow":
		_draw_held_weapon(bob)


func _draw_potion() -> void:
	var bob := Vector2(0, _bounce * 0.6)
	draw_circle(Vector2(0, 8), 8, Color(DungeonTheme.SUCCESS, 0.18))
	draw_circle(bob + Vector2(0, 4), 9, DungeonTheme.SUCCESS.darkened(0.2))
	draw_circle(bob + Vector2(0, 3), 6, DungeonTheme.SUCCESS)
	draw_rect(Rect2(bob + Vector2(-3, -10), Vector2(6, 8)), DungeonTheme.STONE_HI)
	draw_circle(bob + Vector2(-2, 1), 2, Color(1, 1, 1, 0.45))
	if highlight:
		draw_arc(bob, 16, 0, TAU, 28, DungeonTheme.SUCCESS, 2.0)


func _draw_door() -> void:
	var col := DungeonTheme.SUCCESS if opened else DungeonTheme.GOLD
	draw_rect(Rect2(-18, -28, 36, 48), DungeonTheme.STONE)
	draw_rect(Rect2(-14, -24, 28, 40), col.darkened(0.25 if opened else 0.05))
	draw_arc(Vector2(0, -24), 14, PI, TAU, 16, col, 3.0)
	if not opened:
		draw_circle(Vector2(8, 0), 3, DungeonTheme.GOLD.lightened(0.3))
	if highlight:
		draw_rect(Rect2(-20, -30, 40, 52), Color(DungeonTheme.SUCCESS, 0.55), false, 2.0)


func _draw_chest() -> void:
	var lid := DungeonTheme.GOLD if opened else DungeonTheme.GOLD.darkened(0.15)
	draw_rect(Rect2(-14, -4, 28, 16), DungeonTheme.GOLD.darkened(0.35))
	draw_rect(Rect2(-14, -14, 28, 12), lid)
	draw_rect(Rect2(-3, -4, 6, 8), DungeonTheme.STONE_HI)
	if highlight:
		draw_arc(Vector2.ZERO, 22, 0, TAU, 28, DungeonTheme.GOLD, 2.0)


func _draw_exit() -> void:
	draw_rect(Rect2(-16, -16, 32, 32), DungeonTheme.GLOW.darkened(0.45))
	draw_rect(Rect2(-12, -12, 24, 24), Color("#0f3460"))
	for i in 3:
		var y := 8 - i * 8
		draw_rect(Rect2(-10, y, 20, 6), DungeonTheme.GLOW.darkened(0.2 + i * 0.1))
	if highlight:
		draw_arc(Vector2.ZERO, 24, 0, TAU, 28, DungeonTheme.GLOW, 2.0)


func _draw_stairs_up() -> void:
	draw_rect(Rect2(-16, -16, 32, 32), DungeonTheme.SUCCESS.darkened(0.45))
	draw_rect(Rect2(-12, -12, 24, 24), Color("#0b3d30"))
	for i in 3:
		var y := -10 + i * 8
		draw_rect(Rect2(-10, y, 20, 6), DungeonTheme.SUCCESS.darkened(0.1 + i * 0.08))
	if highlight:
		draw_arc(Vector2.ZERO, 24, 0, TAU, 28, DungeonTheme.SUCCESS, 2.0)


func _draw_wizard_hat() -> void:
	var bob := Vector2(0, _bounce - 18)
	var pts := PackedVector2Array([bob + Vector2(0, -14), bob + Vector2(-12, 6), bob + Vector2(12, 6)])
	draw_colored_polygon(pts, Color("#3d0066"))
	draw_circle(bob + Vector2(0, 6), 3, DungeonTheme.GOLD)


func _draw_arrow_loot() -> void:
	var bob := Vector2(0, _bounce * 0.4)
	draw_line(bob + Vector2(-12, 4), bob + Vector2(10, -6), DungeonTheme.GOLD, 2.2)
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(10, -6), bob + Vector2(4, -8), bob + Vector2(6, -2)
	]), DungeonTheme.ACCENT)
	draw_line(bob + Vector2(-12, 4), bob + Vector2(-8, 8), Color("#c9783a"), 1.6)
	draw_line(bob + Vector2(-12, 4), bob + Vector2(-16, 0), Color("#c9783a"), 1.6)


func _draw_key() -> void:
	var bob := Vector2(0, _bounce * 0.5)
	draw_circle(bob + Vector2(-6, 0), 6, DungeonTheme.GOLD)
	draw_circle(bob + Vector2(-6, 0), 2.5, DungeonTheme.BG)
	draw_rect(Rect2(bob + Vector2(-2, -2), Vector2(14, 4)), DungeonTheme.GOLD)
	draw_rect(Rect2(bob + Vector2(8, 2), Vector2(3, 5)), DungeonTheme.GOLD)


func _draw_held_weapon(bob: Vector2) -> void:
	var look := facing.normalized() if facing.length() > 0.01 else Vector2.DOWN
	if held_weapon == "sword":
		var swing := use_pose * 1.35
		var dir := look.rotated(-0.55 + swing)
		var hand := bob + look.rotated(-0.85) * 13.0
		var hilt := hand - dir * 3.0
		var tip := hand + dir * (15.0 + use_pose * 10.0)
		draw_line(hilt, tip, DungeonTheme.GLOW.lightened(0.15), 3.2)
		draw_line(hilt + dir.rotated(PI * 0.5) * 6.0, hilt + dir.rotated(-PI * 0.5) * 6.0, DungeonTheme.GOLD, 2.4)
		draw_circle(hilt, 2.2, Color("#8d6e63"))
		if use_pose > 0.15:
			draw_arc(bob, 22.0 + use_pose * 6.0, dir.angle() - 0.5, dir.angle() + 0.15, 10, Color(DungeonTheme.GLOW, 0.45 * use_pose), 2.0)
	else:
		var pull := use_pose
		var center := bob + look * (11.0 + pull * 2.0)
		var perp := look.rotated(PI * 0.5)
		var ang := look.angle()
		draw_arc(center, 13.0, ang - 1.15, ang + 1.15, 18, Color("#c9783a"), 2.8)
		var nock := center - look * (3.0 + pull * 7.0)
		draw_line(center + perp * 12.0, nock, DungeonTheme.TEXT, 1.3)
		draw_line(center - perp * 12.0, nock, DungeonTheme.TEXT, 1.3)
		if pull > 0.05:
			draw_line(nock, nock + look * (10.0 + pull * 6.0), DungeonTheme.GOLD, 2.0)
			draw_circle(nock + look * 2.0, 2.0, DungeonTheme.GOLD.lightened(0.2))


## Brief attack pose — triggered from player.play_weapon_use and goblin attack/cast.
func play_use() -> void:
	use_pose = 0.0
	var tween := create_tween()
	tween.tween_property(self, "use_pose", 1.0, 0.08).set_trans(Tween.TRANS_QUAD)
	tween.tween_property(self, "use_pose", 0.0, 0.22).set_trans(Tween.TRANS_BACK)


func _draw_sword() -> void:
	var bob := Vector2(0, _bounce * 0.4)
	draw_rect(Rect2(bob + Vector2(-2, -16), Vector2(4, 22)), DungeonTheme.GLOW)
	draw_rect(Rect2(bob + Vector2(-8, 4), Vector2(16, 4)), DungeonTheme.GOLD)
	draw_rect(Rect2(bob + Vector2(-3, 8), Vector2(6, 8)), Color("#8d6e63"))
	draw_circle(bob + Vector2(0, -16), 3, DungeonTheme.GLOW.lightened(0.3))


func _draw_bow() -> void:
	var bob := Vector2(0, _bounce * 0.4)
	draw_arc(bob, 14, -0.7, 0.7 + PI, 18, Color("#c9783a"), 3.0)
	draw_line(bob + Vector2(0, -13), bob + Vector2(0, 13), DungeonTheme.TEXT, 1.5)
	draw_rect(Rect2(bob + Vector2(6, -1), Vector2(10, 2)), DungeonTheme.GOLD)


func _draw_armor() -> void:
	var bob := Vector2(0, _bounce * 0.3)
	draw_circle(bob + Vector2(0, -8), 8, DungeonTheme.STONE_HI)
	draw_rect(Rect2(bob + Vector2(-12, -4), Vector2(24, 18)), DungeonTheme.GLOW.darkened(0.35))
	draw_rect(Rect2(bob + Vector2(-10, -2), Vector2(20, 6)), DungeonTheme.GLOW.darkened(0.15))


func _draw_treasure() -> void:
	var bob := Vector2(0, _bounce * 0.5)
	draw_circle(bob, 12, DungeonTheme.GOLD)
	draw_circle(bob + Vector2(-4, -3), 3, Color(1, 1, 1, 0.55))
	draw_rect(Rect2(bob + Vector2(-8, 4), Vector2(16, 6)), DungeonTheme.GOLD.darkened(0.25))
	if highlight:
		draw_arc(bob, 18, 0, TAU, 28, DungeonTheme.GOLD, 2.0)
