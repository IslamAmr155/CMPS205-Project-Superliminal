#include "deserialize-utils.hpp"
#include "rigid-body.hpp"

namespace our {

    void RigidBodyComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        // Get the physics world
        r3d::PhysicsWorld* physicsWorld = this->getOwner()->getWorld()->getPhysicsWorld();

        if (!physicsWorld) return;

        // Get the relative position of the rigid body instead of its global position
        glm::vec3 relative = data.value("relativePosition", glm::vec3(0.0f, 0.0f, 0.0f));
        relativePosition = r3d::Vector3(relative.x, relative.y, relative.z);

        r3d::Transform transform = this->getOwner()->localTransform.getTransform();
        transform.setPosition(transform.getPosition() + relativePosition);

        // Create the rigid body
        this->rigidBody = physicsWorld->createRigidBody(transform);

        // First get the type of the rigid body
        const std::string type = data.value("rb_type", "Dynamic");
        if(type == "Dynamic") {
            rigidBody->setType(r3d::BodyType::DYNAMIC);
        } else if(type == "Static") {
            rigidBody->setType(r3d::BodyType::STATIC);
        } else if(type == "Kinematic") {
            rigidBody->setType(r3d::BodyType::KINEMATIC);
        }

        // Set the gravity of the object
        rigidBody->enableGravity(data.value("enableGravity", false));

        // The purpose of the sleeping technique is to deactivate resting bodies so that they are not simulated anymore. This is used to save computation time because simulating many bodies is costly. A sleeping body (or group of sleeping bodies) is awaken as soon as another body collides with it or a joint in which it is involed is enabled. The sleeping technique is enabled by default.
        rigidBody->setIsAllowedToSleep(data.value("allowedToSleep", true));

        const glm::vec3 motionAxis = data.value("motionAxis", glm::vec3(1.0f, 1.0f, 1.0f));
        rigidBody->setLinearLockAxisFactor(r3d::Vector3(motionAxis.x, motionAxis.y, motionAxis.z));

        const glm::vec3 rotationAxis = data.value("rotationAxis", glm::vec3(1.0f, 1.0f, 1.0f));
        rigidBody->setAngularLockAxisFactor(r3d::Vector3(rotationAxis.x, rotationAxis.y, rotationAxis.z));

        // Create the collider
        if (data.contains("collider")) {
            deserialize_collider(data["collider"]);
        }
    }

    void RigidBodyComponent::deserialize_collider(const nlohmann::json& data) {
        if (!data.is_object()) return;

        // Get the physics common
        r3d::PhysicsCommon& physicsCommon = this->getOwner()->getWorld()->getPhysicsCommon();

        // Get the collider type
        const std::string type = data.value("type", "Box Collider");
        this->colliderType = type;

        // Create the collision shape
        r3d::CollisionShape* collisionShape = nullptr;

        if (type == "Box Collider") {
            // Parse the half extents. The half extents represent the shape of the collider
            const glm::vec3 halfExtents = data.value("halfExtents", glm::vec3(1.0f));
            this->halfExtents = halfExtents;
            collisionShape = physicsCommon.createBoxShape(r3d::Vector3(halfExtents.x, halfExtents.y, halfExtents.z));
        } else if (type == "Sphere Collider") {
            // Parse the radius
            const r3d::decimal radius = data.value("radius", 1.0f);
            this->radius = radius;
            collisionShape = physicsCommon.createSphereShape(radius);
        } else if (type == "Capsule Collider") {
            // Parse the radius
            const r3d::decimal radius = data.value("radius", 1.0f);
            this->radius = radius;
            // Parse the height
            const r3d::decimal height = data.value("height", 1.0f);
            this->height = height;
            collisionShape = physicsCommon.createCapsuleShape(radius, height);
        }

        // Create the collider
        this->collider = this->rigidBody->addCollider(collisionShape, r3d::Transform::identity());

        // Set the material properties
        r3d::Material& material = this->collider->getMaterial();

        material.setBounciness(data.value("bounciness", material.getBounciness()));
        material.setFrictionCoefficient(data.value("friction", material.getFrictionCoefficient()));
        material.setMassDensity(data.value("massDensity", material.getMassDensity()));

        // A trigger, is a collider that cannot collide with any other colliders but can only report when it is overlapping with another collider. For instance, consider a game where a player moves around and has to avoid touching some bombs. The player has a rigid body with a capsule collider for instance and the bombs are rigid bodies where each one has a sphere collider.
        collider->setIsTrigger(data.value("isTrigger", false));
    }
}