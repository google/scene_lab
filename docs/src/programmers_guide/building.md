Build Targets    {#scene_lab_guide_build_targets}
=============

[Scene Lab][] compiles to a C++ library. It includes [CMake][] build scripts.

The CMake build script can be used to generate standard
makefiles on [Linux][], [Xcode][] projects on [OS X][],
and [Visual Studio][] solutions on [Windows][]. See,

   * [Building for Linux][]
   * [Building for OS X][]
   * [Building for Windows][]

You can also build and link Scene Lab for Android, though the Scene Lab UI does
not yet function or let you edit entities on the device itself. Scene Lab
includes a sample project to show how to build for Android.

   * [Building for Android][]


<br>

  [Building for Android]: @ref scene_lab_guide_android
  [Building for Linux]: @ref scene_lab_guide_linux
  [Building for OS X]: @ref scene_lab_guide_osx
  [Building for Windows]: @ref scene_lab_guide_windows
  [CMake]: http://www.cmake.org/
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [OS X]: http://www.apple.com/osx/
  [Scene Lab]: @ref scene_lab_overview
  [Visual Studio]: http://www.visualstudio.com/
  [Windows]: http://windows.microsoft.com/
  [Xcode]: http://developer.apple.com/xcode/
