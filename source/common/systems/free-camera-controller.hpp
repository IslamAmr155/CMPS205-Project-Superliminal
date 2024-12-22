#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/rigid-body.hpp"

#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <reactphysics3d/reactphysics3d.h>
namespace r3d = reactphysics3d;

class RaycastCollision : public r3d::RaycastCallback
{
    public:
    float& distance;
    bool& picked;
    our::Entity*& hitEntity, *&pickedEntity;
    r3d::Vector3 startPoint, endPoint, &hitPoint;
    our::World* world;

    bool testForDistance = true;

    RaycastCollision(const r3d::Vector3& start, const r3d::Vector3& end, bool& picked, our::World* world, 
                    our::Entity*& hitEntity, float& distance, our::Entity*& pickedEntity, 
                    r3d::Vector3& hitPoint, bool testForDistance = true) 
                : startPoint(start), endPoint(end), picked(picked), world(world), hitEntity(hitEntity), 
                distance(distance), pickedEntity(pickedEntity), hitPoint(hitPoint), testForDistance(testForDistance) {}

    virtual r3d::decimal notifyRaycastHit(const r3d::RaycastInfo& info) {
        glm::vec3 glmStartPoint(startPoint.x, startPoint.y, startPoint.z);
        // std::cout << "Start Point: " << glmStartPoint.x << " " << glmStartPoint.y << " " << glmStartPoint.z << std::endl;
        glm::vec3 glmEndPoint(endPoint.x, endPoint.y, endPoint.z);
        // std::cout << "End Point: " << glmEndPoint.x << " " << glmEndPoint.y << " " << glmEndPoint.z << std::endl;
        hitPoint = info.worldPoint;
        // std::cout << "Hit Point: " << hitPoint.x << " " << hitPoint.y << " " << hitPoint.z << std::endl;
        distance = glm::length(glmEndPoint - glmStartPoint) * info.hitFraction;
        r3d::Body* body = info.body;
        r3d::Collider* collider = info.collider;
        r3d::decimal continueRaycast = info.hitFraction;

        // for (auto entity : world->getEntities()) {
        //     auto rigidBody = entity->getComponent<our::RigidBodyComponent>(); 
        //         // std::cout << "Entity: " << entity->name << std::endl;
        //     if (rigidBody && rigidBody->getRigidBody()->getNbColliders() != 0 && rigidBody->getRigidBody()->getCollider(0) == collider) {
        //         hitEntity = entity;
        //         break;
        //     }
        // }
        hitEntity = static_cast<our::Entity*>(body->getUserData());

        if (!testForDistance) {
            if (picked && hitEntity != pickedEntity) {
                picked = false;
                continueRaycast = 0.0;
            } else if (picked && hitEntity == pickedEntity) {
                continueRaycast = -1.0;
            } else if (!picked && !hitEntity->pickable) {
                picked = false;
                continueRaycast = 0.0;
            } else if (!picked && hitEntity->pickable) {
                continueRaycast = 0.0;
                picked = true;
                pickedEntity = hitEntity;
            }
        } else {
            if (picked && hitEntity != pickedEntity) {
                continueRaycast = 0.0;
                // std::cout << "Hit entity if picked: " << hitEntity->name << std::endl;
            } else {
                continueRaycast = -1.0;
                // std::cout << "Hit entity: " << hitEntity->name << std::endl;
            }
        }
 
        // Return a fraction of 1.0 to gather all hits
        return continueRaycast;
    }
};

namespace our
{

    // The free camera controller system is responsible for moving every entity which contains a FreeCameraControllerComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic.
    // For more information, see "common/components/free-camera-controller.hpp"
    class FreeCameraControllerSystem
    {
        Application *app;          // The application in which the state runs
        bool mouse_locked = false; // Is the mouse locked


    public:
        Entity* pickedEntity = nullptr; // The entity that is currently picked
        bool picked = false; // Is the entity picked
        float originalDistance; // The distance to the picked entity
        Entity* previousParent = nullptr;
        glm::vec3 previousScale;
        float maximumScaleRatio = 5.0f, currentScaleRatio = 1.0f, minimumScaleRatio = 0.1f;
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application *app)
        {
            this->app = app;
        }

        // This should be called every frame to update all entities containing a FreeCameraControllerComponent
        void update(World *world, float deltaTime)
        {
            // First of all, we search for an entity containing both a CameraComponent and a FreeCameraControllerComponent
            // As soon as we find one, we break
            CameraComponent *camera = nullptr;
            FreeCameraControllerComponent *controller = nullptr;
            for (auto entity : world->getEntities())
            {
                camera = entity->getComponent<CameraComponent>();
                controller = entity->getComponent<FreeCameraControllerComponent>();
                if (camera && controller)
                    break;
            }
            for (auto entity : world->getEntities())
            {
                if (entity->name == "Cube") {
                    r3d::Vector3 pos = entity->localTransform.getPosition();
                    // std::cout << "Cube position: " << pos.x << " " << pos.y << " " << pos.z << std::endl;
                }
            }
            // If there is no entity with both a CameraComponent and a FreeCameraControllerComponent, we can do nothing so we return
            if (!(camera && controller))
                return;
            // Get the entity that we found via getOwner of camera (we could use controller->getOwner())
            Entity *entity = camera->getOwner();

            // If the left mouse button is pressed, we lock and hide the mouse. This common in First Person Games.
            if (app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && !mouse_locked)
            {
                app->getMouse().lockMouse(app->getWindow());
                mouse_locked = true;
                // If the left mouse button is released, we unlock and unhide the mouse.
            }
            else if (!app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && mouse_locked)
            {
                app->getMouse().unlockMouse(app->getWindow());
                mouse_locked = false;
            }

            // We get a reference to the entity's position and rotation
            // glm::vec3& position = entity->localTransform.position;
            // glm::vec3& rotation = entity->localTransform.rotation;

            // Retrieve r3d position and rotation then convert them to glm types for calculation purposes.
            const r3d::Vector3 &pos = entity->localTransform.getPosition();
            const r3d::Quaternion orien = entity->localTransform.getOrientation();
            glm::vec3 position(pos.x, pos.y, pos.z);
            glm::quat rotation(orien.w, orien.x, orien.y, orien.z);

            // If the left mouse button is pressed, we get the change in the mouse location
            // and use it to update the camera rotation
            if (app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1))
            {
                glm::vec2 delta = app->getMouse().getMouseDelta();
                // rotation.x -= delta.y * controller->rotationSensitivity; // The y-axis controls the pitch
                // rotation.y -= delta.x * controller->rotationSensitivity; // The x-axis controls the yaw

                // TODO: needs revision (mainly regarding gimble lock i think)
                float pitch = -delta.y * controller->rotationSensitivity;
                float yaw = -delta.x * controller->rotationSensitivity;

                glm::quat yawQuat = glm::angleAxis(yaw, glm::vec3(0, 1, 0));

                // To prevent 360 degrees rotation (breaking the neck xD) we limit the pitch to 90 degrees
                // We also prevent the pitch from exceeding a certain angle from the XZ plane to prevent gimbal locks
                glm::mat4 M = camera->getOwner()->getLocalToWorldMatrix();
                // Compute true view matrix
                glm::vec3 eye = glm::vec3(M * glm::vec4(0, 0, 0, 1));
                glm::vec3 center = glm::vec3(M * glm::vec4(0, 0, -1, 1));
                glm::vec3 forward = glm::normalize(center - eye);

                // Check if the forward is above or below the vertical axis, if it is, we reset the pitch to 0 degrees
                if (glm::dot(forward, glm::vec3(0, 1, 0)) > 0.75f && pitch > 0)
                    pitch = 0.0f;
                else if (glm::dot(forward, glm::vec3(0, -1, 0)) > 0.75f && pitch < 0)
                    pitch = 0.0f;

                glm::quat pitchQuat = glm::angleAxis(pitch, glm::vec3(1, 0, 0));

                rotation = yawQuat * rotation * pitchQuat; // Not sure about this order
                rotation = glm::normalize(rotation);
            }

            // We prevent the pitch from exceeding a certain angle from the XZ plane to prevent gimbal locks
            // if (rotation.x < -glm::half_pi<float>() * 0.99f)
            //     rotation.x = -glm::half_pi<float>() * 0.99f;
            // if (rotation.x > glm::half_pi<float>() * 0.99f)
            //     rotation.x = glm::half_pi<float>() * 0.99f;
            // // This is not necessary, but whenever the rotation goes outside the 0 to 2*PI range, we wrap it back inside.
            // // This could prevent floating point error if the player rotates in single direction for an extremely long time.
            // rotation.y = glm::wrapAngle(rotation.y);

            // We update the camera fov based on the mouse wheel scrolling amount
            float fov = camera->fovY + app->getMouse().getScrollOffset().y * controller->fovSensitivity;
            fov = glm::clamp(fov, glm::pi<float>() * 0.01f, glm::pi<float>() * 0.99f); // We keep the fov in the range 0.01*PI to 0.99*PI
            camera->fovY = fov;

            // We get the camera model matrix (relative to its parent) to compute the front, up and right directions
            glm::mat4 matrix = entity->localTransform.toMat4();

            // TODO: needs revision
            glm::vec3 front = glm::normalize(glm::vec3(matrix * glm::vec4(0, 0, -1, 0)));
            front.y = 0; // We don't want the camera to move up when we move forward, so we project on the xz plane to prevent any slope differences.
            front = glm::normalize(front);
            glm::vec3 right = glm::normalize(glm::vec3(matrix * glm::vec4(1, 0, 0, 0)));
            right.y = 0; // We don't want the camera to move up when we move right, so we project on the xz plane to prevent any slope differences.
            right = glm::normalize(right);
            glm::vec3 up = glm::vec3(0, 1, 0); // The up vector is always (0, 1, 0) in world space

            glm::vec3 current_sensitivity = controller->positionSensitivity;
            // If the LEFT SHIFT key is pressed, we multiply the position sensitivity by the speed up factor
            // if (app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT))
            //     current_sensitivity *= controller->speedupFactor;

            // We change the camera position based on the keys WASD/QE
            // S & W moves the player back and forth
            if (app->getKeyboard().isPressed(GLFW_KEY_W))
                position += front * (deltaTime * current_sensitivity.z);
            if (app->getKeyboard().isPressed(GLFW_KEY_S))
                position -= front * (deltaTime * current_sensitivity.z);
            // Q & E moves the player up and down
            if (app->getKeyboard().isPressed(GLFW_KEY_Q))
                position += up * (deltaTime * current_sensitivity.y);
            if (app->getKeyboard().isPressed(GLFW_KEY_E))
                position -= up * (deltaTime * current_sensitivity.y);
            // A & D moves the player left or right
            if (app->getKeyboard().isPressed(GLFW_KEY_D))
                position += right * (deltaTime * current_sensitivity.x);
            if (app->getKeyboard().isPressed(GLFW_KEY_A))
                position -= right * (deltaTime * current_sensitivity.x);

            // If hilding an object, check if the object behind it is closer to us than the object we are holding
            // If it is, take the minimum distance so that the held object is closer to us

            float distance = 0.0f;
            Entity* hitEntityForDistance = nullptr;
            // Get the position of the camera
            r3d::Vector3 cameraPosition = camera->getOwner()->localTransform.getPosition();

            r3d::Quaternion cameraOrientation = camera->getOwner()->localTransform.getOrientation();

            // Convert the quaternion to Euler angles to manipulate the y-axis rotation
            glm::vec3 eulerAngles = glm::eulerAngles(glm::quat(cameraOrientation.w, cameraOrientation.x, cameraOrientation.y, cameraOrientation.z));

            r3d::Vector3 endPosition = cameraPosition + r3d::Vector3(front.x, front.z * glm::tan(eulerAngles.x), front.z) * 100;

            r3d::Vector3 hitPoint;

            glm::mat4 dummyMatrix = entity->localTransform.toMat4();

            // TODO: needs revision
            glm::vec3 dummyFront = glm::normalize(glm::vec3(matrix * glm::vec4(0, 0, -1, 0)));
            endPosition  = cameraPosition + r3d::Vector3(dummyFront.x, dummyFront.y, dummyFront.z) * 100;

            RaycastCollision raycastCallback(cameraPosition, endPosition, picked, world, hitEntityForDistance, distance, pickedEntity, hitPoint);

            r3d::Ray ray(cameraPosition, endPosition);

            world->getPhysicsWorld()->raycast(ray, &raycastCallback);
            if (hitEntityForDistance->name != "")
                std::cout << hitEntityForDistance->name << std::endl;


            // distance > 0 added just fr testing as this should never be the case in the game when the player is surrounded by the room
            if (picked && distance > 0 && distance < originalDistance) {
                r3d::Transform transform = pickedEntity->localTransform.getTransform();
                transform.setPosition(r3d::Vector3(0,0,-distance*0.8f));
                pickedEntity->localTransform.setTransform(transform);
                pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->setTransform(transform);

                float scaleRatio = distance / originalDistance;
                glm::vec3 newScale = previousScale * scaleRatio;
                pickedEntity->localTransform.setScale(newScale);
            }

            if (app->getKeyboard().justPressed(GLFW_KEY_SPACE)) {
                float distance = 0.0f;
                Entity* hitEntity = nullptr;
                // Get the position of the camera
                r3d::Vector3 cameraPosition = camera->getOwner()->localTransform.getPosition();

                r3d::Quaternion cameraOrientation = camera->getOwner()->localTransform.getOrientation();

                // Convert the quaternion to Euler angles to manipulate the y-axis rotation
                glm::vec3 eulerAngles = glm::eulerAngles(glm::quat(cameraOrientation.w, cameraOrientation.x, cameraOrientation.y, cameraOrientation.z));

                r3d::Vector3 endPosition = cameraPosition + r3d::Vector3(front.x, front.z * glm::tan(eulerAngles.x), front.z) * 100;

                r3d::Vector3 hitPoint;

                glm::mat4 dummyMatrix = entity->localTransform.toMat4();

                // TODO: needs revision
                glm::vec3 dummyFront = glm::normalize(glm::vec3(matrix * glm::vec4(0, 0, -1, 0)));
                endPosition  = cameraPosition + r3d::Vector3(dummyFront.x, dummyFront.y, dummyFront.z) * 100;

                // Create a raycast callback object
                RaycastCollision raycastCallback(cameraPosition, endPosition, picked, world, hitEntity, distance, pickedEntity, hitPoint, false);

                // Create the ray
                r3d::Ray ray(cameraPosition, endPosition);

                // Perform the raycast
                world->getPhysicsWorld()->raycast(ray, &raycastCallback);

                if (picked && pickedEntity->pickable) {
                    // Store distance for finding scale ratio
                    originalDistance = distance;

                    // Store previous parent and scale
                    previousParent = pickedEntity->parent;
                    previousScale = pickedEntity->localTransform.getScale();

                    // Set the picked entity's transform according to the camera as a parent
                    r3d::Transform transform;

                    glm::vec3 dimensions = pickedEntity->getComponent<RigidBodyComponent>()->halfExtents;
                    float halfDimension = glm::length(dimensions) / 2.0f;
                    transform.setPosition(r3d::Vector3(0,0,-distance * 0.8f));

                    pickedEntity->localTransform.setTransform(transform);
                    pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->setTransform(transform);

                    // set the parent to the camera
                    pickedEntity->parent = camera->getOwner();

                    // Deactivate rigid body while holding the object to avoid collisions with the object
                    pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->setIsActive(false);
                } else if (!picked && pickedEntity && pickedEntity->pickable) {
                    // Activate the rigid body when the object is dropped
                    pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->setIsActive(true);

                    // Calculate the scale ratio
                    float scaleRatio = distance / originalDistance;
                    
                    r3d::Transform transform;
                    
                    // Clamp the scale ratio to a max and min value
                    glm::vec3 newScale = previousScale;
                    if (currentScaleRatio * scaleRatio > maximumScaleRatio) {
                        scaleRatio = maximumScaleRatio / currentScaleRatio;
                    } else if (currentScaleRatio * scaleRatio < minimumScaleRatio) {
                        scaleRatio = minimumScaleRatio / currentScaleRatio;
                    }   
                    currentScaleRatio *= scaleRatio;
                    newScale *= scaleRatio;

                    // Calculate the new position of the object
                    glm::vec3 newPosition = glm::vec3(hitPoint.x, hitPoint.y, hitPoint.z) - glm::vec3(front.x, front.y, front.z) * 0.5f;

                    // Update dimensions of collider based on scale ratio
                    pickedEntity->getComponent<RigidBodyComponent>()->halfExtents = pickedEntity->getComponent<RigidBodyComponent>()->halfExtents * scaleRatio;
                    pickedEntity->getComponent<RigidBodyComponent>()->radius = pickedEntity->getComponent<RigidBodyComponent>()->radius * scaleRatio;
                    pickedEntity->getComponent<RigidBodyComponent>()->height = pickedEntity->getComponent<RigidBodyComponent>()->height * scaleRatio;

                    // Create new collider based on the new dimensions
                    const std::string type = pickedEntity->getComponent<RigidBodyComponent>()->colliderType;

                    r3d::PhysicsCommon& physicsCommon = pickedEntity->getWorld()->getPhysicsCommon();

                    // Create the collision shape
                    r3d::CollisionShape* collisionShape = nullptr;

                    if (type == "Box Collider") {
                        // Parse the half extents. The half extents represent the shape of the collider
                        const glm::vec3 halfExtents = pickedEntity->getComponent<RigidBodyComponent>()->halfExtents;
                        collisionShape = physicsCommon.createBoxShape(r3d::Vector3(halfExtents.x, halfExtents.y, halfExtents.z));
                    } else if (type == "Sphere Collider") {
                        // Parse the radius
                        const r3d::decimal radius = pickedEntity->getComponent<RigidBodyComponent>()->radius;
                        collisionShape = physicsCommon.createSphereShape(radius);
                    } else if (type == "Capsule Collider") {
                        // Parse the radius
                        const r3d::decimal radius = pickedEntity->getComponent<RigidBodyComponent>()->radius;
                        // Parse the height
                        const r3d::decimal height = pickedEntity->getComponent<RigidBodyComponent>()->height;
                        collisionShape = physicsCommon.createCapsuleShape(radius, height);
                    }

                    // Set the new scale and position of the object
                    pickedEntity->localTransform.setScale(newScale);
                    pickedEntity->localTransform.setPosition(newPosition);

                    // Set the new transform of the rigid body
                    transform.setPosition(r3d::Vector3(newPosition.x, newPosition.y, newPosition.z));
                    pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->setTransform(transform);
                    
                    // Update mass density of the object
                    r3d::Material material = pickedEntity->getComponent<RigidBodyComponent>()->collider->getMaterial();
                    material.setMassDensity(material.getMassDensity() * scaleRatio);

                    // Remove the old collider and add the new one
                    pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->removeCollider(pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->getCollider(0));
                    pickedEntity->getComponent<RigidBodyComponent>()->collider = pickedEntity->getComponent<RigidBodyComponent>()->getRigidBody()->addCollider(collisionShape, r3d::Transform::identity());
                    pickedEntity->getComponent<RigidBodyComponent>()->collider->setMaterial(material);

                    // Set the parent back to the previous parent
                    pickedEntity->parent = previousParent;
                    pickedEntity = nullptr;
                }
            }

            // We set the entity's position and rotation to the new values
            entity->localTransform.setPosition(position);
            entity->localTransform.setOrientation(r3d::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w));
            RigidBodyComponent *rigidBody = entity->getComponent<RigidBodyComponent>();
            if (rigidBody && rigidBody->getRigidBody())
            {

                glm::quat orientation = rotation;

                glm::vec3 forward = glm::rotate(orientation, glm::vec3(0, 0, -1));
                forward.y = 0;
                forward = glm::normalize(forward);

                float angle = glm::acos(glm::dot(forward, glm::vec3(0, 0, -1)));
                glm::vec3 cross = glm::cross(forward, glm::vec3(0, 0, -1));

                if(cross.y < 0)
                    angle = -angle;

                glm::quat rotationY = glm::angleAxis(angle, glm::vec3(0, 1, 0));
                r3d::Quaternion orientationQuat(rotationY.x, rotationY.y, rotationY.z, rotationY.w);

                r3d::Transform transform = rigidBody->getRigidBody()->getTransform();
                transform.setPosition(entity->localTransform.getPosition() + rigidBody->relativePosition);
                transform.setOrientation(orientationQuat);
                rigidBody->getRigidBody()->setTransform(transform);
            }
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void
        exit()
        {
            if (mouse_locked)
            {
                mouse_locked = false;
                app->getMouse().unlockMouse(app->getWindow());
            }
        }
    };
}
