extends Control

# Script to display alien agents from the sprite sheet
@onready var sprite: TextureRect = $MarginContainer/Sprite
@onready var margin_container: MarginContainer = $MarginContainer
@onready var atlas = AtlasTexture.new()

# Dictionary of regions for each character
# Key = character name, Value = Rect2(x, y, width, height)
@export var character_regions: Dictionary = {
	"zorglub": Rect2(432, 27, 91, 180),
	"bleep": Rect2(573, 227, 125, 156),
	"glorp": Rect2(138, 214, 132, 165),
	"xylox": Rect2(284, 216, 115, 163),
	"nebula": Rect2(276, 14, 139, 199),
}

func _ready():
	# Wait for layout to be ready
	await get_tree().process_frame
	_update_sprite_size()

func _notification(what):
	if what == NOTIFICATION_RESIZED:
		_update_sprite_size()

func _update_sprite_size():
	# Calculate ratio based on available height in MarginContainer and texture height
	var available_height = margin_container.size.y
	var texture_height = 180.0 # Height of the texture region
	var ratio = available_height / texture_height if texture_height > 0 else 1.0

	# Calculate sprite size based on ratio (maintain aspect ratio)
	var sprite_width = 91.0 * ratio
	var sprite_height = texture_height * ratio

	# Set minimum size and center the sprite
	sprite.custom_minimum_size = Vector2(sprite_width, sprite_height)
	sprite.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	sprite.size_flags_vertical = Control.SIZE_SHRINK_CENTER

	# Load the sprite sheet if not already loaded
	if not atlas.atlas:
		atlas.atlas = load("res://assets/assets-2.png")
		sprite.texture = atlas

# Change character by name
func set_character(character_name: String):
	if not character_regions.has(character_name):
		push_error("Character '%s' not found!" % character_name)
		return
	atlas.region = character_regions[character_name]

func set_modulate_color(color: Color):
	"""Change la couleur de modulation pour indiquer l'état (danger, safe, etc.)"""
	if sprite:
		sprite.modulate = color
