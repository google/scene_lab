Using Scene Lab    {#scene_lab_using}
===============

# User Interface

## Activating Scene Lab

It’s up to your game to manage when to activate Scene Lab. Maybe you’ll set this
to a keypress, or an option in your pause menu. Scene Lab is designed to be
activated and deactivated repeatedly within a level to make changes, test them,
tweak them, etc.

## The Menu Bar

The top of the screen contains the menu bar, more correctly called a button
bar. This contains a context-sensitive collection of buttons, including:

Menu Button | Effect
--- | ---
Save Scene | Save all entities to disk.
Exit Scene Lab | Exit back to your game. Prompts the user to save if they haven't already.

*TODO: Add screenshots when sample project is done.*

# Moving Around

Scene Lab has two overall modes: Movement mode and Edit mode. In Movement mode,
you can move around your game's environment by using the WASD keys (and, if the
ground-parallel movement config option is enabled, with the R and F keys to move
up and down) and moving the mouse.

You can toggle to and from Edit mode by clicking the right mouse button.

Key | Movement Mode Action
--- | ---
W | Move the camera forwards.
A | Move the camera to the left.
S | Move the camera backwards.
D | Move the camera to the right.
R | If ground-parallel movement is enabled, move the camera up.
F | If ground-parallel movement is enabled, move the camera down.
Right-click mouse | Enter Edit mode.

# Selecting and Editing Entities

In Edit Mode, your mouse cursor will be visible, and you can select entities by
clicking on them. You can then interact further with the selected entity,
whether editing its properties or using the mouse to move it around the scene.

## Selecting Entities

In Edit mode, clicking on an object with the mouse will select it. In technical
terms, it will cast a ray from the screen in the direction you clicked, and
determine which entity the ray intersects based on its PhysicsComponent bounding
box. These bounding boxes are normally automatically generated for entities with
a RenderMesh, but if your object has its own PhysicsComponent data and physics
shapes, it will use those instead.

Note that because the bounding boxes are larger than the objects they contain,
it is possible to select an object even though it seems like you are clicking on
the object behind it. You can always turn on Show Physics in the Settings tab to
look at the physics shapes for the selected entity.

If you want to go back to Movement mode, click the right mouse button, which
toggles between the two modes.

## Moving Entities Around

After you have selected an entity, click and hold the mouse button down on that
entity to move it around the scene, or rotate or scale it. The action performed
depends on which Mouse Tool you have selected.

### Mouse Tools

On the lower-left of the screen is the mouse tool selector. Clicking on this
lets you toggle the mouse tool used when editing an object. The choices are:

Mouse Tool | Effect when dragging the selected entity around.
--- | ---
Move Horizontal | Move the object along the ground plane,
Move Vertical | Move the object up and down along its current X/Y position, or left/right along its plane parallel to the clipping plane.
Rotate Horizontal | Change the object’s rotation about the axis perpendicular to the ground.
Rotate Vertical | Change the object’s rotation about the axis consisting of the camera’s forward direction projected parallel to the ground plane.
Scale All | Change the object’s overall scale.
Scale X | Change the object’s X scale.
Scale Y | Change the object’s Y scale.
Scale Z | Change the object’s Z scale.

*TODO: Add screenshot when sample project is done.*

## Editing via the Keyboard

When you have an entity selected, you can use predefined keyboard keys to adjust
certain aspects of the entity's transform. These work whether you are in
Movement or Edit mode.

### Translating

Key | Action
--- | ---
I | Move entity forward (in the positive-X direction).
J | Move entity backwards (in the negative-X direction).
K | Move entity to the left (in the negative-Y direction).
L | Move entity to the right (in the positive-Y direction).
P | Move entity up (in the positive-Z direction).
; (semicolon) | Move entity down (in the negative-Z direction).
SHIFT | Hold SHIFT to move the entity more slowly.

### Rotating

Key | Action
--- | ---
U | Rotate the entity clockwise about the X axis.
O | Rotate the entity counter-clockwise about the X axis.
Y | Rotate the entity clockwise about the Y axis.
H | Rotate the entity counter-clockwise about the Y axis.
N | Rotate the entity clockwise about the Z axis.
M | Rotate the entity counter-clockwise abou the Z axis.
SHIFT | Hold SHIFT to rotate the entity more slowly.

### Scaling

Key | Action
--- | ---
+ (plus) or = (equals) | Scale the entity to be larger in all 3 axes.
- (minus) | Scale the entity to be smaller in all 3 axes.
0 (zero) | Set the entity's scale to (1, 1, 1).

### Miscellaneous Keyboard Shortcuts

Key | Action
--- | ---
INSERT or V key | Make a copy of the entity and select the new copy so you can edit it.
DELETE or X key | Delete the currently selected entity from the world.
[ (left bracket) | Select the next entity in the entity list.
] (right bracket) | Select the previous entity in the entity list.

# The Edit Window

Depending on what you are doing, there are several edit windows that look
similar but operate somewhat differently.

The edit window may contain more data than can fit on the screen. If it does,
you can scroll the window around by clicking and dragging the mouse pointer on
the window, or by using your mouse wheel.

*TODO: Add screenshot when sample project is done.*

### Edit Entity

Edit component properties for the currently-selected entity.
See {#scene_lab_flatbuffer_editor} for more information.

*TODO: Add screenshot when sample project is done.*

### List Entities

List all entities in the scene, by their entity ID and prototype.

You can do a text search for a specific entity
or prototype name. Clicking on an entity button once selects it, and clicking
again shows you the properties for that entity, allowing you to edit them.

*TODO: Add screenshot when sample project is done.*

### Settings
Change some settings in Scene Lab.

Setting | Effect
--- | ---
Data types: On / Off | Show data type names for subtables, structs, and enums. Click to toggle.
Show physics: On / Off | Show the physics shapes for the selected entity. Click to toggle.
Expand all: On / Off | Expand all subtables in the Flatbuffer view; otherwise you must click on the "..." next to a table to expand it. Click to toggle.
Ground-Parallel Camera: On / Off | If On, all W-A-S-D keyboard movement will be parallel to the ground, and you must use the R and F keys to move up and down. If Off, movement will be in the direction of the camera.
Maximize View | Click to maximize the edit window to fill the entire screen.
Hide View | Click to hide the edit window. Click any tab to show the edit window again.

*TODO: Add screenshot when sample project is done.*
