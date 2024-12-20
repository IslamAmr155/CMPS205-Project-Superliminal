#pragma once

#include "../ecs/component.hpp"
#include "../deserialize-utils.hpp"
#include "../ecs/entity.hpp"
#include "../ecs/world.hpp"

#include <reactphysics3d/reactphysics3d.h>
namespace r3d = reactphysics3d;

namespace our
{

    // This component denotes that any renderer should draw the given mesh using the given material at the transformation of the owning entity.
    class RigidBodyComponent : public Component
    {
        r3d::RigidBody *rigidBody = nullptr;    // The actual body

        void deserialize_collider(const nlohmann::json &data);

    public:
        // The relative position of the rigid body
        r3d::Vector3 relativePosition;

        r3d::Collider *collider = nullptr;      // The collider object that represents the collision physics of the rigid body.
        std::string colliderType;
        glm::vec3 halfExtents;
        r3d::decimal radius;
        r3d::decimal height;

        // The ID of this component type is "Rigid Body"
        static std::string getID() { return "Rigid Body"; }

        // This will deserialize the rigid body component from a json object
        void deserialize(const nlohmann::json &data) override;

        // This will return the rigid body
        r3d::RigidBody *getRigidBody()
        {
            return rigidBody;
        }

        ~RigidBodyComponent()
        {
            if (rigidBody)
            {
                getOwner()->getWorld()->getPhysicsWorld()->destroyRigidBody(rigidBody); // Only the physics world can destroy the rigid body to prevent ambiguous behavior.
            }
        }
    };
}