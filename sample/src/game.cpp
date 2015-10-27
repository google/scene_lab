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

#include "game.h"

#include "SDL.h"
#include "component_library/camera_interface.h"
#include "component_library/default_entity_factory.h"
#include "components_generated.h"
#include "fplbase/utilities.h"
#include "scene_lab/util.h"

namespace scene_lab_sample {

static const char kAssetsDir[] = "sample/assets";
static const char kEntityLibraryFile[] = "entity_prototypes.bin";
static const char kEntityListFile[] = "entity_list.bin";
static const char kComponentDefBinarySchema[] =
    "flatbufferschemas/components.bfbs";

static const int kMinUpdateTime = 1000 / 60;
static const int kMaxUpdateTime = 1000 / 30;
static const int kWindowWidth = 1200;
static const int kWindowHeight = 800;

static const float kStartingHeight = 4.0f;

static const float kBackgroundColor[] = {0.5f, 0.5f, 0.5f, 1.0f};

Game::Game() : asset_manager_(renderer_) {}

bool Game::Initialize(const char* const binary_directory) {
  if (!fpl::ChangeToUpstreamDir(binary_directory, kAssetsDir)) {
    fpl::LogError("Couldn't find assets directory: %s", kAssetsDir);
    return false;
  }

  srand(time(nullptr));

  if (!fpl::LoadFile("scene_lab_config.bin", &config_)) {
    fpl::LogError("Couldn't load scene_lab_config.bin from %s", kAssetsDir);
    return false;
  }

  const mathfu::vec2i window_size(kWindowWidth, kWindowHeight);
  if (!renderer_.Initialize(window_size, "Scene Lab Sample")) {
    return false;
  }

  input_.Initialize();

  entity_factory_.reset(new fpl::component_library::DefaultEntityFactory());
  entity_manager_.set_entity_factory(entity_factory_.get());
  font_manager_.SetRenderer(renderer_);

  scene_lab_.reset(new scene_lab::SceneLab());

  SetupComponents();

  scene_lab_->Initialize(scene_lab::GetSceneLabConfig(config_.c_str()),
                         &entity_manager_, &font_manager_);
  scene_lab_->GetCamera()->set_position(
      mathfu::vec3(0.0f, 0.0f, kStartingHeight));

  in_editor_ = false;

  // Set up asset loaders
  asset_loaders_.push_back(
      scene_lab::AssetLoader("materials", ".fplmat", [&](const char* filename) {
        if (asset_manager_.FindMaterial(filename) != nullptr)
          asset_manager_.UnloadMaterial(filename);
        asset_manager_.LoadMaterial(filename);
      }));
  asset_loaders_.push_back(
      scene_lab::AssetLoader("meshes", ".fplmesh", [&](const char* filename) {
        if (asset_manager_.FindMesh(filename) != nullptr)
          asset_manager_.UnloadMesh(filename);
        asset_manager_.LoadMesh(filename);
      }));
  prev_asset_load_time_ = 0;

  LoadNewAssets();

  entity_factory_->SetFlatbufferSchema(kComponentDefBinarySchema);
  entity_factory_->AddEntityLibrary(kEntityLibraryFile);
  entity_factory_->LoadEntitiesFromFile(kEntityListFile, &entity_manager_);

  input_.SetRelativeMouseMode(true);
  input_.AdvanceFrame(&renderer_.window_size());

#if !defined(__ANDROID__)
  // Start with Scene Lab active unless we are on Android.
  in_editor_ = true;
  scene_lab_->Activate();
#endif  // !defined(__ANDROID__)

  return true;
}

bool Game::Update(fpl::entity::WorldTime delta_time) {
  if (input_.GetButton(fpl::FPLK_F5).went_down()) {
    // Check assets path to see if any new files were added, and load them if
    // so.
    LoadNewAssets();
  }

  renderer_.AdvanceFrame(input_.minimized(), input_.Time());

  if (in_editor_) {
    scene_lab_->AdvanceFrame(delta_time);

    if (input_.GetButton(fpl::FPLK_F10).went_down() ||
        input_.GetButton(fpl::FPLK_ESCAPE).went_down()) {
      scene_lab_->RequestExit();
    }
    if (scene_lab_->IsReadyToExit()) {
      scene_lab_->Deactivate();
      in_editor_ = false;
    }
  } else {
    entity_manager_.UpdateComponents(delta_time);

    if (input_.GetButton(fpl::FPLK_F10).went_down()) {
      in_editor_ = true;
      scene_lab_->Activate();
    }
    if (input_.GetButton(fpl::FPLK_ESCAPE).went_down()) {
      return false;  // exit
    }
  }
  return true;
}

void Game::Render() {
  fpl::CameraInterface* camera = scene_lab_->GetCamera();

  camera->set_viewport_resolution(mathfu::vec2(renderer_.window_size()));

  fpl::mat4 camera_transform = camera->GetTransformMatrix();
  renderer_.set_color(mathfu::kOnes4f);
  renderer_.ClearFrameBuffer(mathfu::vec4(kBackgroundColor));  // 50% gray
  renderer_.DepthTest(true);
  renderer_.set_model_view_projection(camera_transform);

  render_mesh_component_.RenderPrep(*camera);
  render_mesh_component_.RenderAllEntities(renderer_, *camera);

  input_.AdvanceFrame(&renderer_.window_size());
  if (in_editor_) {
    scene_lab_->Render(&renderer_);
  } else {
    RenderInGameGui();
  }
}

void Game::RenderInGameGui() {
  // Tell the user how to activate edit mode.
  fpl::gui::Run(asset_manager_, font_manager_, input_, [&]() {
    fpl::gui::StartGroup(fpl::gui::kLayoutOverlay, 10, "help");
    fpl::gui::ColorBackground(mathfu::vec4(0, 0, 0, 1));
    fpl::gui::Label(
        "Game is active. Press F10 to activate Scene Lab or ESC to exit.", 20);
    fpl::gui::EndGroup();
  });
}

void Game::Run() {
  // Initialize so that we don't sleep the first time through the loop.
  prev_world_time_ = (int)(input_.Time() * 1000) - kMinUpdateTime;

  while (!input_.exit_requested()) {
    const fpl::entity::WorldTime world_time = (int)(input_.Time() * 1000);
    const fpl::entity::WorldTime delta_time =
        std::min(world_time - prev_world_time_, kMaxUpdateTime);
    prev_world_time_ = world_time;

    if (!Update(delta_time)) return;
    Render();
  }
}

void Game::LoadNewAssets() {
  time_t new_time =
      scene_lab::LoadAssetsIfNewer(prev_asset_load_time_, asset_loaders_);
  if (new_time > 0) {
    prev_asset_load_time_ = new_time;
    // Some assets were loaded. Let's wait for them to load.
    asset_manager_.StartLoadingTextures();
    while (!asset_manager_.TryFinalize()) {
    }
  }
}

void Game::SetComponentType(fpl::entity::ComponentId component_id,
                            size_t enum_id) {
  entity_factory_->SetComponentType(component_id, enum_id,
                                    EnumNamesComponentDataUnion()[enum_id]);
}

void Game::SetupComponents() {
  common_services_component_.Initialize(&asset_manager_, entity_factory_.get(),
                                        nullptr, &input_, &renderer_);

  physics_component_.set_gravity(-30.0);
  physics_component_.set_max_steps(5);

  SetComponentType(
      entity_manager_.RegisterComponent(&common_services_component_),
      ComponentDataUnion_CommonServicesDef);

  SetComponentType(entity_manager_.RegisterComponent(&render_mesh_component_),
                   ComponentDataUnion_RenderMeshDef);
  SetComponentType(entity_manager_.RegisterComponent(&physics_component_),
                   ComponentDataUnion_PhysicsDef);
  SetComponentType(entity_manager_.RegisterComponent(&meta_component_),
                   ComponentDataUnion_MetaDef);
  SetComponentType(entity_manager_.RegisterComponent(&edit_options_component_),
                   ComponentDataUnion_EditOptionsDef);
  SetComponentType(entity_manager_.RegisterComponent(&animation_component_),
                   ComponentDataUnion_AnimationDef);
  // Make sure you register TransformComponent after any components that use it.
  SetComponentType(entity_manager_.RegisterComponent(&transform_component_),
                   ComponentDataUnion_TransformDef);

  scene_lab_->AddComponentToUpdate(
      fpl::component_library::TransformComponent::GetComponentId());
  scene_lab_->AddComponentToUpdate(
      fpl::component_library::RenderMeshComponent::GetComponentId());
}

}  // namespace scene_lab_sample
