Building for OS X    {#scene_lab_guide_osx}
=================

# Version Requirements

Following are the minimum required versions of tools and libraries you
need to build [Scene Lab][] for [OS X]:

   * [OS X][]: Mavericks 10.9.1.
   * [Xcode][]: 5.0.1
   * [CMake][]: 2.8.12.1

# Prerequisites

   * Install [Xcode][].
   * Install [CMake][].

# Building with Xcode

Firstly, the [Xcode][] project needs to be generated using [CMake][]:

   * Open a command line window.
   * Go to the [Scene Lab][] project directory.
   * Use [CMake][] to generate the [Xcode][] project.

~~~{.sh}
    cd scene_lab
    cmake -G Xcode .
~~~

Then the project can be opened in [Xcode][] and built:

   * Double-click on `scene_lab/scene_lab.xcodeproj` to open the project in
     [Xcode][].
   * Select "Product-->Build" from the menu.

The unit tests can be run from within [Xcode][]:

   * Select an application `Scheme`, for example
     "angle_test-->My Mac 64-bit", from the combo box to the right of the
     "Run" button.
   * Click the "Run" button.


# Building from the Command Line

To build:

   * Open a command line window.
   * Go to the [SceneLab][] project directory.
   * Use [CMake][] to generate the makefiles.
   * Run make.

For example:

~~~{.sh}
    cd scene_lab
    cmake -G "Xcode" .
    xcodebuild
~~~

<br>

  [CMake]: http://www.cmake.org
  [OS X]: http://www.apple.com/osx/
  [Scene Lab]: @ref scene_lab_overview
  [Xcode]: http://developer.apple.com/xcode/
