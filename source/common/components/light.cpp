#include "light.hpp"
#include "deserialize-utils.hpp"

namespace our {
    // Receives the mesh & material from the AssetLoader by the names given in the json object
    void LightComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        
        int type = data.value("lightType", 0);
        switch(type){
            case 0: this->type = Type::DIRECTIONAL; break;
            case 1: this->type = Type::POINT; break;
            case 2: this->type = Type::SPOT; break;
        }

        this->color = data.value("color", glm::vec3(0.0f, 0.0f, 0.0f));

        this->diffuse = data.value("diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
        this->specular = data.value("specular", glm::vec3(1.0f, 1.0f, 1.0f));
        this->ambient = data.value("ambient", glm::vec3(0.1f, 0.1f, 0.1f));

        this->direction = data.value("direction", glm::vec3(0.0f, 0.0f, 0.0f));
        this->position = data.value("position", glm::vec3(0.0f, 0.0f, 0.0f));

        this->attenuation.constant = data.value("constant", 1.0f);
        this->attenuation.linear = data.value("linear", 0.0f);
        this->attenuation.quadratic = data.value("quadratic", 0.0f);

        this->spot_angle.inner = glm::radians(data.value("inner", 0.0f));
        this->spot_angle.outer = glm::radians(data.value("outer", 0.0f));
    }
}