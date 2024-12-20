#include "world.hpp"

namespace our
{

    // This will deserialize a json array of entities and add the new entities to the current world
    // If parent pointer is not null, the new entities will be have their parent set to that given pointer
    // If any of the entities has children, this function will be called recursively for these children
    void World::deserialize(const nlohmann::json &data, Entity *parent)
    {
        if (!data.is_array())
            return;
        for (const auto &entityData : data)
        {
            // TODO: (Req 8) Create an entity, make its parent "parent" and call its deserialize with "entityData".
            Entity *entity = add();
            entity->parent = parent;
            entity->deserialize(entityData);

            if (entityData.contains("children"))
            {
                // TODO: (Req 8) Recursively call this world's "deserialize" using the children data
                //  and the current entity as the parent
                deserialize(entityData["children"], entity);
            }
        }
    }

    // This will deserialize the physics world from a json object
    void World::deserializePhysicsWorld(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;
        // Create the world settings
        r3d::PhysicsWorld::WorldSettings settings;

        // TODO: Decide what settings to use for the physics world
        glm::vec3 gravity = glm::vec3(
            data["gravity"][0],
            data["gravity"][1],
            data["gravity"][2]
        );
        settings.gravity = r3d::Vector3(gravity.x, gravity.y, gravity.z);

        // Create the physics world with your settings
        physicsWorld = physicsCommon.createPhysicsWorld(settings);

        // Add the button contact listener
        ButtonContactListener *listener = new ButtonContactListener();
        physicsWorld->setEventListener(listener);
    }
}