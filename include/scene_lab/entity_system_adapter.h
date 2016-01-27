// Copyright 2016 Google Inc. All rights reserved.
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

#ifndef SCENE_LAB_ENTITY_SYSTEM_ADAPTER_H_
#define SCENE_LAB_ENTITY_SYSTEM_ADAPTER_H_

#include <string>
#include <vector>
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/utilities.h"

namespace scene_lab {

/// Generic type to represent an entity ID. Even if your entity IDs are more
/// complex than this, you should be able to encode them into a string.
///
/// You can change GenericEntityId to be any type that can have its equality
/// compared via the "==" operator and can be value-copied by the "="
/// operator. For example, scalar types like uint64_t, or std::string (the
/// default) are valid.
///
/// If you do change the type from std::string, be sure to change the value for
/// the kNoEntityId constant in entity_system_adapter.cpp.
typedef std::string GenericEntityId;

/// Generic type representing Component IDs, if you are using a component-based
/// entity system. If you aren't, you should just use an empty component ID and
/// serialize everything at once.
///
/// You can change GenericComponentId to be any type that can have its equality
/// compared via the "==" operator and can be value-copied by the "="
/// operator. For example, scalar types like uint64_t, or std::string (the
/// default) are valid.
///
/// If you do change the type from std::string, be sure to change the value for
/// the kNoComponentId constant in entity_system_adapter.cpp.
typedef std::string GenericComponentId;

/// Generic type representing prototype IDs, if you handle them differently than
/// entity IDs. In Scene Lab, the difference between a "prototype" and an
/// "entity" is that an entity is an active 'thing' in the world, whereas a
/// prototype is an entry in an offline dictionary of things you may want to
/// load.
///
/// You can change GenericPrototypeId to be any type that can have its equality
/// compared via the "==" operator and can be value-copied by the "="
/// operator. For example, scalar types like uint64_t, or std::string (the
/// default) are valid.
typedef std::string GenericPrototypeId;

/// A minimum set of what you need to specify an entity's transform.
struct GenericTransform {
  /// World position of an entity.
  mathfu::vec3 position;
  /// Scale of an entity.
  mathfu::vec3 scale;
  /// Orientation of an entity.
  mathfu::quat orientation;
  GenericTransform()
      : position(mathfu::kZeros3f),
        scale(mathfu::kOnes3f),
        orientation(mathfu::kQuatIdentityf) {}
};

/// A minimum set of what you need for a camera.
struct GenericCamera {
  /// Camera's position.
  mathfu::vec3 position;
  /// Camera's direction vector.
  mathfu::vec3 facing;
  /// Camera's up vector.
  mathfu::vec3 up;
  GenericCamera()
      : position(mathfu::kZeros3f),
        facing(mathfu::kAxisY3f),
        up(mathfu::kAxisZ3f) {}
};

/// Information about your viewport; this will be used for casting rays into
/// your scene.
struct ViewportSettings {
  /// The vertical viewport angle in radians.
  float vertical_angle;

  /// The aspect ratio (horizontal / vertical) of your screen resolution.
  float aspect_ratio;

  ViewportSettings() : vertical_angle(0), aspect_ratio(1) {}
};

/// @brief An adapter that allows you to use Scene Lab with your choice of
/// entity component system (ECS).
///
/// To extend Scene Lab to support your own ECS, simply subclass this
/// adapter and pass it to Scene Lab when initializing it.
class EntitySystemAdapter {
 public:
  /// An entity ID that refers to "no entity", initialized in
  /// entity_system_adapter.cpp (if you change the type of GenericEntityId, you
  /// should change the value of this appropriately).
  static const GenericEntityId kNoEntityId;
  /// An component ID that refers to "no component", initialized in
  /// entity_system_adapter.cpp (if you change the type of GenericComponentId,
  /// you should change the value of this appropriately).
  static const GenericComponentId kNoComponentId;

  /// Virtual destructor.
  virtual ~EntitySystemAdapter();

  /// Update your ECS one frame. You should only update the parts of it that
  /// are OK to update while in edit mode! e.g. not physics.
  virtual void AdvanceFrame(double delta_seconds) { (void)delta_seconds; }

  /// Optional: Render anything extra you'd like to render.
  virtual void Render() {}

  /// Called when Scene Lab is activated.
  virtual void OnActivate() {}

  /// Called when Scene Lab is deactivated.
  virtual void OnDeactivate() {}

  /// Called when an entity is modified.
  virtual void OnEntityUpdated(const GenericEntityId& id) { (void)id; }

  /// Called when an entity is created.
  virtual void OnEntityCreated(const GenericEntityId& id) { (void)id; }

  /// Called when an entity is deleted.
  virtual void OnEntityDeleted(const GenericEntityId& id) { (void)id; }

  /// Check whether an entity exists.
  ///
  /// @return true if the entity exists, false if it does not.
  virtual bool EntityExists(const GenericEntityId& id) = 0;

  /// Get the transform for a given entity.
  ///
  /// @return true if it successfully retrieved the transform, false otherwise
  /// (e.g. if the entity does not have a transform).
  virtual bool GetEntityTransform(const GenericEntityId& id,
                                  GenericTransform* transform_output) = 0;

  /// Set the transform for a given entity, adding a transform to it if needed.
  ///
  /// @return true if it successfully set the entity's transform, false
  /// otherwise (e.g. if the entity does not have a transform and one cannot be
  /// added). Don't forget to update your entity's physics.
  virtual bool SetEntityTransform(const GenericEntityId& id,
                                  const GenericTransform& transform) = 0;

  /// Get a list of the entity's child entities (assuming a hierarchical scene).
  ///
  /// @return true if there were any number of children (including zero), or
  /// false if the entity or entity system don't support the concept of child
  /// entities, or if the entity doesn't exist.
  virtual bool GetEntityChildren(
      const GenericEntityId& id,
      std::vector<GenericEntityId>* children_out) = 0;

  /// Get a list of the entity's parent entity (assuming a heirarchical
  /// scene).
  ///
  /// @return true if there is 0 or 1 parent, or false if the entity or entity
  /// system don't support the concept of parent entities, the entity wasn't
  /// found, etc.
  virtual bool GetEntityParent(const GenericEntityId& id,
                               GenericEntityId* parent_out) = 0;

  /// Set an entity's parent, thus making that entity a child of said parent.
  ///
  /// @return true if it was able to be set, or false if not (e.g. if the
  /// entity does not have the ability have parent/child relationships, or if
  /// your entity system does not support it).
  ///
  /// @note You can use kNoEntityId as the parent if you want the child entity
  /// to have no parent.
  virtual bool SetEntityParent(const GenericEntityId& child,
                               const GenericEntityId& parent) = 0;

  /// Get the camera's position / facing from your entity system.
  virtual bool GetCamera(GenericCamera* camera) = 0;

  /// Set your entity system's camera's position / facing.
  virtual bool SetCamera(const GenericCamera& camera) = 0;

  // Get the viewport settings from your entity system's camera.
  virtual bool GetViewportSettings(ViewportSettings* viewport) = 0;

  /// Create a duplicate of the given entity, returning the new entity's ID.
  ///
  /// @return true if successful or false if the entity could not be duplicated.
  virtual bool DuplicateEntity(const GenericEntityId& id,
                               GenericEntityId* new_id_output) = 0;

  /// Create a new default/blank/empty entity, whatever that means to your
  /// system. Outputs the new entity ID.
  ///
  /// @return true if successful or false if the entity could not be created.
  virtual bool CreateEntity(GenericEntityId* new_id_output) = 0;

  /// Create a new entity based on a given prototype, if your system supports
  /// entity prototypes. Outputs the new entity ID.
  ///
  /// @return true if successful or false if the entity could not be created
  /// or if the prototype was not found (or if your entity system doesn't
  /// support prototypes).
  ///
  /// @note For the purposes of Scene Lab, a "prototype" differs from an entity
  /// in that a prototype only specifies a set of entity component data and is
  /// not an actual loaded entity itself. If you want to create an entity from
  /// another live entity, use DuplicateEntity instead.
  virtual bool CreateEntityFromPrototype(const GenericPrototypeId& prototype,
                                         GenericEntityId* new_id_output) = 0;

  /// Delete a given entity.
  ///
  /// @return true if successful or false if it failed to delete the entity.
  virtual bool DeleteEntity(const GenericEntityId& id) = 0;

  /// Highlight or unhighlight a given entity on screen. Entities will be
  /// highlighted when they are selected for editing.
  ///
  /// @return true if successful or false if it failed (e.g. if the entity has
  /// no on-screen representation or cannot be highlighted due to its shader, or
  /// your entity system does not have the concept of highlighting). If
  /// is_highlighted is false and id is kNoEntityId, unhighlight ALL entities.
  virtual bool SetEntityHighlighted(const GenericEntityId& id,
                                    bool is_highlighted) = 0;

  /// Optional: draw the physics bounding box(es) for the given entity. This
  /// will be called during Scene Lab's Render() if the option is enabled in
  /// Scene Lab.
  virtual bool DebugDrawPhysics(const GenericEntityId& id) {
    (void)id;
    return false;
  }

  /// Cast a ray into the scene, and determine which entity that ray first
  /// intersects with.
  ///
  /// @return true if the ray intersected with at least one entity, in which
  /// case the entity will be output (as will the intersection point), or false
  /// if the ray did not intersect any entities or if there was a problem
  /// performing the ray intersection (perhaps no physics system is available).
  virtual bool GetRayIntersection(const mathfu::vec3& start_point,
                                  const mathfu::vec3& direction_normalized,
                                  GenericEntityId* entity_output,
                                  mathfu::vec3* intersection_point_output) = 0;

  /// Cycle forward or backwards through the entities. If `direction` is
  /// positive, cycle forward that number of entities. If `direction` is
  /// negative, cycle backwards that number of entities. If `direction` is 0,
  /// reset to the "first" entity.
  ///
  /// @return true if it was able to cycle, or false if not (for example if the
  /// current entity ID was bad or if the entity system does not support cycling
  /// through all entities).
  virtual bool CycleEntities(int direction, GenericEntityId* next_entity) = 0;

  /// Get a list of all entity IDs in your entity system. This will be used for
  /// serialization and editing UI.
  ///
  /// @return true if successful, or false if we were unable to get a list of
  /// all entity IDs for some reason.
  ///
  /// @note If this is an expensive operation in your entity system, you may
  /// want to implement caching in your adapter. If you do so, ensure that
  /// RefreshEntityIDs invalidates your cache (and you can call it if you
  /// externally modify the entities that are loaded).
  virtual bool GetAllEntityIDs(std::vector<GenericEntityId>* ids_out) = 0;

  /// Optional: A notice that you should refresh your cached entity ID list.
  virtual void RefreshEntityIDs() {}

  /// If your system supports using prototypes, get a list of all of the
  /// prototype IDs in your entity system.
  ///
  /// @return true if it was able to
  /// populate the list, or false if not.
  ///
  /// @note If this is an expensive operation in your adapter, you may want to
  /// cache this in your adapter. If you do so, ensure RefreshPrototypeIDs
  /// invalidates your cache (and you can call it if you externally modify the
  /// prototype list).
  virtual bool GetAllPrototypeIDs(std::vector<GenericPrototypeId>* ids_out) = 0;

  /// Optional: A notice that you should refresh your cached prototype ID list.
  virtual void RefreshPrototypeIDs() {}

  /// Get a human-readable name for the entity, which should be based on the
  /// entity's ID and be unique. The entity's identifier will generally be shown
  /// on screen as "Name (Description)"; see GetEntityDescription().
  virtual bool GetEntityName(const GenericEntityId& id,
                             std::string* name_out) = 0;

  /// Optional: Get a human-readable description of the entity. The entity's
  /// identifier will generally be shown on screen as "Name (Description)"; see
  /// GetEntityName().
  ///
  /// @return true if there was a description or false if not.
  virtual bool GetEntityDescription(const GenericEntityId& id,
                                    std::string* description_out) {
    // Default implementation: no description text.
    (void)id;
    (void)description_out;
    return false;
  }

  /// Optional: Return true if the given text (or regex) filter should show the
  /// given entity ID (or prototype ID), or false if not.
  ///
  /// @note if "filter" is blank, you should return true.
  virtual bool FilterShowEntityID(const GenericEntityId& id,
                                  const std::string& filter) {
    (void)id;
    (void)filter;
    return true;
  }

  /// Get the source file that this entity was created in, which will be passed
  /// to SaveEntitiesInFile(). The supported cases are:
  /// * Entity comes from a known file: Set source_file_out, return true. The
  ///   entity's data will be saved back into the given file.
  /// * Entity comes from an unknown file: Return true, but set source_file_out
  ///   to a blank string, indicating unknown filename. The entity data will be
  ///   saved to a default filename.
  /// * Entity is transient (a projectile, particle, etc.) and we don't need to
  ///   export it: Return false, indicating no filename.
  virtual bool GetEntitySourceFile(const GenericEntityId& id,
                                   std::string* source_file_out) = 0;

  /// Get the FlatBuffers binary schema you are using for your entity data.
  virtual bool GetSchema(const reflection::Schema** schema_out) = 0;

  /// Get the FlatBuffers schema you are using for your entity data.
  ///
  /// @return true if you have a text schema, false if not.
  ///
  /// @note Scene Lab cannot save JSON files without a text schema.
  virtual bool GetTextSchema(std::string* schema_out) = 0;

  /// Get the Flatbuffers table object used by the given component.
  virtual bool GetTableObject(const GenericComponentId& id,
                              const reflection::Object** table_out) = 0;

  /// Optional: get the name of the Flatbuffers table used by the given
  /// component.
  virtual bool GetTableName(const GenericComponentId& id,
                            std::string* name_out) {
    const reflection::Object* obj;
    if (!GetTableObject(id, &obj)) return false;
    *name_out = obj->name()->str();
    return true;
  }

  /// Serialize a given list of entities into a FlatBuffer, which we will write
  /// to disk or network. The output is a binary Flatbuffer with a list of
  /// entities that can be subsequently loaded back in by your entity system.
  virtual bool SerializeEntities(const std::vector<GenericEntityId>& id,
                                 std::vector<uint8_t>* buffer_out) = 0;

  /// Override whatever file cache you are keeping for the given filename, with
  /// a new copy of the binary (Flatbuffer) file data. Subsequent reads your
  /// entity system performs from <filename> should actually return <data>.
  virtual void OverrideFileCache(const std::string& filename,
                                 const std::vector<uint8_t>& data) {
    (void)filename;
    (void)data;
  }

  /// Get a list of all components the given entity has.
  ///
  /// @return true if it set the component list, false if the entity was not
  /// found.
  virtual bool GetEntityComponentList(
      const GenericEntityId& id,
      std::vector<GenericComponentId>* components_out) = 0;

  /// Get the full list of all component IDs in your system.
  virtual void GetFullComponentList(
      std::vector<GenericComponentId>* components_out) = 0;

  /// Check whether the entity's data for a component is from its prototype.
  ///
  /// @return true if the entity's data for the given component comes
  /// unchanged from its prototype, or false if the entity has its
  /// own component data that overrides the prototype (or if your entity
  /// system simply doesn't support prototypes).
  virtual bool IsEntityComponentFromPrototype(
      const GenericEntityId& entity, const GenericComponentId& component) = 0;

  /// For the editor GUI, we need to serialize and deserialize one
  /// entity-component at a time. This is used exclusively for editing,
  /// so you may want to export your Flatbuffers with force_defaults on.
  ///
  /// We use flatbuffers::unique_ptr_t here so that you can just naively call
  /// FlatBuffersBuilder::ReleasePointer() in your component data export
  /// function.
  virtual bool SerializeEntityComponent(
      const GenericEntityId& entity_id, const GenericComponentId& component,
      flatbuffers::unique_ptr_t* data_out) = 0;

  /// For the editor GUI, we need to serialize and deserialize one
  /// entity-component at a time, to load edited data back in.
  virtual bool DeserializeEntityComponent(const GenericEntityId& entity_id,
                                          const GenericComponentId& component,
                                          const uint8_t* data) = 0;
};

}  // namespace scene_lab

#endif  // SCENE_LAB_ENTITY_SYSTEM_ADAPTER_H_
