#pragma once

#include "../ecs/component.hpp"
#include "../mesh/mesh.hpp"
#include "../material/material.hpp"
#include "../asset-loader.hpp"

namespace our {

    // This component denotes that any renderer should draw the given mesh using the given material at the transformation of the owning entity.
    class LightComponent : public Component {
    public:
        // We will support 3 types of lights.
        // 1- Directional Light: where we assume that the light rays are parallel. We use this to approximate sun light.
        // 2- Point Light: where we assume that the light source is a single point that emits light in every direction. It can be used to approximate light bulbs.
        // 3- Spot Light: where we assume that the light source is a single point that emits light in the direction of a cone. It can be used to approximate torches, highway light poles.
        enum class Type {
            DIRECTIONAL,
            POINT,
            SPOT
        } type = Type::DIRECTIONAL;

        glm::vec3 color;

        // We also define the color & intensity of the light for each component of the Phong model (Ambient, Diffuse, Specular).
        glm::vec3 diffuse, specular, ambient;
        glm::vec3 position; // Used for Point and Spot Lights only
        glm::vec3 direction; // Used for Directional and Spot Lights only

        // This affects how the light will dim out as we go further from the light.
        // The formula is light_received = light_emitted / (a*d^2 + b*d + c) where a, b, c are the quadratic, linear and constant factors respectively.
        struct {
            float constant, linear, quadratic;
        } attenuation; // Used for Point and Spot Lights only

        // This specifies the inner and outer cone of the spot light.
        // The light power is 0 outside the outer cone, the light power is full inside the inner cone.
        // The light power is interpolated in between the inner and outer cone.
        struct {
            float inner, outer;
        } spot_angle; // Used for Spot Lights only

        // The ID of this component type is "Mesh Renderer"
        static std::string getID() { return "Light"; }

        // Receives the mesh & material from the AssetLoader by the names given in the json object
        void deserialize(const nlohmann::json& data) override;
    };

}