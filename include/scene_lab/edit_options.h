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

#ifndef SCENE_LAB_EDIT_OPTIONS_H_
#define SCENE_LAB_EDIT_OPTIONS_H_

#include <set>
#include <string>
#include <unordered_map>
#include "corgi/component.h"
#include "editor_components_generated.h"

namespace scene_lab {

// SceneLab refers to EditOptionsComponent so this needs to be forward
// declared.
class SceneLab;

struct EditOptionsData {
  EditOptionsData()
      : selection_option(SelectionOption_Unspecified),
        render_option(RenderOption_Unspecified) {}
  SelectionOption selection_option;
  RenderOption render_option;

  // Back up some other components' data that may be changed when we go
  // in and out of edit mode
  bool backup_rendermesh_hidden;
};

class EditOptionsComponent : public corgi::Component<EditOptionsData> {
 public:
  virtual ~EditOptionsComponent() {}

  void EditorEnter();
  void EditorExit();
  void EntityCreated(corgi::EntityRef entity);

  virtual void AddFromRawData(corgi::EntityRef& entity, const void* raw_data);
  virtual RawDataUniquePtr ExportRawData(const corgi::EntityRef& entity) const;

  // This must be called once and only once when initializing Scene Lab.
  void SetSceneLabCallbacks(SceneLab* scene_lab);
};

}  // namespace scene_lab

CORGI_REGISTER_COMPONENT(scene_lab::EditOptionsComponent,
                         scene_lab::EditOptionsData)

#endif  // SCENE_LAB_EDIT_OPTIONS_H_
