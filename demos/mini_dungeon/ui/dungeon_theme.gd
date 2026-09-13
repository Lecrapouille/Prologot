## Mini Dungeon palette and HUD theme (colors shared by procedural drawing).
extends RefCounted
class_name DungeonTheme

const BG := Color("#12121c")
const FLOOR := Color("#1e1e32")
const FLOOR_ALT := Color("#252544")
const STONE := Color("#2b2d42")
const STONE_HI := Color("#3d405b")
const STONE_SHADOW := Color("#16161f")
const ACCENT := Color("#e94560")
const SUCCESS := Color("#16c79a")
const GOLD := Color("#f4a261")
const GLOW := Color("#53d8fb")
const PLAYER := Color("#5dade2")
const GOBLIN := Color("#6ab04c")
const TEXT := Color("#eaeaea")
const TEXT_DIM := Color("#8b8ba3")


## Attach HUD theme to the PanelContainer in dungeon.tscn.
static func apply(root: Control) -> void:
	var theme := Theme.new()
	theme.set_color("font_color", "Label", TEXT)
	theme.set_font_size("font_size", "Label", 15)
	theme.set_stylebox("background", "ProgressBar", _bar(Color(0, 0, 0, 0.45)))
	theme.set_stylebox("fill", "ProgressBar", _bar(ACCENT))
	theme.set_stylebox("panel", "PanelContainer", _panel())
	root.theme = theme


static func _bar(color: Color) -> StyleBoxFlat:
	var box := StyleBoxFlat.new()
	box.bg_color = color
	box.corner_radius_top_left = 5
	box.corner_radius_top_right = 5
	box.corner_radius_bottom_left = 5
	box.corner_radius_bottom_right = 5
	return box


static func _panel() -> StyleBoxFlat:
	var box := StyleBoxFlat.new()
	box.bg_color = Color("#16213e")
	box.corner_radius_top_left = 12
	box.corner_radius_top_right = 12
	box.corner_radius_bottom_left = 12
	box.corner_radius_bottom_right = 12
	box.border_width_left = 1
	box.border_width_top = 1
	box.border_width_right = 1
	box.border_width_bottom = 1
	box.border_color = Color(GLOW, 0.28)
	box.shadow_color = Color(0, 0, 0, 0.4)
	box.shadow_size = 8
	box.content_margin_left = 14
	box.content_margin_top = 10
	box.content_margin_right = 14
	box.content_margin_bottom = 10
	return box
