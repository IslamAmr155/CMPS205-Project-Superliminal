#pragma once

#include "../ecs/world.hpp"
#include "../components/movement.hpp"
#include "../components/rigid-body.hpp"
#include "../components/free-camera-controller.hpp"
#include "../global/global.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <reactphysics3d/reactphysics3d.h>
namespace r3d = reactphysics3d;

namespace our
{

    // The movement system is responsible for moving every entity which contains a MovementComponent.
    // This system is added as a simple example for how use the ECS framework to implement logic.
    // For more information, see "common/components/movement.hpp"
    class MovementSystem
    {
    public:
        // This should be called every frame to update all entities containing a MovementComponent.
        void update(World *world, float deltaTime)
        {

            // First, set the new transform matrices for all the entities of the physics world.
            r3d::PhysicsWorld *physicsWorld = world->getPhysicsWorld();
            if (!physicsWorld)
                return;

            // R3D Physics World has a constant frame rate of 60Hz
            const float timeStep = 1.0f / 144.0f;

            // Add the time difference in the accumulator
            float accumulator = deltaTime;

            // While there is enough accumulated time to take a step
            while (accumulator > 0)
            {
                // If the remaining time (accumulator) is less than the time step, we take a smaller time step
                float step = accumulator < timeStep ? accumulator : timeStep;

                // Update the Dynamics world with a constant time step
                physicsWorld->update(step);

                // Decrease the accumulated time
                accumulator -= step;
            }

            // For each entity in the world
            for (auto entity : world->getEntities())
            {
                // Get the movement component if it exists
                MovementComponent *movement = entity->getComponent<MovementComponent>();
                // If the movement component exists
                if (movement)
                {
                    // Change the position and rotation based on the linear & angular velocity and delta time.
                    // entity->localTransform.position += deltaTime * movement->linearVelocity;
                    // entity->localTransform.rotation += deltaTime * movement->angularVelocity;

                    // Retrieve current position and orientation
                    const r3d::Vector3 &position = entity->localTransform.getPosition();
                    const r3d::Quaternion &orientation = entity->localTransform.getOrientation();
                    glm::vec3 position_vec = glm::vec3(position.x, position.y, position.z);
                    glm::quat orientation_quat = glm::quat(orientation.w, orientation.x, orientation.y, orientation.z);

                    // Update position
                    position_vec += deltaTime * movement->linearVelocity;

                    // Convert angular velocity to quaternion derivative
                    orientation_quat += 0.5f * deltaTime * movement->angularVelocity * orientation_quat;

                    // Normalize the quaternion
                    orientation_quat = glm::normalize(orientation_quat);

                    // Update the entity's transform
                    entity->localTransform.setPosition(position_vec);
                    entity->localTransform.setOrientation(r3d::Quaternion(orientation_quat.x, orientation_quat.y, orientation_quat.z, orientation_quat.w));
                }

                // Update the rigid body (if it exists)
                RigidBodyComponent *rigidBody = entity->getComponent<RigidBodyComponent>();
                if (rigidBody)
                {
                    r3d::Transform transform = rigidBody->getRigidBody()->getTransform();
                    transform.setPosition(transform.getPosition() + rigidBody->relativePosition);

                    FreeCameraControllerComponent *controller = entity->getComponent<FreeCameraControllerComponent>();
                    if (controller)
                    {
                        r3d::Quaternion q = entity->localTransform.getOrientation();
                        // Update the rigid body's position only, keeping the orientation constant.
                        // I'm still not entirely sure why but it was mentioned in the docs somewhere...
                        transform.setOrientation(q);

                        // Set the linear velocity of the rigid body to zeroes except the y direction to allow rotating up and down as well
                        r3d::Vector3 linearVelocity = rigidBody->getRigidBody()->getLinearVelocity();
                        linearVelocity.x = 0, linearVelocity.z = 0;
                        rigidBody->getRigidBody()->setLinearVelocity(linearVelocity);
                        rigidBody->getRigidBody()->setAngularVelocity(r3d::Vector3(0, 0, 0));
                    }
                    entity->localTransform.setTransform(transform);
                }

                // Update the pressed buttons if there are any
                if (entity->name == "Button 7ooda")
                {
                    // If the button has been pressed
                    if (getMyGameTime() - entity->lastContactTime > 0.2f && entity->delta > 0.0f)
                    {
                        // If the button has been pressed for more than 0.2 seconds
                        // Move the button back to its original position
                        r3d::Vector3 position = entity->localTransform.getPosition();
                        auto rigidBody = entity->getComponent<RigidBodyComponent>()->getRigidBody();
                        rigidBody->setTransform(r3d::Transform(r3d::Vector3(position.x, position.y + 0.01f, position.z), rigidBody->getTransform().getOrientation()));
                        entity->localTransform.setPosition(glm::vec3(position.x, position.y + 0.01f, position.z));
                        entity->delta -= 0.01f;
                    }
                }
            }
        }
    };
}
