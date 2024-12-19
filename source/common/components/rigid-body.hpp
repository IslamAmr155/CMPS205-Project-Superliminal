#pragma once

#include "../ecs/world.hpp"
#include "../ecs/component.hpp"
#include "../deserialize-utils.hpp"
#include "../ecs/entity.hpp"

#include <reactphysics3d/reactphysics3d.h>
namespace r3d = reactphysics3d;

namespace our
{
    class World;
    // This component denotes that any renderer should draw the given mesh using the given material at the transformation of the owning entity.
    class RigidBodyComponent : public Component
    {
        r3d::RigidBody *rigidBody = nullptr;    // The actual body
        r3d::Collider *collider = nullptr;      // The collider object that represents the collision physics of the rigid body.

        void deserialize_collider(const nlohmann::json &data);

    public:
        // The relative position of the rigid body
        r3d::Vector3 relativePosition;

        // The ID of this component type is "Rigid Body"
        static std::string getID() { return "Rigid Body"; }

        std::string getName() override
        {
            return "Rigid Body";
        }

        // This will deserialize the rigid body component from a json object
        void deserialize(const nlohmann::json &data) override;

        void raiseDoor(float offset) override
        {
            // If the rigid body exists
            if (rigidBody)
            {
                auto entity = getOwner();
                r3d::Vector3 position = entity->localTransform.getPosition();
                printf("raising by %f\n", offset);
                entity->localTransform.setPosition(glm::vec3(position.x, position.y + offset, position.z));
                auto rigidBody = this->getRigidBody();
                r3d::Transform transform = rigidBody->getTransform();
                r3d::Vector3 pos = transform.getPosition();
                transform.setPosition(r3d::Vector3(pos.x, pos.y + offset, pos.z));
                rigidBody->setTransform(transform);
            }
        }

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