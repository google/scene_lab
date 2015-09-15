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

#include <assert.h>
#include <string.h>
#include "library_components_generated.h"
#include "component_library/physics.h"
#include "component_library/rendermesh.h"
#include "component_library/common_services.h"
#include "mathfu/utilities.h"
#include "world_editor/editor_event.h"
#include "world_editor/edit_options.h"
#include "world_editor/world_editor.h"

FPL_ENTITY_DEFINE_COMPONENT(fpl::editor::EditOptionsComponent,
                            fpl::editor::EditOptionsData)

namespace fpl {
namespace editor {

using fpl::component_library::CommonServicesComponent;
using fpl::component_library::PhysicsComponent;
using fpl::component_library::PhysicsData;
using fpl::component_library::RenderMeshComponent;
using fpl::component_library::RenderMeshData;

void EditOptionsComponent::SetWorldEditorCallbacks(WorldEditor* world_editor) {
  assert(world_editor);
  world_editor->AddOnEnterEditorCallback([this]() { EditorEnter(); });
  world_editor->AddOnExitEditorCallback([this]() { EditorExit(); });
  world_editor->AddOnCreateEntityCallback(
      [this](const entity::EntityRef& entity) { EntityCreated(entity); });
}

void EditOptionsComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  const EditOptionsDef* edit_def = static_cast<const EditOptionsDef*>(raw_data);
  EditOptionsData* edit_data = AddEntity(entity);
  if (raw_data != nullptr) {
    if (edit_def->selection_option() != SelectionOption_Unspecified) {
      edit_data->selection_option = edit_def->selection_option();
    }
    if (edit_def->render_option() != RenderOption_Unspecified) {
      edit_data->render_option = edit_def->render_option();
    }
  }
}

entity::ComponentInterface::RawDataUniquePtr
EditOptionsComponent::ExportRawData(const entity::EntityRef& entity) const {
  const EditOptionsData* data = GetComponentData(entity);
  if (data == nullptr) return nullptr;

  flatbuffers::FlatBufferBuilder fbb;
  bool defaults = entity_manager_->GetComponent<CommonServicesComponent>()
                      ->export_force_defaults();
  fbb.ForceDefaults(defaults);
  EditOptionsDefBuilder builder(fbb);
  if (defaults || data->selection_option != SelectionOption_Unspecified)
    builder.add_selection_option(data->selection_option);

  if (defaults || data->render_option != RenderOption_Unspecified)
    builder.add_render_option(data->render_option);

  fbb.Finish(builder.Finish());
  return fbb.ReleaseBufferPointer();
}

void EditOptionsComponent::EditorEnter() {
  auto render_mesh_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    entity::EntityRef entity = iter->entity;
    if (iter->data.render_option == RenderOption_OnlyInEditor ||
        iter->data.render_option == RenderOption_NotInEditor) {
      RenderMeshData* rendermesh_data =
          render_mesh_component->GetComponentData(entity);
      if (rendermesh_data != nullptr) {
        bool hide = (iter->data.render_option == RenderOption_NotInEditor);
        iter->data.backup_rendermesh_hidden = rendermesh_data->currently_hidden;
        rendermesh_data->currently_hidden = hide;
      }
    }
    if (physics_component &&
        (iter->data.selection_option == SelectionOption_PointerOnly ||
         iter->data.selection_option == SelectionOption_Any ||
         iter->data.selection_option == SelectionOption_Unspecified)) {
      entity_manager_->AddEntityToComponent<PhysicsComponent>(entity);
      // Generate shapes for raycasting, setting the results to not be
      // included on export.
      physics_component->GenerateRaycastShape(entity, false);
    }
  }
}

void EditOptionsComponent::EntityCreated(entity::EntityRef entity) {
  auto render_mesh_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  auto physics_component = entity_manager_->GetComponent<PhysicsComponent>();
  EditOptionsData* data = Data<EditOptionsData>(entity);
  if (data->render_option == RenderOption_OnlyInEditor ||
      data->render_option == RenderOption_NotInEditor) {
    RenderMeshData* rendermesh_data =
        render_mesh_component->GetComponentData(entity);
    if (rendermesh_data != nullptr) {
      bool hide = (data->render_option == RenderOption_NotInEditor);
      data->backup_rendermesh_hidden = rendermesh_data->currently_hidden;
      rendermesh_data->currently_hidden = hide;
    }
  }
  if (physics_component &&
      (data->selection_option == SelectionOption_PointerOnly ||
       data->selection_option == SelectionOption_Any ||
       data->selection_option == SelectionOption_Unspecified)) {
    entity_manager_->AddEntityToComponent<PhysicsComponent>(entity);
    // Generate shapes for raycasting, setting the results to not be
    // included on export.
    physics_component->GenerateRaycastShape(entity, false);
  }
}

void EditOptionsComponent::EditorExit() {
  auto render_mesh_component =
      entity_manager_->GetComponent<RenderMeshComponent>();
  for (auto iter = component_data_.begin(); iter != component_data_.end();
       ++iter) {
    if (iter->data.render_option == RenderOption_OnlyInEditor ||
        iter->data.render_option == RenderOption_NotInEditor) {
      RenderMeshData* rendermesh_data =
          render_mesh_component->GetComponentData(iter->entity);
      if (rendermesh_data != nullptr) {
        rendermesh_data->currently_hidden = iter->data.backup_rendermesh_hidden;
      }
    }
  }
}

}  // namespace editor
}  // namespace fpl
