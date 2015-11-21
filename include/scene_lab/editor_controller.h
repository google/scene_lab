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

#ifndef SCENE_LAB_EDITOR_CONTROLLER_H_
#define SCENE_LAB_EDITOR_CONTROLLER_H_

#include "corgi_component_library/camera_interface.h"
#include "fplbase/input.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "scene_lab_config_generated.h"

namespace scene_lab {

/// @file
/// Pointer and keyboard controls for Scene Lab. This class provides a simple
/// interface for button and key presses, and for tracking the camera's facing
/// when the mouse moves like in a first-person shooter.
/// TODO: Add gamepad and virtual thumbstick support.
class EditorController {
 public:
  static const int kNumButtons = 10;  // max buttons from fplbase/input.h

  /// Initialize the controller.
  EditorController(const SceneLabConfig* config,
                   fplbase::InputSystem* input_system)
      : config_(config),
        input_system_(input_system),
        mouse_locked_(false),
        facing_current_(mathfu::kZeros3f),
        facing_previous_(mathfu::kZeros3f),
        pointer_current_(mathfu::kZeros2f),
        pointer_previous_(mathfu::kZeros2f) {
    for (int i = 0; i < kNumButtons; i++) {
      buttons_previous_[i] = buttons_current_[i] = false;
    }
  }

  /// Call this every frame to update the *WentDown() functions.
  void Update();

  /// ButtonWentDown() returns true only on the first frame the given button is
  /// being pressed. Buttons are numbered 0 thru kNumButtons-1. The primary
  /// button is number 0.
  bool ButtonWentDown(int button) const {
    return buttons_current_[button] && !buttons_previous_[button];
  }
  /// ButtonWentUp() returns true only on the first frame the given button has
  /// stopped being pressed.
  bool ButtonWentUp(int button) const {
    return buttons_previous_[button] && !buttons_current_[button];
  }
  /// ButtonIsDown() returns true while the given button is being held down.
  bool ButtonIsDown(int button) const { return buttons_current_[button]; }
  /// ButtonIsUp() returns true while the given button is not being held down.
  /// Equivalent to ButtonIsDown(button);
  bool ButtonIsUp(int button) const { return !buttons_current_[button]; }

  /// KeyWentDown() returns true on the first frame a given key is being
  /// pressed.
  bool KeyWentDown(fplbase::FPL_Keycode key) const {
    return input_system_->GetButton(key).went_down();
  }
  /// KeyWentUp() returns true on the first frame after a given key has stopped
  /// being pressed.
  bool KeyWentUp(fplbase::FPL_Keycode key) const {
    return input_system_->GetButton(key).went_up();
  }
  /// KeyIsDown() returns true while the given key is being held down.
  bool KeyIsDown(fplbase::FPL_Keycode key) const {
    return input_system_->GetButton(key).is_down();
  }
  /// KeyIsUp() returns true while the given key is not being held down.
  /// Equivalent to !KeyIsDown(key).
  bool KeyIsUp(fplbase::FPL_Keycode key) const {
    return !input_system_->GetButton(key).is_down();
  }

  /// Get the direction we are facing. If the mouse is locked, then moving it
  /// will cause your facing to change like in a first-person shooter.
  const mathfu::vec3& GetFacing() const { return facing_current_; }

  /// Set the current facing to a specific value.
  void SetFacing(const mathfu::vec3& facing) {
    facing_current_ = facing_previous_ = facing;
  }

  /// Get the on-screen pointer position. This is probably only useful is the
  /// mouse is unlocked.
  const mathfu::vec2& GetPointer() const { return pointer_current_; }

  /// Get the delta in on-screen pointer position from the previous update.
  mathfu::vec2 GetPointerDelta() const {
    return pointer_current_ - pointer_previous_;
  }

  /// Lock the mouse into the middle of the screen, which will start updating
  /// facing.
  void LockMouse() {
    mouse_locked_ = true;
    input_system_->SetRelativeMouseMode(true);
  }

  /// Stop locking the mouse to the middle of the screen; it will no longer
  /// update facing, but will update pointer location instead.
  void UnlockMouse() {
    mouse_locked_ = false;
    input_system_->SetRelativeMouseMode(false);
  }

  /// Get the pointer position in the world, as a ray from the near to far
  /// clipping plane. Returns true if the calculation succeeded.
  bool GetMouseWorldRay(const corgi::CameraInterface& camera,
                        const mathfu::vec2i& screen_size, mathfu::vec3* near,
                        mathfu::vec3* far) const;

  /// Is the mouse locked? If so, moving the mouse changes our facing. If not,
  /// moving the mouse moves the mouse pointer.
  bool mouse_locked() const { return mouse_locked_; }

  MATHFU_DEFINE_CLASS_SIMD_AWARE_NEW_DELETE

 private:
  const SceneLabConfig* config_;
  fplbase::InputSystem* input_system_;

  bool mouse_locked_;

  mathfu::vec3 facing_current_;
  mathfu::vec3 facing_previous_;

  mathfu::vec2 pointer_current_;
  mathfu::vec2 pointer_previous_;

  bool buttons_current_[kNumButtons];
  bool buttons_previous_[kNumButtons];
};

}  // namespace scene_lab

#endif  // SCENE_LAB_EDITOR_CONTROLLER_H_
