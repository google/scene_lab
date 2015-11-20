Scene Lab   {#scene_lab_readme}
=========

## Overview

Scene Lab is a library that allows game developers who are using Fun Propulsion
Labs technologies for their games to lay out objects in the game world and
change their properties, all within the game itself.

Go to our [landing page][] to browse our documentation and see some examples.

   * Discuss Scene Lab with other developers and users on the
     [Scene Lab Google Group][].
   * File issues on the [Scene Lab Issues Tracker][].
   * Post your questoins to [stackoverflow.com][] with a mention of
     **scene-lab**.

## Features

This initial release of Scene Lab is focused on letting you perform certain core
tasks needed for editing a game world:

  * Fly around the game world with mouse and keyboard to look at the current
    layout.
  * Right-click to enter edit mode, which allows you to:
    * Click on entities to select them and view their properties.
    * Move, rotate, and scale entities in the game world by dragging with the
      mouse.
    * Edit properties of an entity via a text-based FlatBuffer editor.
    * Duplicate or delete entities in the game world.
  * Save in-game entities to a binary FlatBuffer file that can be loaded back
    into your game, and optionally to JSON files.

## Downloading

The library is written in portable C++ and has been tested on the following
platforms:

   * [Android][]
   * [Linux][] (x86_64)
   * [OS X][]
   * [WIndows][]

**Important**: The Scene Lab world editor uses submodules, so download the
source using:

~~~{.sh}
  git clone --recursive https://github.com/google/scene_lab.git
~~~

## Dependencies

Scene Lab depends on the following libraries:

  * [CORGI][] (Component Oriented Reusable Game Interface), with the included
    component library
  * [FPLBase][]
  * [FlatBuffers][]
  * [FlatUI][]
  * [mathfu][]
  * [Breadboard][] (required by CORGI's included component library)

This initial version of Scene Lab only supports desktop (Linux, Mac, Windows)
builds.

In order to use Scene Lab to lay out your scene, you must use CORGI and
its included [component library][] for your in-game objects. Your objects should
use the following components:

  * `MetaComponent` (all objects must have this)
  * `TransformComponent` (for moving / rotating / scaling objects)
  * `PhysicsComponent` (optional, for selecting objects with the mouse)
  * `RenderMeshComponent` (optional, for highlighting the selected object)

You must also use an entity factory based on the component library's
`EntityFactory`, which uses a prototype-based system for instantiating entities,
and ensure that you have included the `EditOptionsComponent` and associated
data in your code.

Additionally, if you have any custom components, you must implement their
`ExportRawData` functions if you want the user to be able to edit an object's
properties from those components.

## Notes

For application on Google Play that integrate this tool, usage is tracked.  This
tracking is done automatically using the embedded version string
(`kSceneLabVersionString`), and helps us continue to optimize it.  Aside from
consuming a few extra bytes in your application binary, it shouldn't affect your
application at all.  We use this information to let us know if Scene Lab is
useful and if we should continue to invest in it. Since this is open source, you
are free to remove the version string but we would appreciate if you would leave
it in.

## Contributing

To contribute to this project see [CONTRIBUTING][].

<br>

  [Android]: http://www.android.com
  [Breadboard]: http://google.github.io/breadboard/
  [component library]: http://google.github.io/corgi/component_library.html
  [CONTRIBUTING]: http://github.com/google/scene_lab/blob/master/CONTRIBUTING
  [CORGI]: http://google.github.io/corgi/
  [FlatBuffers]: http://google.github.io/flatbuffers/
  [FlatUI]: http://google.github.io/flatui/
  [FPLBase]: http://google.github.io/fplbase/
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [landing page]: http://google.github.io/scene_lab
  [mathfu]: http://google.github.io/mathfu/
  [OS X]: http://www.apple.com/osx/
  [Scene Lab Google Group]: http://group.google.com/group/scene_lab
  [Scene Lab Issues Tracker]: http://github.com/google/scene_lab/issues
  [stackoverflow.com]: http://www.stackoverflow.com
  [Windows]: http://windows.microsoft.com/
