Scene Lab    {#scene_lab_index}
=========

# Overview    {#scene_lab_overview}

Scene Lab is a library that allows application developers who use
[FlatBuffers][] and [CORGI][] to edit properties of objects in real-time within
the target application. For example, a game developer can use [Scene Lab][] to
lay out objects in a game world inside their game.

<img src="editor_screenshot_select.png" style="height: 30em"/>

# Features

This initial release of Scene Lab was developed for [Zooshi][] and is focused on
letting you perform certain core tasks needed for editing a game world:

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

For more information on what you can do with the Scene Lab interface, please
read [Using Scene Lab][].

# Platforms and Dependencies.

[Scene Lab][] has been tested on the following platforms:

   * [Linux][] (x86_64)
   * [OS X][]
   * [Windows][]

This library is entirely written in portable C++11 and depends on the
following libraries (included in the download / submodules):

   * [CORGI][] (Component Oriented Reusable Game Interface), with the included
     component library.
   * [FlatBuffers][] for serialization.
   * [FlatUI][] for the user interface, which in turn uses:
     * [FreeType][] for font rendering.
     * [HarfBuzz][] and [Libunibreak][] for handling unicode text correctly.
   * [mathfu][] for vector math.
   * [FPLBase][] for rendering and input, which in turn uses:
     * [SDL][] as a cross-platform layer.
     * [OpenGL][] (desktop/ES).
     * [WebP][] for image loading.
   * [FPLUtil][] (only for build).
   * [Breadboard][] (required by CORGI's included component library).

Please see the [Integration Guide][] for information on how to take advantage of
Scene Lab to edit the game world in your own games.

# Download

Download using git or from the
[releases page](http://github.com/google/scene_lab/releases).

**Important**: Scene Lab uses submodules to reference other components it depends
upon so download the source using:

~~~{.sh}
    git clone --recursive https://github.com/google/scene_lab.git
~~~

# Feedback and Reporting Bugs

   * Discuss Scene Lab with other developers and users on the
     [Scene Lab Google Group][].
   * File issues on the [Scene Lab Issues Tracker][].
   * Post your questions to [stackoverflow.com][] with a mention of **Scene Lab**.

  [Breadboard]: https://google.github.io/breadboard/
  [Build Configurations]: @ref scene_lab_build_config
  [CORGI]: https://google.github.io/corgi/
  [FPLBase]: https://google.github.io/fplbase/
  [FPLUtil]: https://google.github.io/fplutil/
  [FlatBuffers]: https://google.github.io/flatbuffers/
  [FlatUI]: https://google.github.io/flatui/
  [FreeType]: http://www.freetype.org/
  [HarfBuzz]: http://www.freedesktop.org/wiki/Software/HarfBuzz/
  [Integration Guide]: @ref scene_lab_integration_guide
  [Libunibreak]: http://vimgadgets.sourceforge.net/libunibreak/
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [mathfu]: https://google.github.io/mathfu/
  [OS X]: http://www.apple.com/osx/
  [OpenGL]: https://www.opengl.org/
  [SDL]: https://www.libsdl.org/
  [Scene Lab Google Group]: http://group.google.com/group/scene_lab
  [Scene Lab Issues Tracker]: http://github.com/google/scene_lab/issues
  [Scene Lab]: @ref scene_lab_overview
  [stackoverflow.com]: http://www.stackoverflow.com
  [Using Scene Lab]: @ref scene_lab_using
  [WebP]: https://developers.google.com/speed/webp/?hl=en
  [Windows]: http://windows.microsoft.com/



