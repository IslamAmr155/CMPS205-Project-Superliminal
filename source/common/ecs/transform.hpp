#pragma once

#include <glm/glm.hpp>
#include <json/json.hpp>
#include <reactphysics3d/reactphysics3d.h>
namespace r3d = reactphysics3d;

namespace our {

    // A transform defines the translation, rotation & scale of an object relative to its parent
    struct Transform {
        r3d::Transform transform;
    public:
        // Since r3d works with translation and rotation only, we will modify our Transform class accordingly
        // glm::vec3 position = glm::vec3(0, 0, 0); // The position is defined as a vec3. (0,0,0) means no translation
        // glm::vec3 rotation = glm::vec3(0, 0, 0); // The rotation is defined using euler angles (y: yaw, x: pitch, z: roll). (0,0,0) means no rotation
        
        // We only need the scale. We will use the r3d::Transform class to store the position and orientation (aka rotation, but I will keep it as orientation for consistency with the library).
        glm::vec3 scale = glm::vec3(1, 1, 1); // The scale is defined as a vec3. (1,1,1) means no scaling.

        // This function computes and returns a matrix that represents this transform
        glm::mat4 toMat4() const;
         // Deserializes the entity data and components from a json object
        void deserialize(const nlohmann::json&);

        // Return the transform of the object
        const r3d::Transform& getTransform() const { return transform; }

        // Set the transform of the object
        void setTransform(const r3d::Transform& transform) { this->transform = transform; }

        // Set the origin of the transform.
        void setPosition(const glm::vec3 position) { transform.setPosition(r3d::Vector3(position.x, position.y, position.z)); }

        // Return the origin of the transform.
        const r3d::Vector3& getPosition() const { return transform.getPosition(); }  

        // Set the orientation of the transform.
        void setOrientation(const r3d::Quaternion& orientation) { transform.setOrientation(orientation); }

        // Return the orientation of the transform.
        const r3d::Quaternion& getOrientation() const { return transform.getOrientation(); }   

        // Set the scale of the transform.
        void setScale(const glm::vec3 scale) { this->scale = scale; }

        // Return the scale of the transform.
        const glm::vec3& getScale() const { return scale; }   
    };
}