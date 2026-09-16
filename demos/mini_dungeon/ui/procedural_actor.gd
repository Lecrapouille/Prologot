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
			_draw_creature(DungeonTheme.GOBLIN, Color("#1e2a14"))
		Kind.WIZARD:
			_draw_creature(Color("#9b5de5"), Color("#f72585"))
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


func _twinkle() -> float:
	return 0.55 + 0.45 * sin(_bounce)


func _draw_ground_shadow(center: Vector2, radius: float, alpha: float = 0.30) -> void:
	draw_circle(center, radius, Color(0, 0, 0, alpha))


func _draw_sparkles(origin: Vector2, color: Color) -> void:
	var t := _twinkle()
	draw_circle(origin + Vector2(-16, -14), 1.5 * t, Color(color, 0.75))
	draw_circle(origin + Vector2(15, -10), 1.15 * t, Color(color, 0.5))
	draw_circle(origin + Vector2(4, -20), 1.0 * t, Color(color, 0.4))


func _look() -> Vector2:
	return facing.normalized() if facing.length() > 0.01 else Vector2.DOWN


func _draw_creature(body: Color, accent: Color) -> void:
	var bob := Vector2(0, _bounce)
	var look := _look()
	var perp := look.rotated(PI * 0.5)
	_draw_ground_shadow(Vector2(0, 12), 14, 0.32)

	if kind == Kind.PLAYER:
		var cape := PackedVector2Array([
			bob - look * 2.0,
			bob - look * 18.0 + perp * 11.0 + Vector2(0, 6),
			bob - look * 20.0 + Vector2(0, 8),
			bob - look * 18.0 - perp * 11.0 + Vector2(0, 6),
		])
		draw_colored_polygon(cape, Color("#1d4e89"))
		draw_colored_polygon(PackedVector2Array([
			cape[0], cape[1], cape[2]
		]), Color("#2563a8"))
	elif kind == Kind.GOBLIN:
		for side in [-1.0, 1.0]:
			var ear := PackedVector2Array([
				bob + perp * side * 9.0 + Vector2(0, -2),
				bob + perp * side * 18.0 + look * -2.0 + Vector2(0, -9),
				bob + perp * side * 7.0 + Vector2(0, -8),
			])
			draw_colored_polygon(ear, body.darkened(0.18))
			draw_line(ear[0], ear[1], body.lightened(0.08), 1.0)
	elif kind == Kind.WIZARD:
		var robe := PackedVector2Array([
			bob + Vector2(0, 2),
			bob - perp * 15.0 + Vector2(0, 16),
			bob + Vector2(0, 18),
			bob + perp * 15.0 + Vector2(0, 16),
		])
		draw_colored_polygon(robe, Color("#3d0066"))
		draw_colored_polygon(PackedVector2Array([robe[0], robe[1], robe[2]]), Color("#5a189a"))

	draw_circle(bob + Vector2(1, 3), 13.5, body.darkened(0.28))
	draw_circle(bob, 14.5, body)
	draw_circle(bob + Vector2(-3.5, -3.5), 6.5, body.lightened(0.16))
	draw_circle(bob + look * 1.5 + Vector2(0, 3), 7.5, body.lightened(0.06))

	if kind == Kind.PLAYER:
		draw_arc(bob + Vector2(0, -2), 13.2, PI * 1.05, PI * 1.95, 18, Color("#7f8c9b"), 3.4)
		draw_rect(Rect2(bob + Vector2(-11, -6), Vector2(22, 5)), Color("#95a5b2"))
		draw_rect(Rect2(bob + Vector2(-10, -4), Vector2(20, 2)), Color("#2c3e50"))
	elif kind == Kind.GOBLIN:
		draw_circle(bob + look * 6.0, 4.2, body.darkened(0.12))
		for side in [-1.0, 1.0]:
			draw_line(bob + look * 7.0 + perp * side * 2.2, bob + look * 10.0 + perp * side * 3.4, Color("#dfe6e9"), 1.6)

	var eye := bob + look * 5.0
	var white := Color("#f4f7fb") if kind != Kind.GOBLIN else Color("#f7e27a")
	draw_circle(eye + perp * -4.2 + Vector2(0, -2), 3.3, white)
	draw_circle(eye + perp * 4.2 + Vector2(0, -2), 3.3, white)
	draw_circle(eye + perp * -4.2 + Vector2(0, -2) + look * 1.3, 1.55, accent)
	draw_circle(eye + perp * 4.2 + Vector2(0, -2) + look * 1.3, 1.55, accent)
	draw_circle(eye + perp * -4.6 + Vector2(-0.8, -3.1), 0.8, Color(1, 1, 1, 0.55))
	draw_circle(eye + perp * 3.8 + Vector2(-0.8, -3.1), 0.8, Color(1, 1, 1, 0.55))

	if kind == Kind.WIZARD:
		_draw_wizard_hat()

	if highlight:
		draw_arc(bob, 22, 0, TAU, 40, DungeonTheme.GLOW, 2.5)
		_draw_sparkles(bob, DungeonTheme.GLOW)
	if held_weapon == "sword" or held_weapon == "bow":
		_draw_held_weapon(bob)


func _draw_potion() -> void:
	var bob := Vector2(0, _bounce * 0.6)
	_draw_ground_shadow(Vector2(0, 12), 9, 0.26)
	draw_circle(bob + Vector2(0, 4), 13, Color(DungeonTheme.SUCCESS, 0.16))
	var glass := Color("#b8f3de")
	var liquid := DungeonTheme.SUCCESS
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(-7, -1), bob + Vector2(7, -1),
		bob + Vector2(8, 10), bob + Vector2(-8, 10),
	]), glass.darkened(0.35))
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(-6, 1), bob + Vector2(6, 1),
		bob + Vector2(7, 9), bob + Vector2(-7, 9),
	]), liquid)
	draw_circle(bob + Vector2(-3, 4), 2.2, Color(1, 1, 1, 0.42))
	draw_circle(bob + Vector2(2, 6), 1.1, Color(1, 1, 1, 0.28))
	draw_rect(Rect2(bob + Vector2(-3.2, -9), Vector2(6.4, 9)), glass.darkened(0.2))
	draw_rect(Rect2(bob + Vector2(-2.2, -8), Vector2(2.2, 7)), Color(1, 1, 1, 0.28))
	draw_rect(Rect2(bob + Vector2(-4.5, -12), Vector2(9, 4)), Color("#8d6e63"))
	draw_rect(Rect2(bob + Vector2(-3.5, -13), Vector2(7, 2.4)), Color("#c9a27a"))
	if highlight:
		draw_arc(bob, 16, 0, TAU, 28, DungeonTheme.SUCCESS, 2.0)
		_draw_sparkles(bob, DungeonTheme.SUCCESS)


func _draw_door() -> void:
	# Leaf fills the alcove gap (24x56); walls already provide the stone frame.
	var wood := Color("#8b4e24")
	var wood_hi := Color("#c2783c")
	var wood_lo := Color("#5a3418")
	var iron := Color("#6d7278")
	var leaf := Rect2(-12, -28, 24, 56)
	if opened:
		draw_rect(leaf, Color("#0a0a12"))
		draw_rect(Rect2(-12, -28, 5, 56), wood_lo)
		draw_rect(Rect2(-11, -27, 3, 54), wood)
		draw_rect(Rect2(7, -28, 5, 56), wood_lo)
		draw_rect(Rect2(8, -27, 3, 54), wood)
	else:
		draw_rect(Rect2(leaf.position + Vector2(1, 2), leaf.size), wood_lo)
		draw_rect(leaf, wood)
		for i in 5:
			var x := leaf.position.x + 4 + i * 4
			draw_line(Vector2(x, leaf.position.y + 2), Vector2(x, leaf.end.y - 2), wood_lo.lightened(0.08), 1.0)
		draw_rect(Rect2(-12, -12, 24, 3), iron)
		draw_rect(Rect2(-12, 8, 24, 3), iron)
		draw_circle(Vector2(7, 0), 3.0, DungeonTheme.GOLD.darkened(0.2))
		draw_circle(Vector2(7, 0), 2.0, DungeonTheme.GOLD)
		draw_circle(Vector2(7, 0), 0.7, wood_lo)
		draw_rect(Rect2(-11, -27, 4, 10), wood_hi.darkened(0.05))
	if highlight:
		draw_rect(leaf.grow(2), Color(DungeonTheme.SUCCESS, 0.75), false, 2.0)


func _draw_chest() -> void:
	var bob := Vector2(0, _bounce * 0.18)
	var wood := Color("#9a5c2e")
	var wood_hi := Color("#d08a4a")
	var wood_lo := Color("#5a3418")
	var band := DungeonTheme.GOLD
	var band_lo := DungeonTheme.GOLD.darkened(0.38)
	var w := 36.0
	var left := -w * 0.5

	_draw_ground_shadow(Vector2(0, 14), 15, 0.32)

	var body := Rect2(bob + Vector2(left, 0), Vector2(w, 16))
	draw_rect(Rect2(body.position + Vector2(1, 2), body.size), wood_lo)
	draw_rect(body, wood)
	for i in 5:
		var x := body.position.x + 4 + i * 7
		draw_line(Vector2(x, body.position.y + 1), Vector2(x, body.end.y - 1), wood_lo.lightened(0.12), 1.0)
	draw_rect(Rect2(body.position.x, body.position.y + 2, w, 3), band_lo)
	draw_rect(Rect2(body.position.x, body.position.y + 2, w, 2), band)
	draw_rect(Rect2(body.position.x, body.end.y - 4, w, 3), band_lo)
	draw_rect(Rect2(body.position.x, body.end.y - 4, w, 2), band)
	for p in [Vector2(left + 3, 4), Vector2(w * 0.5 - 3, 4), Vector2(left + 3, 13), Vector2(w * 0.5 - 3, 13)]:
		draw_circle(bob + p, 1.5, band.lightened(0.3))

	if opened:
		var lid := Rect2(bob + Vector2(left, -22), Vector2(w, 13))
		draw_rect(Rect2(lid.position + Vector2(1, 1), lid.size), wood_lo)
		draw_rect(lid, wood_hi)
		draw_rect(Rect2(lid.position.x, lid.end.y - 3, w, 3), band)
		draw_rect(Rect2(bob + Vector2(left + 3, 1), Vector2(w - 6, 7)), Color("#2a1810"))
		draw_circle(bob + Vector2(-5, 4), 2.6, band.lightened(0.25))
		draw_circle(bob + Vector2(2, 3), 2.1, band)
		draw_circle(bob + Vector2(7, 5), 1.6, band.darkened(0.12))
	else:
		var lid := Rect2(bob + Vector2(left, -15), Vector2(w, 16))
		draw_rect(lid, wood_hi)
		draw_rect(Rect2(bob + Vector2(left + 2, -18), Vector2(w - 4, 6)), wood_hi.lightened(0.1))
		draw_rect(Rect2(bob + Vector2(left, -4), Vector2(w, 3)), band_lo)
		draw_rect(Rect2(bob + Vector2(left, -3), Vector2(w, 2)), band.lightened(0.12))
		draw_rect(Rect2(bob + Vector2(-6, -4), Vector2(12, 10)), band_lo)
		draw_rect(Rect2(bob + Vector2(-5, -3), Vector2(10, 8)), band)
		draw_circle(bob + Vector2(0, 1), 2.1, wood_lo)
		draw_rect(Rect2(bob + Vector2(-1.1, 1), Vector2(2.2, 3.2)), wood_lo)

	if highlight:
		draw_arc(bob + Vector2(0, 2), 27, 0, TAU, 36, band.lightened(0.25), 2.2)
		_draw_sparkles(bob, band.lightened(0.2))


func _draw_stone_frame(origin: Vector2, size: Vector2) -> void:
	var r := Rect2(origin, size)
	draw_rect(r.grow(3), DungeonTheme.STONE.darkened(0.18))
	draw_rect(r, DungeonTheme.STONE)
	draw_rect(Rect2(r.position, Vector2(r.size.x, 3)), DungeonTheme.STONE_HI)
	draw_rect(Rect2(r.position + Vector2(0, r.size.y - 4), Vector2(r.size.x, 4)), DungeonTheme.STONE_SHADOW)
	for corner in [Vector2(0, 0), Vector2(r.size.x - 6, 0), Vector2(0, r.size.y - 6), Vector2(r.size.x - 6, r.size.y - 6)]:
		draw_rect(Rect2(r.position + corner, Vector2(6, 6)), DungeonTheme.STONE_HI.darkened(0.05))


func _draw_chevron(center: Vector2, up: bool, color: Color) -> void:
	var dir := -1.0 if up else 1.0
	draw_colored_polygon(PackedVector2Array([
		center + Vector2(0, dir * 7),
		center + Vector2(-6, dir * -1),
		center + Vector2(6, dir * -1),
	]), color)
	draw_colored_polygon(PackedVector2Array([
		center + Vector2(0, dir * 2),
		center + Vector2(-6, dir * -6),
		center + Vector2(6, dir * -6),
	]), color.darkened(0.15))


func _draw_exit() -> void:
	var bob := Vector2(0, _bounce * 0.08)
	var stone := DungeonTheme.STONE
	var glow := DungeonTheme.GLOW
	_draw_ground_shadow(Vector2(4, 22), 20, 0.34)
	_draw_stone_frame(bob + Vector2(-28, -24), Vector2(56, 50))
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(-22, -18), bob + Vector2(22, -18),
		bob + Vector2(18, 20), bob + Vector2(-18, 20),
	]), Color("#071018"))
	draw_circle(bob + Vector2(0, 16), 10, Color(glow, 0.16))
	for i in 6:
		var t := i / 5.0
		var half := 20.0 - i * 2.4
		var y := -16.0 + i * 6.2
		var shift := i * 1.4
		var top := glow.darkened(0.18 + t * 0.42).lerp(stone, t * 0.25)
		draw_rect(Rect2(bob + Vector2(-half + shift, y), Vector2(half * 2.0, 5.4)), top)
		draw_rect(Rect2(bob + Vector2(-half + shift, y + 4.0), Vector2(half * 2.0, 2.2)), top.darkened(0.32))
		draw_line(bob + Vector2(-half + shift + 2, y + 1), bob + Vector2(half + shift - 2, y + 1), top.lightened(0.2), 1.0)
	draw_line(bob + Vector2(-21, -17), bob + Vector2(-16, 20), Color("#4b5563"), 2.4)
	draw_line(bob + Vector2(21, -17), bob + Vector2(16, 20), Color("#4b5563"), 2.4)
	_draw_chevron(bob + Vector2(0, 2), false, glow.lightened(0.1))
	if highlight:
		draw_arc(bob + Vector2(0, 2), 30, 0, TAU, 36, glow, 2.2)
		_draw_sparkles(bob, glow)


func _draw_stairs_up() -> void:
	var bob := Vector2(0, _bounce * 0.08)
	var stone := DungeonTheme.STONE
	var glow := DungeonTheme.SUCCESS
	_draw_ground_shadow(Vector2(0, 22), 18, 0.32)
	_draw_stone_frame(bob + Vector2(-28, -26), Vector2(56, 52))
	draw_rect(Rect2(bob + Vector2(-18, -22), Vector2(36, 16)), Color("#0b3d30"))
	draw_circle(bob + Vector2(0, -18), 11, Color(glow, 0.22))
	draw_arc(bob + Vector2(0, -8), 16, PI, TAU, 16, stone, -1.0)
	draw_arc(bob + Vector2(0, -8), 16, PI, TAU, 16, DungeonTheme.STONE_HI, 3.0)
	for i in 6:
		var t := i / 5.0
		var half := 10.0 + i * 2.6
		var y := -12.0 + i * 6.4
		var top := glow.darkened(0.08 + (1.0 - t) * 0.28).lerp(stone.lightened(0.08), 0.35)
		draw_rect(Rect2(bob + Vector2(-half, y), Vector2(half * 2.0, 5.6)), top)
		draw_rect(Rect2(bob + Vector2(-half, y + 4.2), Vector2(half * 2.0, 2.2)), top.darkened(0.28))
		draw_line(bob + Vector2(-half + 2, y + 1), bob + Vector2(half - 2, y + 1), top.lightened(0.22), 1.0)
	draw_line(bob + Vector2(-22, 20), bob + Vector2(-16, -10), Color("#6b7280"), 2.6)
	draw_line(bob + Vector2(22, 20), bob + Vector2(16, -10), Color("#6b7280"), 2.6)
	_draw_chevron(bob + Vector2(0, -2), true, glow.lightened(0.12))
	if highlight:
		draw_arc(bob + Vector2(0, 2), 30, 0, TAU, 36, glow, 2.2)
		_draw_sparkles(bob, glow)


func _draw_wizard_hat() -> void:
	var bob := Vector2(0, _bounce - 18)
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -16), bob + Vector2(-13, 7), bob + Vector2(13, 7)
	]), Color("#240046"))
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -16), bob + Vector2(-4, 6), bob + Vector2(13, 7)
	]), Color("#5a189a"))
	draw_rect(Rect2(bob + Vector2(-14, 5), Vector2(28, 4)), Color("#3d0066"))
	draw_rect(Rect2(bob + Vector2(-14, 5), Vector2(28, 1.6)), DungeonTheme.GOLD)
	draw_circle(bob + Vector2(0, -2), 2.2, DungeonTheme.GOLD)
	draw_circle(bob + Vector2(-6, 2), 1.1, Color("#f72585"))
	draw_circle(bob + Vector2(5, 1), 0.9, Color("#ffe66d"))


func _draw_arrow_loot() -> void:
	var bob := Vector2(0, _bounce * 0.4)
	_draw_ground_shadow(Vector2(0, 10), 8, 0.22)
	draw_rect(Rect2(bob + Vector2(-5, -2), Vector2(10, 12)), Color("#6b3e22"))
	draw_rect(Rect2(bob + Vector2(-4, -1), Vector2(8, 10)), Color("#8d5a32"))
	draw_rect(Rect2(bob + Vector2(-4, 2), Vector2(8, 2)), Color("#c9783a"))
	for i in 3:
		var x := -3 + i * 3
		draw_line(bob + Vector2(x, -10), bob + Vector2(x, 2), Color("#d9c27a"), 1.3)
		draw_colored_polygon(PackedVector2Array([
			bob + Vector2(x, -12), bob + Vector2(x - 2, -8), bob + Vector2(x + 2, -8)
		]), DungeonTheme.GLOW)
	if highlight:
		_draw_sparkles(bob, DungeonTheme.GOLD)


func _draw_key() -> void:
	var bob := Vector2(0, _bounce * 0.5)
	var gold := DungeonTheme.GOLD
	_draw_ground_shadow(Vector2(0, 10), 8, 0.22)
	draw_circle(bob + Vector2(-7, 0), 7.2, gold.darkened(0.28))
	draw_circle(bob + Vector2(-7, 0), 6.2, gold)
	draw_circle(bob + Vector2(-8.5, -2), 2.0, gold.lightened(0.35))
	draw_circle(bob + Vector2(-7, 0), 2.6, DungeonTheme.BG)
	draw_rect(Rect2(bob + Vector2(-2, -2.1), Vector2(16, 4.2)), gold.darkened(0.15))
	draw_rect(Rect2(bob + Vector2(-2, -1.4), Vector2(16, 2.6)), gold)
	draw_rect(Rect2(bob + Vector2(9, 1.6), Vector2(3.2, 6)), gold)
	draw_rect(Rect2(bob + Vector2(13, 1.6), Vector2(2.4, 4)), gold)
	draw_circle(bob + Vector2(4, -0.2), 1.1, gold.lightened(0.4))
	if highlight:
		draw_arc(bob, 16, 0, TAU, 24, gold, 1.6)
		_draw_sparkles(bob, gold)


func _draw_held_weapon(bob: Vector2) -> void:
	var look := _look()
	if held_weapon == "sword":
		var swing := use_pose * 1.35
		var dir := look.rotated(-0.55 + swing)
		var hand := bob + look.rotated(-0.85) * 13.0
		var hilt := hand - dir * 3.0
		var tip := hand + dir * (15.0 + use_pose * 10.0)
		draw_line(hilt, tip, DungeonTheme.GLOW.darkened(0.15), 4.0)
		draw_line(hilt, tip, DungeonTheme.GLOW.lightened(0.2), 2.2)
		draw_line(hilt + dir.rotated(PI * 0.5) * 6.0, hilt + dir.rotated(-PI * 0.5) * 6.0, DungeonTheme.GOLD, 2.6)
		draw_circle(hilt, 2.3, Color("#8d6e63"))
		if use_pose > 0.15:
			draw_arc(bob, 22.0 + use_pose * 6.0, dir.angle() - 0.5, dir.angle() + 0.15, 10, Color(DungeonTheme.GLOW, 0.45 * use_pose), 2.0)
	else:
		var pull := use_pose
		var center := bob + look * (11.0 + pull * 2.0)
		var perp := look.rotated(PI * 0.5)
		var ang := look.angle()
		draw_arc(center, 13.0, ang - 1.15, ang + 1.15, 18, Color("#8d5a32"), 3.4)
		draw_arc(center, 13.0, ang - 1.15, ang + 1.15, 18, Color("#c9783a"), 2.0)
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
	_draw_ground_shadow(Vector2(0, 12), 8, 0.22)
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -20), bob + Vector2(-4, 2), bob + Vector2(4, 2)
	]), DungeonTheme.GLOW.darkened(0.2))
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -20), bob + Vector2(-1.2, 2), bob + Vector2(4, 2)
	]), DungeonTheme.GLOW.lightened(0.18))
	draw_line(bob + Vector2(0.6, -18), bob + Vector2(0.6, 1), Color(1, 1, 1, 0.4), 1.0)
	draw_rect(Rect2(bob + Vector2(-9, 2), Vector2(18, 4)), DungeonTheme.GOLD.darkened(0.2))
	draw_rect(Rect2(bob + Vector2(-8, 2.6), Vector2(16, 2.6)), DungeonTheme.GOLD)
	draw_rect(Rect2(bob + Vector2(-3, 6), Vector2(6, 9)), Color("#6d4c41"))
	draw_rect(Rect2(bob + Vector2(-2, 7), Vector2(2, 7)), Color("#a1887f"))
	draw_circle(bob + Vector2(0, 16), 3.2, DungeonTheme.GOLD)
	draw_circle(bob + Vector2(0, 16), 1.4, DungeonTheme.GLOW)
	if highlight:
		_draw_sparkles(bob, DungeonTheme.GLOW)


func _draw_bow() -> void:
	var bob := Vector2(0, _bounce * 0.4)
	_draw_ground_shadow(Vector2(0, 12), 8, 0.22)
	draw_arc(bob, 15, -0.75, 0.75 + PI, 22, Color("#5a3418"), 4.4)
	draw_arc(bob, 15, -0.75, 0.75 + PI, 22, Color("#c9783a"), 2.6)
	draw_arc(bob + Vector2(1, 0), 13.5, -0.55, 0.55 + PI, 16, Color("#e8b86d"), 1.2)
	draw_line(bob + Vector2(0, -14), bob + Vector2(0, 14), DungeonTheme.TEXT.darkened(0.15), 1.4)
	draw_rect(Rect2(bob + Vector2(-3, -3), Vector2(6, 6)), Color("#8d6e63"))
	draw_rect(Rect2(bob + Vector2(7, -1.2), Vector2(11, 2.4)), DungeonTheme.GOLD)
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(18, 0), bob + Vector2(13, -3), bob + Vector2(13, 3)
	]), DungeonTheme.GLOW)
	if highlight:
		_draw_sparkles(bob, DungeonTheme.GOLD)


func _draw_armor() -> void:
	var bob := Vector2(0, _bounce * 0.3)
	_draw_ground_shadow(Vector2(0, 16), 13, 0.3)
	var steel := Color("#8ea4b5")
	var steel_hi := Color("#d7e4ee")
	var steel_lo := Color("#4b5d6b")
	var trim := DungeonTheme.GOLD
	draw_rect(Rect2(bob + Vector2(-2.4, 14), Vector2(4.8, 8)), Color("#5d4037"))
	draw_rect(Rect2(bob + Vector2(-7, 20), Vector2(14, 3)), Color("#4e342e"))
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -6), bob + Vector2(-13, 1), bob + Vector2(-11, 15),
		bob + Vector2(0, 18), bob + Vector2(11, 15), bob + Vector2(13, 1)
	]), steel_lo)
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -5), bob + Vector2(-11, 2), bob + Vector2(-9, 14),
		bob + Vector2(0, 16), bob + Vector2(9, 14), bob + Vector2(11, 2)
	]), steel)
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -5), bob + Vector2(-3, 3), bob + Vector2(11, 2)
	]), steel_hi)
	draw_line(bob + Vector2(0, -4), bob + Vector2(0, 15), steel_hi, 1.6)
	draw_rect(Rect2(bob + Vector2(-8, 5), Vector2(16, 3)), trim.darkened(0.15))
	draw_rect(Rect2(bob + Vector2(-7, 5.6), Vector2(14, 1.8)), trim)
	for px in [-6.0, 0.0, 6.0]:
		draw_circle(bob + Vector2(px, 6.5), 1.1, trim.lightened(0.25))
	draw_circle(bob + Vector2(-13, 1), 5.2, steel_lo)
	draw_circle(bob + Vector2(13, 1), 5.2, steel_lo)
	draw_circle(bob + Vector2(-13, 0), 4.1, steel)
	draw_circle(bob + Vector2(13, 0), 4.1, steel)
	draw_circle(bob + Vector2(-14, -1), 1.6, steel_hi)
	draw_circle(bob + Vector2(12, -1), 1.6, steel_hi)
	draw_circle(bob + Vector2(0, -12), 8.6, steel_lo)
	draw_circle(bob + Vector2(0, -13), 7.4, steel)
	draw_circle(bob + Vector2(-2, -15), 3.2, steel_hi)
	draw_rect(Rect2(bob + Vector2(-6, -12), Vector2(12, 3.2)), Color("#2c3e50"))
	draw_rect(Rect2(bob + Vector2(-1.2, -21), Vector2(2.4, 6)), trim)
	draw_circle(bob + Vector2(0, -21), 1.8, trim.lightened(0.2))
	if highlight:
		draw_arc(bob, 22, 0, TAU, 28, DungeonTheme.GLOW, 1.8)
		_draw_sparkles(bob, DungeonTheme.GLOW)


func _draw_treasure() -> void:
	var bob := Vector2(0, _bounce * 0.5)
	_draw_ground_shadow(Vector2(0, 12), 12, 0.28)
	draw_circle(bob + Vector2(0, 6), 10, DungeonTheme.GOLD.darkened(0.35))
	for p in [Vector2(-7, 7), Vector2(0, 8), Vector2(7, 6), Vector2(-3, 10), Vector2(4, 10)]:
		draw_circle(bob + p, 3.1, DungeonTheme.GOLD.darkened(0.12))
		draw_circle(bob + p + Vector2(-0.8, -0.8), 1.1, DungeonTheme.GOLD.lightened(0.25))
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -12), bob + Vector2(-8, -2), bob + Vector2(8, -2)
	]), Color("#4cc9f0"))
	draw_colored_polygon(PackedVector2Array([
		bob + Vector2(0, -12), bob + Vector2(-2, -2), bob + Vector2(8, -2)
	]), Color("#90e0ef"))
	draw_circle(bob + Vector2(-2, -6), 1.4, Color(1, 1, 1, 0.65))
	if highlight:
		draw_arc(bob, 18, 0, TAU, 28, DungeonTheme.GOLD, 2.0)
	_draw_sparkles(bob, DungeonTheme.GOLD)
