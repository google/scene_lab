// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// Utility functions that might be useful if you are using Scene Lab.
#ifndef SCENE_LAB_UTIL_H
#define SCENE_LAB_UTIL_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace scene_lab {

/// @file
/// Scan a directory on the file system for all files matching a given file
/// extension. Return all files found, along with the "last modified" time for
/// each file.
///
/// This allows you to check a directory to see if any files have been modified
/// since you last used them, which could be useful if you are hypothetically
/// updating assets used within an editor tool whenever they change on disk, for
/// example.
std::unordered_map<std::string, time_t> ScanDirectory(
    const std::string& directory, const std::string& file_ext);

/// AssetLoader struct, basically a tuple of directory, file extension, and
/// loader function.
///
/// The purpose of this is so you can scan for all the files in a given folder
/// matching the file extension, and load them via whatever method you wish.
struct AssetLoader {
  typedef std::function<void(const char* filename)> load_function_t;
  std::string directory;
  std::string file_extension;
  load_function_t load_function;
  AssetLoader(const std::string& dir, const std::string& file_ext,
              const load_function_t& load_func)
      : directory(dir), file_extension(file_ext), load_function(load_func) {}
};

/// Load assets via the designated asset loaders. Scans through the directory
/// specified by each AssetLoader for the given file pattern, and calls the load
/// function on each file that's strictly newer than the specified timestamp.
///
/// Returns the timestamp of the latest file loaded (so you can pass that into
/// the next run of this function), or 0 if no files were loaded.
time_t LoadAssetsIfNewer(time_t threshold,
                         const std::vector<AssetLoader>& asset_loaders);

/// Scan a directory for assets matching a given file extension. Any files in
/// the directory that are strictly newer than the time threshold will be loaded
/// via calling the load function passed in.
///
/// Returns the timestamp of the latest file loaded (so you can pass that into
/// the next run of this function), or 0 if no files were loaded.
time_t LoadAssetsIfNewer(time_t threshold, const std::string& directory,
                         const std::string& file_extension,
                         const AssetLoader::load_function_t& load_function);

}  // namespace scene_lab

#endif  // SCENE_LAB_UTIL_H
