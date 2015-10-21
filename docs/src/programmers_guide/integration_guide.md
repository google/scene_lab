Integration Guide    {#scene_lab_integration_guide}
=================

# Library Integration

To use Scene Lab via [CMake][], add the library to your CMakeLists.txt.

~~~
    set(SCENE_LAB_DIR ${CMAKE_SOURCE_DIR}/[path to scene lab]/scene_lab)
    set(MY_OBJ_DIR ${CMAKE_BINARY_DIR}/obj)  # or similar
    add_subdirectory("${SCENE_LAB_DIR}" ${MY_OBJ_DIR}/scene_lab)
~~~

You will have to add scene_lab's schemas directory to your flatc command
line as an include directory.

~~~
    [flatc command line] -I ${SCENE_LAB_DIR}/schemas
~~~

And add Scene Lab's include directories to your include directories:

~~~
    include_directories(${SCENE_LAB_FLATBUFFERS_GENERATED_INCLUDES_DIR})
    include_directories(${SCENE_LAB_DIR}/include)
~~~

If you'd like to output your world's FlatBuffers files as JSON files, you will
need to include the FlatBuffers parser in your project:

~~~
    set(my_project_SRCS
        ...  # lots of other source files
        ${dependencies_flatbuffers_dir}/src/idl_parser.cpp
        ${dependencies_flatbuffers_dir}/src/idl_gen_text.cpp
    )
~~~

And finally, link scene_lab into your project.

~~~
    target_link_libraries(my_project
        ...  # lots of other libraries to link in
        scene_lab
    )
~~~

## Prerequisites

Scene Lab is designed to be integrated into a game built using [CORGI][] and
[FlatBuffers][]. In order to take full advantage of Scene Lab's in-engine world
editing capabilities, you should be using CORGI's included component library as
the basis for your entities. Specifically you will need to use the
[TransformComponent][], [RenderMeshComponent][], and [PhysicsComponent][] (to be
able to click on entities via ray-casting), as well as [MetaComponent][] for
keeping track of entity IDs and prototypes.

You must also use a version of [EntityFactory][] that supports prototypes. The
entity factory included with CORGI's component library is a good place to start.

Note: If you roll your own custom replacement for the RenderMeshComponent,
PhysicsComponent, TransformComponent, or MetaComponent, you may be able to adapt
Scene Lab to use them, but that is outside the scope of this integration guide.

## Using the SceneLab class

To integrate the editor, instantiate a [SceneLab][] object, then call
[Initialize()][]. You will also need provide a camera for the editor to use, via
some subclass of `CameraInterface` of your game’s choosing; instantiate this and
then pass it along via [SetCamera()][]. SceneLab will take ownership of it.

~~~{.cpp}
    // Set up prerequisites.
    const SceneLabConfig* scene_lab_config = ...  // Get the SceneLabConfig
                                                  // Flatbuffer contents.
    corgi::EntityManager* entity_manager = ...    // Get CORGI's EntityManager.
    gui::FontManager* font_manager = ...          // Get FlatUI's FontManager.

    // Initialize Scene Lab.
    SceneLab* scene_lab = new SceneLab();
    scene_lab->Initialize(scene_lab_config, entity_manager, font_manager);

    // Set up the camera. MyCamera is your own subclass of CameraInterface.
    std::unique_ptr<MyCamera> my_scene_lab_camera;
    my_scene_lab_camera.reset(new MyCamera());
    ... // Set up my_camera however you want.
    scene_lab->SetCamera(std::move(my_scene_lab_camera));
~~~

When you are in edit mode, most [CORGI][] components will not be updated;
however, you probably want to update the `TransformComponent` and maybe others
depending on your game. To set these up ahead of time before entering edit mode,
call [AddComponentToUpdate()][] multiple times, once for each component you want
to update (including Transform).

~~~{.cpp}
    // Continue to update Transform and RenderMesh while Scene Lab is active.
    scene_lab->AddComponentToUpdate(
        component_library::TransformComponent::GetComponentId());
    scene_lab->AddComponentToUpdate(
        component_library::RenderMeshComponent::GetComponentId());
~~~

To enter edit mode, simply call [Activate()][]. If you want to use your in-game
camera position as the initial world editor position, call `SetInitialCamera()`
first.

~~~{.cpp}
    // Did the user decide to start Scene Lab?
    if (...) {
        // Tell Scene Lab to initialize its camera location based on our own in-
        // game camera (which must also be a subclass of CameraInterface).
        scene_lab->SetInitialCamera(my_gameplay_camera);
        // Enter Scene Lab.
        scene_lab->Activate();
    }
~~~

While the editor is in use, you must call [AdvanceFrame()][] each frame (which
will take care of updating the required components). After rendering the world
each frame, call [Render()][], which will draw the editor GUI overlay.

~~~{.cpp}
    while (...) {  // Every frame.
        // Update Scene Lab's state, UI, etc.
        scene_lab->AdvanceFrame(delta_time);

        // Prepare to render. Get the camera transform.
        const CameraInterface* camera = scene_lab_->GetCamera();
        mat4 camera_transform = camera->GetTransformMatrix();

        Renderer* renderer = ...  // Get the FPLBase Renderer.
        renderer->set_color(mathfu::kOnes4f);
        renderer->DepthTest(true);
        renderer->set_model_view_projection(camera_transform);

        MyWorldRenderFunction(camera, renderer); // Render the world yourself.

        scene_lab->Render(renderer);

        // Wait until next frame...
    }
~~~

You can save the current state of the world at any time by calling
[SaveScene()][]. When you are finished using the editor, call [Deactivate()][],
and stop calling [AdvanceFrame()][] and [Render()][] for now. You can re-enter
the editor at any time by calling [Activate()][] and resuming calling
[AdvanceFrame()][] and [Render()][] each frame.

~~~{.cpp}
    // Did the user press a key indicating they want to save?
    if (...) {
        scene_lab->SaveScene();
    }

    // Did the user decide to immediately exit Scene Lab without saving?
    if (...) {
        scene_lab->Deactivate();
        // Stop calling scene_lab->AdvanceFrame() and scene_lab->Render().
    }
~~~

Note: If you want to give the user a chance to save or discard any changes
before exiting edit mode, then when the user wants to exit the editor, call
[RequestExit()][] instead of [Deactivate()][], and then once [IsReadyToExit()][]
returns true, you can call [Deactivate()][].

~~~{.cpp}
    // Did the user decide to exit Scene Lab gracefully?
    if (...) {
        scene_lab->RequestExit();
    }
    if (scene_lab->IsReadyToExit()) {
        scene_lab->Deactivate();
        // Stop calling scene_lab->AdvanceFrame() and scene_lab->Render().
    }
~~~

Even if you haven't requested to exit, you should generally check if
[IsReadyToExit()][] returns true; if it is, you should treat that as if the user
pressed whatever button you are using for exiting the editor. This allows the
user to exit by using Scene Lab's own on-screen buttons.

## Using the CORGI Component Library

As discussed above, Scene Lab will be at its most useful if you are taking
advantage of [CORGI][] (Component Oriented Reusable Game Interface) and its
included [component library][], which provides basic functionality for creating
entities ([EntityFactory][]), rendering them ([RenderMeshComponent][]), placing
them in the world ([TransformComponent][]), giving them physics
([PhysicsComponent][]), and managing their metadata ([MetaComponent][]).

### Data Format

CORGI's component library uses [FlatBuffers][] to store data about all of the
entities in the game, as well as their prototypes. For more information on how
the CORGI component library stores data, see its documentation on the [CORGI][]
site.

If you add your own custom components to your game (and you almost certainly
will), you are responsible for creating functions to export and import the
component's data for a given entity. You must implement the `AddFromRawData()`
and `ExportRawData()` functions, which are symmetrical--one reads a FlatBuffer
and fills out the component data based on what's inside, and one reads the
component data and returns a FlatBuffer containing what's inside.

See the components in the CORGI [component library][] for examples of how to
implement these functions.


  [CMake]: http://www.cmake.org/
  [CORGI]: https://google.github.io/corgi/
  [component library]: https://google.github.io/corgi/component_library.html
  [RenderMeshComponent]: https://google.github.io/corgi/render_mesh_component.html
  [TransformComponent]: https://google.github.io/corgi/transform_component.html
  [PhysicsComponent]: https://google.github.io/corgi/physics_component.html
  [MetaComponent]: https://google.github.io/corgi/meta_component.html
  [EntityFactory]: https://google.github.io/corgi/entity_factory.html
  [FlatBuffers]: https://google.github.io/flatbuffers/
  [SceneLab]: @ref fpl::scene_lab::SceneLab
  [Initialize()]: @ref fpl::scene_lab::SceneLab::Initialize
  [SetCamera()]: @ref fpl::scene_lab::SceneLab::SetCamera
  [AddComponentToUpdate()]: @ref fpl::scene_lab::SceneLab::AddComponentToUpdate
  [Activate()]: @ref fpl::scene_lab::SceneLab::Activate
  [AdvanceFrame()]: @ref fpl::scene_lab::SceneLab::AdvanceFrame
  [Render()]: @ref fpl::scene_lab::SceneLab::Render
  [Deactivate()]: @ref fpl::scene_lab::SceneLab::Deactivate
  [SaveScene()]: @ref fpl::scene_lab::SceneLab::SaveScene
  [RequestExit()]: @ref fpl::scene_lab::SceneLab::RequestExit
  [IsReadyToExit()]: @ref fpl::scene_lab::SceneLab::IsReadyToExit
