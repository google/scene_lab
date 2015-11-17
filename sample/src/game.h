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

#ifndef GAME_H_
#define GAME_H_

#include <unordered_map>
#include "corgi_component_library/animation.h"
#include "corgi_component_library/common_services.h"
#include "corgi_component_library/entity_factory.h"
#include "corgi_component_library/meta.h"
#include "corgi_component_library/physics.h"
#include "corgi_component_library/rendermesh.h"
#include "corgi_component_library/transform.h"
#include "corgi/entity_manager.h"
#include "flatbuffers/flatbuffers.h"
#include "flatui/font_manager.h"
#include "fplbase/asset_manager.h"
#include "fplbase/input.h"
#include "fplbase/renderer.h"
#include "fplbase/utilities.h"
#include "mathfu/glsl_mappings.h"
#include "pindrop/pindrop.h"
#include "scene_lab/edit_options.h"
#include "scene_lab/scene_lab.h"
#include "scene_lab/util.h"

namespace scene_lab_sample {

/// A sample game class, which handles the main game loop and holds all of the
/// subsystems we are using, including Scene Lab, CORGI components, renderer,
/// asset manager, etc.
///
/// This game class also attempts to do some intelligent work with loading game
/// assets, allowing you to refresh and load assets that have been updated.
class Game {
 public:
  Game();
  virtual ~Game() {}

  /// Initialize the game, passing in the directory name for the game's assets.
  /// If you do override this, make sure to call the base class Initialize() as
  /// well.
  virtual bool Initialize(const char* const binary_directory);

  /// Run the game. This function returns when the user is ready to exit.
  virtual void Run();

 protected:
  /// @cond SCENELAB_INTERNAL

  /// Update the game state. Call this once per frame.
  virtual bool Update(corgi::WorldTime delta_time);

  /// Render the game at the current game state. Call this once per frame after
  /// updating.
  virtual void Render();

  /// If you are not in Scene Lab, render a GUI for your game. You can override
  /// this if you want to show a different GUI (or not render any GUI at all).
  virtual void RenderInGameGui();

  /// Load any assets that have been modified since we last loaded assets. If
  /// you have a different way of loading game assets in your game, you can
  /// override this method.
  virtual void LoadNewAssets();

  /// Set up all of the components you are using in your game. If you are using
  /// a different list of components in your game, you can override this method.
  virtual void SetupComponents();

  /// Set the component type for each component based on the enum. If you are
  /// using a different Flatbuffers enum definition, you should override this
  /// method to refer to the enum names you use.
  virtual void SetComponentType(corgi::ComponentId component_id,
                                size_t enum_id);

  /// @endcond
 private:
  // Binary configuration.
  std::string config_;

  // Hold rendering context.
  fplbase::Renderer renderer_;

  // Load and own rendering resources.
  fplbase::AssetManager asset_manager_;

  corgi::EntityManager entity_manager_;
  flatui::FontManager font_manager_;
  fplbase::InputSystem input_;

  // Manage ownership and playing of audio assets.
  pindrop::AudioEngine audio_engine_;

  // World time of previous update.
  corgi::WorldTime prev_world_time_;

  std::unique_ptr<scene_lab::SceneLab> scene_lab_;

  std::unique_ptr<corgi::component_library::EntityFactory> entity_factory_;

  std::vector<scene_lab::AssetLoader> asset_loaders_;
  time_t prev_asset_load_time_;

  // Components we are using
  corgi::component_library::AnimationComponent animation_component_;
  corgi::component_library::CommonServicesComponent common_services_component_;
  corgi::component_library::MetaComponent meta_component_;
  corgi::component_library::PhysicsComponent physics_component_;
  corgi::component_library::RenderMeshComponent render_mesh_component_;
  corgi::component_library::TransformComponent transform_component_;
  scene_lab::EditOptionsComponent edit_options_component_;

  bool in_editor_;
};

}  // namespace scene_lab_sample

#endif  // GAME_H_
