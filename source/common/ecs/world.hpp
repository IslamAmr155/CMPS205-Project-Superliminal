#pragma once

#include <unordered_set>
#include "entity.hpp"
#include "../global/global.hpp"

#include <reactphysics3d/reactphysics3d.h>
namespace r3d = reactphysics3d;

namespace our
{
    // This class holds a set of entities
    class World
    {
        std::unordered_set<Entity *> entities;         // These are the entities held by this world
        std::unordered_set<Entity *> markedForRemoval; // These are the entities that are awaiting to be deleted
                                                       // when deleteMarkedEntities is called

        r3d::PhysicsCommon physicsCommon;          // The physics common object that will be used to create the physics world
        r3d::PhysicsWorld *physicsWorld = nullptr; // The physics world that will be used to simulate physics

    public:
        World() = default;

        // This will deserialize a json array of entities and add the new entities to the current world
        // If parent pointer is not null, the new entities will be have their parent set to that given pointer
        // If any of the entities has children, this function will be called recursively for these children
        void deserialize(const nlohmann::json &data, Entity *parent = nullptr);

        // This will deserialize the physics world from a json object
        void deserializePhysicsWorld(const nlohmann::json &data);

        // This will return the physics world
        r3d::PhysicsWorld *getPhysicsWorld()
        {
            return physicsWorld;
        }

        // This will return the physics common
        r3d::PhysicsCommon &getPhysicsCommon()
        {
            return physicsCommon;
        }

        // This adds an entity to the entities set and returns a pointer to that entity
        // WARNING The entity is owned by this world so don't use "delete" to delete it, instead, call "markForRemoval"
        // to put it in the "markedForRemoval" set. The elements in the "markedForRemoval" set will be removed and
        // deleted when "deleteMarkedEntities" is called.
        Entity *add()
        {
            // TODO: (Req 8) Create a new entity, set its world member variable to this,
            //  and don't forget to insert it in the suitable container.
            Entity *entity = new Entity();
            entity->world = this;
            entities.insert(entity);
            return entity;
        }

        // This returns an immutable reference to the set of all entites in the world.
        const std::unordered_set<Entity *> &getEntities()
        {
            return entities;
        }

        // This marks an entity for removal by adding it to the "markedForRemoval" set.
        // The elements in the "markedForRemoval" set will be removed and deleted when "deleteMarkedEntities" is called.
        void markForRemoval(Entity *entity)
        {
            // TODO: (Req 8) If the entity is in this world, add it to the "markedForRemoval" set.
            if (entities.find(entity) != entities.end())
            {
                markedForRemoval.insert(entity);
            }
        }

        // This removes the elements in "markedForRemoval" from the "entities" set.
        // Then each of these elements are deleted.
        void deleteMarkedEntities()
        {
            // TODO: (Req 8) Remove and delete all the entities that have been marked for removal
            for (auto entity : markedForRemoval)
            {
                entities.erase(entity);
                delete entity;
            }
            markedForRemoval.clear();
        }

        // This deletes all entities in the world
        void clear()
        {
            // TODO: (Req 8) Delete all the entites and make sure that the containers are empty
            for (auto entity : entities)
            {
                delete entity;
            }
            entities.clear();
            markedForRemoval.clear();
        }

        // Since the world owns all of its entities, they should be deleted alongside it.
        ~World()
        {
            clear();
            // Delete the physics world
            if (physicsWorld != nullptr)
            {
                physicsCommon.destroyPhysicsWorld(physicsWorld);
            }
        }

        // The world should not be copyable
        World(const World &) = delete;
        World &operator=(World const &) = delete;
    };

    class ButtonContactListener : public r3d::EventListener
    {
    public:

        bool isButton(const r3d::RigidBody *body)
        {
            // Check if this is the button object
            Entity *entity = static_cast<Entity *>(body->getUserData());
            return entity && entity->name == "Button 7ooda";
        }

        void onContact(const r3d::CollisionCallback::CallbackData &callbackData) override
        {
            for (uint32_t i = 0; i < callbackData.getNbContactPairs(); i++)
            {
                auto pair = callbackData.getContactPair(i);

                // Check if the pair involves the button entity
                if (isButton(static_cast<const r3d::RigidBody *>(pair.getBody1())) || isButton(static_cast<const r3d::RigidBody *>(pair.getBody2())))
                {
                    r3d::Body *body1 = pair.getBody1();
                    r3d::Body *body2 = pair.getBody2();

                    if (isButton(static_cast<const r3d::RigidBody *>(body2)))
                        std::swap(body1, body2);

                    void *userData = body1->getUserData();
                    Entity *button = static_cast<Entity *>(userData);
                    World* world = button->getWorld();

                    // Handle button press
                    for (uint32_t j = 0; j < pair.getNbContactPoints(); j++)
                    {
                        auto contactPoint = pair.getContactPoint(j);
                        r3d::Vector3 normal = contactPoint.getWorldNormal();

                        if (abs(normal.y) > 0.9f)
                        {
                            button->lastContactTime = getMyGameTime();
                            // printf("Button pressed at %f\n", button->lastContactTime);

                            if (button->delta < 0.2f)
                            {
                                printf("Delta: %f\n", button->delta);
                                r3d::Vector3 position = button->localTransform.getPosition();
                                button->localTransform.setPosition(glm::vec3(position.x, position.y - 0.01f, position.z));
                                body1->setTransform(r3d::Transform(r3d::Vector3(position.x, position.y - 0.01f, position.z), body1->getTransform().getOrientation()));
                                button->delta += 0.01f;

                                for (auto entity : world->getEntities())
                                {
                                    if (entity->name == "Cube")
                                    {
                                        entity->moveCube();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    };
}