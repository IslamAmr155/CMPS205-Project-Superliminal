#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our {

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const {
        //TODO: (Req 7) Write this function

        // Setup the pipeline state
        pipelineState.setup();

        // Use the shader
        shader->use();
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;

        if(data.contains("pipelineState")){
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint 
    void TintedMaterial::setup() const {
        //TODO: (Req 7) Write this function

        // Call the parent material setup
        Material::setup();

        // Set the tint uniform so that a tinted material is used
        shader->set("tint", tint);
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json& data){
        Material::deserialize(data);
        if(!data.is_object()) return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable "tex" 
    void TexturedMaterial::setup() const {
        //TODO: (Req 7) Write this function

        // Call the material setup
        TintedMaterial::setup();

        // Set the alpha threshold
        shader->set("alphaThreshold", alphaThreshold);

        glActiveTexture(GL_TEXTURE0);
        // If the texture exists bind it
        if(texture)
            texture->bind();

        // If the sampler exists bind it
        if(sampler)
            sampler->bind(0);

        // Set the texture unit to the uniform variable "tex"
        shader->set("tex", 0);
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json& data){
        TintedMaterial::deserialize(data);
        if(!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
    }

    void LitMaterial::setup() const {
        // Call the material setup
        TintedMaterial::setup();

        // Set the albedo texture
        glActiveTexture(GL_TEXTURE0);
        albedo->bind();
        if(sampler)
            sampler->bind(0);
        shader->set("material.albedo_map", 0);
        shader->set("material.albedo_tint", albedo_tint);

        // Set the specular texture
        glActiveTexture(GL_TEXTURE1);
        specular->bind();
        if(sampler)
            sampler->bind(1);
        shader->set("material.specular_map", 1);
        shader->set("material.specular_tint", specular_tint);

        // Set the roughness texture
        glActiveTexture(GL_TEXTURE2);
        roughness->bind();
        if(sampler)
            sampler->bind(2);
        shader->set("material.roughness_map", 2);
        shader->set("material.roughness_range", roughness_range);

        // Set the ambient occlusion texture
        glActiveTexture(GL_TEXTURE3);
        ambient_occlusion->bind();
        if(sampler)
            sampler->bind(3);
        shader->set("material.ambient_occlusion_map", 3);

        // Set the emission texture
        glActiveTexture(GL_TEXTURE4);
        emission->bind();
        if(sampler)
            sampler->bind(4);
        shader->set("material.emissive_map", 4);
        shader->set("material.emissive_tint", emission_tint);
    }

    void LitMaterial::deserialize(const nlohmann::json& data){
        TintedMaterial::deserialize(data);
        if(!data.is_object()) return;
        albedo = AssetLoader<Texture2D>::get(data.value("albedo", ""));
        specular = AssetLoader<Texture2D>::get(data.value("specular", ""));
        roughness = AssetLoader<Texture2D>::get(data.value("roughness", ""));
        ambient_occlusion = AssetLoader<Texture2D>::get(data.value("ambient_occlusion", ""));
        emission = AssetLoader<Texture2D>::get(data.value("emissive", ""));
        albedo_tint = data.value("albedo_tint", glm::vec3(1.0f, 1.0f, 1.0f));
        specular_tint = data.value("specular_tint", glm::vec3(1.0f, 1.0f, 1.0f));
        roughness_range = data.value("roughness_range", glm::vec2(0.0f, 1.0f));
        emission_tint = data.value("emissive_tint", glm::vec3(1.0f, 1.0f, 1.0f));
    }
}