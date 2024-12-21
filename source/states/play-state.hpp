#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>
#include "../common/global/global.hpp"

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;

    float timeOffset;
    float lastTime;

    void onInitialize() override {
        world.win = false;
        cameraController.pickedEntity = nullptr;
        cameraController.picked = false;
        cameraController.originalDistance = 0.0f;
        cameraController.previousParent = nullptr;
        cameraController.previousScale = glm::vec3(1.0f);
        cameraController.currentScaleRatio = 1.0f;

        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if(config.contains("assets")){
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have phsyics world in the scene config, we use it to populate our world
        if(config.contains("physicsWorld")){
            world.deserializePhysicsWorld(config["physicsWorld"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if(config.contains("world")){
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);

        // Start the game time
        timeOffset = our::getMyGameTime();
        lastTime = timeOffset;
    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }

        // Print the time every second incrementally
        if(our::getMyGameTime() - lastTime > 1.0f){
            printf("Time: %f\n", our::getMyGameTime() - timeOffset);
            lastTime = our::getMyGameTime();
        }
        if(our::getMyGameTime() - timeOffset > 300.0f){
            getApp()->changeState("try-again");
        }
        if(world.win){
            getApp()->changeState("try-again");
        }
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};