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

#include "world_editor/editor_controller.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::quat;

namespace fpl {
namespace editor {

void EditorController::Update() {
  facing_previous_ = facing_current_;
  pointer_previous_ = pointer_current_;
  if (mouse_locked_) {
    // Mouse locked to middle of screen, track movement to change facing.
    vec2 delta = editor_config_->mouse_sensitivity() *
                 vec2(input_system_->get_pointers()[0].mousedelta);

    vec3 side_axis =
        quat::FromAngleAxis(-M_PI / 2, mathfu::kAxisZ3f) * facing_current_;

    quat pitch_adjustment = quat::FromAngleAxis(-delta.y(), side_axis);
    quat yaw_adjustment = quat::FromAngleAxis(-delta.x(), mathfu::kAxisZ3f);

    facing_current_ = pitch_adjustment * yaw_adjustment * facing_previous_;
  } else {
    // Mouse not locked, track pointer location for clicking on things.
    pointer_current_ = vec2(input_system_->get_pointers()[0].mousepos);
  }

  for (int i = 0; i < kNumButtons; i++) {
    buttons_previous_[i] = buttons_current_[i];
    buttons_current_[i] = input_system_->GetPointerButton(i).is_down();
  }
}

}  // fpl_base
}  // fpl
