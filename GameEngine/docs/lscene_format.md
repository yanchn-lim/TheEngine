# Ludus Scene Format

Status: Approved  
File extension: `.lscene`  
Current version: 1

This document is the source of truth for the Ludus scene file format.
Implementations must follow this specification unless the format version is changed.

## Design goals

- Keep scene files easy to read and edit by hand.
- Keep the parser small and strict.
- Store entity and component data outside C++ setup code.
- Use stable text IDs for assets and entities.
- Keep executable systems in C++.
- Report invalid data with file, line, and column information.

The format uses indentation but is not YAML. It does not implement YAML features.

## Complete example

```text
scene "Maxwell Test"
version: 1

assets
	shaders
		standard
			vertex: "assets/shaders/standard_gl.vert"
			fragment: "assets/shaders/standard_gl.frag"
			vertex_spirv: "assets/shaders/standard_vk.vert.spv"
			fragment_spirv: "assets/shaders/standard_vk.frag.spv"

	textures
		maxwell
			source: "assets/textures/maxwell.png"

		steak
			source: "assets/textures/steak.png"

	materials
		maxwell
			shader: standard
			texture: maxwell
			depth_test: true
			depth_write: true
			blend: none
			culling: true

		steak
			shader: standard
			texture: steak
			depth_test: true
			depth_write: true
			blend: none
			culling: true

	meshes
		maxwell
			source: "assets/models/maxwell.obj"

entities
	maxwell_left
		name: "Maxwell Left"

		components
			Transform
				position: [-0.85, -0.47, 0.25]
				rotation_degrees: [0.0, 0.0, 0.0]
				scale: [0.004, 0.004, 0.004]

			Renderable
				mesh: maxwell
				material_override: maxwell
				visible: true

			Rotator
				axis: [0.0, 1.0, 0.0]
				speed_degrees: 34.377

	maxwell_right
		name: "Maxwell Right"

		components
			Transform
				position: [0.85, -0.47, -0.25]
				rotation_degrees: [0.0, 0.0, 0.0]
				scale: [0.004, 0.004, 0.004]

			Renderable
				mesh: maxwell
				material_override: steak
				visible: true

			Rotator
				axis: [0.0, 1.0, 0.0]
				speed_degrees: -34.377
```

## Document declaration

Every file starts with these required lines:

```text
scene "Scene Name"
version: 1
```

The first line identifies the document as a Ludus scene. The version controls schema compatibility.

## Indentation

Version 1 uses one tab per indentation level.

```text
assets
	meshes
		maxwell
```

Rules:

- Leading spaces are invalid.
- Mixed indentation is invalid.
- Indentation can increase by only one level at a time.
- Empty lines are allowed.
- Trailing whitespace is ignored.

## Line types

The format has four line types.

Document declaration:

```text
scene "Maxwell Test"
```

Block:

```text
assets
```

Named block:

```text
maxwell
```

Property:

```text
position: [0.0, 0.0, 0.0]
```

Blocks and named blocks use the same syntax. Their meaning comes from their parent.

## Grammar

```text
document          := scene-declaration newline version-property newline statement*
scene-declaration := "scene" whitespace quoted-string
statement         := indentation (block | property) newline
block             := identifier
property          := identifier ":" whitespace? value
value             := quoted-string | identifier | integer | float | boolean | array
array             := "[" value ("," whitespace? value)* "]"
boolean           := "true" | "false"
```

Arrays contain scalar values only in version 1. Nested arrays are invalid.

## Values

Version 1 supports strings, integers, floating-point numbers, booleans, and arrays.

```text
"quoted string"
unquoted_identifier
42
-42
3.14
-0.25
true
false
[1, 2, 3]
["one", "two"]
```

Unquoted identifiers are stored as strings.

```text
mesh: maxwell
blend: none
```

Quoted strings support these escape sequences:

```text
\"  quote
\\  backslash
\n  newline
\t  tab
```

Paths use forward slashes.

## Comments

Comments begin with `#` outside quoted strings.

```text
# Main test model
source: "assets/models/maxwell.obj"
visible: true # Enabled by default
```

## Naming

Identifiers can contain letters, digits, underscores, and hyphens. An identifier cannot begin with a digit. Names are case-sensitive.

Conventions:

```text
Components: PascalCase
Properties: snake_case
Asset IDs: snake_case
Entity IDs: snake_case
```

## Assets

Version 1 supports these asset categories:

```text
shaders
textures
materials
meshes
```

Assets load in this dependency order:

```text
Shaders and textures
Materials
Meshes
Entities
```

Every asset has a stable ID under its category. Components and other assets refer to that ID instead of a file path or runtime handle.

```text
textures
	maxwell
		source: "assets/textures/maxwell.png"
```

Paths are relative to the project working directory.

Asset IDs are local to one scene. Mesh and material registry identities are qualified internally by the normalized scene path. Different scenes can use the same local asset IDs without sharing the wrong mesh or material.

## Materials

Version 1 material fields are:

```text
shader
texture
depth_test
depth_write
blend
culling
```

Supported blend values must match the engine's `BlendMode` values. The initial values are:

```text
none
additive
alpha
premultiplied_alpha
multiply
```

An unsupported blend value is an error.

## Mesh surface materials

A mesh can assign materials to named surfaces.

```text
meshes
	character
		source: "assets/models/character.obj"

		surface_materials
			body: character_body
			eyes: character_eyes
```

Imported surfaces must retain their source names for named assignments to work.

A renderable can replace every surface material with one material:

```text
Renderable
	mesh: character
	material_override: debug_material
```

Material selection order is:

```text
Renderable material_override
Surface default material
Scene-loading error if neither exists
```

## Entities

Every direct child of `entities` is a stable entity ID.

```text
entities
	maxwell_left
```

The stable ID must be unique within the scene. It is separate from the runtime ECS entity ID.

The optional `name` property is a display name. If it is absent, tools can use the entity ID as the display name.

## Components

Components are stored under an entity's `components` block.

```text
components
	Transform
	Renderable
```

A component can have no properties. Its loader then uses its default values.

```text
components
	PlayerControlled
```

Unknown component names are errors.

Component declaration order has no semantic meaning. A component loader must not require another component to have loaded first. Cross-component validation, when needed, must run after all components for the entity are loaded.

## Transform

Transform fields are:

```text
position
rotation_degrees
scale
```

Defaults are:

```text
position: [0, 0, 0]
rotation_degrees: [0, 0, 0]
scale: [1, 1, 1]
```

Scene rotations use XYZ Euler angles in degrees. The component loader converts them to the runtime quaternion.

## Systems

Systems are not listed in `.lscene` version 1.

Core systems are installed by the engine. Game systems are installed by the application. Components in the scene determine which entities participate in each system.

## Loading guarantees

Entity construction is transactional. If an entity or component fails to load, the destination scene does not receive partial entities.

A renderable must provide `material_override` or reference a mesh with a valid default material on every surface.

Stable scene entities should be removed through `Scene::RemoveEntity()` so the ECS world and stable text-ID index remain synchronized.

## Validation

These conditions are errors:

- Unsupported scene version.
- Missing scene declaration.
- Leading spaces.
- Invalid indentation.
- Duplicate key.
- Duplicate entity ID.
- Duplicate asset ID within one category.
- Unknown component.
- Unknown property.
- Unknown asset reference.
- Invalid number or boolean.
- Wrong vector size.
- Missing required property.
- Mesh surface without a default material or instance override.

Errors include the file, line, and column.

```text
assets/scenes/maxwell.lscene:47:11: unknown mesh 'maxwel'
```

## Reserved future features

These features are not part of version 1:

- Prefabs.
- Scene inheritance.
- Parent-child entity transforms.
- System declarations.
- Nested arrays.
- Inline objects.
- Multiline strings.
- Binary scene compilation.
- Runtime hot reload.

Adding one of these features requires a deliberate format change. A breaking change requires a new format version.
