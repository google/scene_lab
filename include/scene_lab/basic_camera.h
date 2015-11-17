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

#ifndef SCENE_LAB_BASIC_CAMERA_H_
#define SCENE_LAB_BASIC_CAMERA_H_

#include "component_library/camera_interface.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace scene_lab {

static const mathfu::vec3 kCameraForward = mathfu::kAxisY3f;
static const mathfu::vec3 kCameraSide = mathfu::kAxisX3f;
static const mathfu::vec3 kCameraUp = mathfu::kAxisZ3f;

/// A simple camera class that implements CameraInterface. If you just need a
/// very basic camera to use Scene Lab, you can use this one, which just
/// implements the barebones requirements, no stereoscopy or other fancy stuff.
///
/// Scene Lab will use this camera by default unless you override by calling
/// SceneLab::SetCamera.
///
/// By default, this camera uses right-handed coordinates.
class BasicCamera : public corgi::CameraInterface {
 public:
  BasicCamera();
  virtual ~BasicCamera() {}

  /// Returns the View/Projection matrix. `index` must be 0.
  virtual mathfu::mat4 GetTransformMatrix(int32_t index) const {
    assert(index == 0);
    (void)index;
    return GetTransformMatrix();
  }

  /// Returns the View/Projection matrix.
  virtual mathfu::mat4 GetTransformMatrix() const;

  /// Returns just View matrix. `index` must be 0.
  virtual mathfu::mat4 GetViewMatrix(int32_t index) const {
    assert(index == 0);
    (void)index;
    return GetViewMatrix();
  }

  /// Returns just the View matrix.
  virtual mathfu::mat4 GetViewMatrix() const;

  /// Set the camera's world position. `index` must be 0.
  virtual void set_position(int32_t index, const mathfu::vec3& position) {
    assert(index == 0);
    (void)index;
    set_position(position);
  }

  /// Set the camera's world position.
  virtual void set_position(const mathfu::vec3& position) {
    position_ = position;
  }

  /// Get the camera's world position. `index` must be 0.
  virtual mathfu::vec3 position(int32_t index) const {
    assert(index == 0);
    (void)index;
    return position();
  }

  /// Get the camera's world position.
  virtual mathfu::vec3 position() const { return position_; }

  /// Set the camera's forward direction.
  virtual void set_facing(const mathfu::vec3& facing) {
    assert(facing.LengthSquared() != 0);
    facing_ = facing;
  }

  /// Get the camera's forward direction.
  virtual const mathfu::vec3& facing() const { return facing_; }

  /// Set the camera's up direction.
  virtual void set_up(const mathfu::vec3& up) {
    assert(up.LengthSquared() != 0);
    up_ = up;
  }
  /// Get the camera's up direction.
  virtual const mathfu::vec3& up() const { return up_; }

  /// Get the camera's right direction, calculated via cross product of its
  /// forward and up directions.
  mathfu::vec3 Right() const {
    return mathfu::vec3::CrossProduct(facing_, up_);
  }

  /// Set the camera's viewport angle, in radians.
  void set_viewport_angle(float viewport_angle) {
    viewport_angle_ = viewport_angle;
  }
  /// Get the camera's viewport angle, in radians.
  virtual float viewport_angle() const { return viewport_angle_; }

  /// Set the camera's viewport resolution.
  virtual void set_viewport_resolution(mathfu::vec2 viewport_resolution) {
    viewport_resolution_ = viewport_resolution;
  }
  /// Get the camera's viewport resolution.
  virtual mathfu::vec2 viewport_resolution() const {
    return viewport_resolution_;
  }
  /// Set the distance to the near clipping plane.
  virtual void set_viewport_near_plane(float viewport_near_plane) {
    viewport_near_plane_ = viewport_near_plane;
  }
  /// Get the distance to the near clipping plane.
  virtual float viewport_near_plane() const { return viewport_near_plane_; }

  /// Set the distance to the far clipping plane.
  virtual void set_viewport_far_plane(float viewport_far_plane) {
    viewport_far_plane_ = viewport_far_plane;
  }
  /// Get the distance to the far clipping plane.
  virtual float viewport_far_plane() const { return viewport_far_plane_; }

  /// Set the camera's viewport.
  virtual void set_viewport(const mathfu::vec4i& viewport) {
    viewport_ = viewport;
  }
  /// Set the camera's viewport. `index` must be 0.
  virtual void set_viewport(int32_t index, const mathfu::vec4i& viewport) {
    assert(index == 0);
    (void)index;
    set_viewport(viewport);
  }
  /// Get the camera's viewport. `index` must be 0.
  virtual const mathfu::vec4i& viewport(int32_t index) const {
    assert(index == 0);
    (void)index;
    return viewport();
  }

  /// Get the camera's viewport settings.
  virtual const mathfu::vec4i& viewport() const { return viewport_; }

  /// Returns false, since this camera is not stereoscopic by design.
  virtual bool IsStereo() const { return false; }
  /// Fails if you try to set stereoscopic mode to anything but false.
  virtual void set_stereo(bool is_stereo) {
    assert(!is_stereo);
    (void)is_stereo;
  }

  /// Initialize the camera's viewport settings.
  void Initialize(float viewport_angle, mathfu::vec2 viewport_resolution,
                  float viewport_near_plane, float viewport_far_plane) {
    viewport_angle_ = viewport_angle;
    viewport_resolution_ = viewport_resolution;
    viewport_near_plane_ = viewport_near_plane;
    viewport_far_plane_ = viewport_far_plane;
  }

 private:
  mathfu::vec3 position_;
  mathfu::vec3 facing_;
  mathfu::vec3 up_;
  float viewport_angle_;
  mathfu::vec2 viewport_resolution_;
  float viewport_near_plane_;
  float viewport_far_plane_;
  mathfu::vec4i viewport_;
};

}  // namespace scene_lab

#endif  // SCENE_LAB_BASIC_CAMERA_H_
