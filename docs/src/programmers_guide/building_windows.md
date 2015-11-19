Building for Windows    {#scene_lab_guide_windows}
====================

# Version Requirements

Following are the minimum required versions of tools and libraries you
need to build [Scene Lab][] for [Windows][]:

   * [Windows][]: 7
   * [Visual Studio][]: 2012
   * [CMake][]: 2.8.12.1

# Prerequisites

Prior to building, install the following:

   * [Visual Studio][]
   * [CMake][] from [cmake.org](http://cmake.org).

# Building with Visual Studio

Generate the [Visual Studio][] project using [CMake][]:

   * Open a command line window.
   * Go to the [Scene Lab][] project directory.
   * Use [CMake][] to generate the [Visual Studio][] project.

~~~{.sh}
    cd scene_lab
    cmake -G "Visual Studio 11 2012" .
~~~

Open the [Scene Lab][] solution in [Visual Studio][].

   * Double-click on `scene_lab/scene_lab.sln` to open the solution.
   * Select "Build-->Build Solution" from the menu.

<br>

  [CMake]: http://www.cmake.org
  [Scene Lab]: @ref scene_lab_overview
  [Visual Studio]: http://www.visualstudio.com/
  [Windows]: http://windows.microsoft.com/

