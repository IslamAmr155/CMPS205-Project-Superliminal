#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"

namespace our {

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json& config){
        this->initialized = false; // We will set this to true when we finish the initialization of the renderer
        
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Then we check if there is a sky texture in the configuration
        if(config.contains("sky")){
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));
            
            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram* skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();
            
            //TODO: (Req 10) Pick the correct pipeline state to draw the sky
            // Hints: the sky will be draw after the opaque objects so we would need depth testing but which depth funtion should we pick?
            // We will draw the sphere from the inside, so what options should we pick for the face culling.
            PipelineState skyPipelineState{};
            skyPipelineState.faceCulling.enabled = true;
            // since we are drawing the sphere from the inside, we will cull the outside which was the front
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.depthTesting.enabled = true;
            // to prevent overdrawing on closer opaque objects
            skyPipelineState.depthTesting.function = GL_LEQUAL;
            
            // Load the sky texture (note that we don't need mipmaps since we want to avoid any unnecessary blurring while rendering the sky)
            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D* skyTexture = texture_utils::loadImage(skyTextureFile, false);

            // Setup a sampler for the sky 
            Sampler* skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Combine all the aforementioned objects (except the mesh) into a material 
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;
        }

        // Then we check if there is a postprocessing shader in the configuration
        if(config.contains("postprocess")){
            //TODO: (Req 11) Create a framebuffer
            glGenFramebuffers(1, &this->postprocessFrameBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->postprocessFrameBuffer);
            //TODO: (Req 11) Create a color and a depth texture and attach them to the framebuffer
            // Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for each channel).
            // The depth format can be (Depth component with 24 bits).
            colorTarget = texture_utils::empty(GL_RGB8, windowSize);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,colorTarget->getOpenGLName(),  0);
            depthTarget = texture_utils::empty(GL_DEPTH_COMPONENT24_SGIX, windowSize);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,depthTarget->getOpenGLName(),  0);
            //TODO: (Req 11) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler* postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram* postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;
        }
    }

    void ForwardRenderer::destroy(){
        // Delete all objects related to the sky
        if(skyMaterial){
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if(postprocessMaterial){
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    // Helper function to set up all the light sources
    void ForwardRenderer::setupLights(std::vector<LightComponent*> lights, ShaderProgram* shader) {
        for(int i = 0; i < lights.size(); i++) {
            LightComponent* light = lights[i];
            std::string prefix = "lights[" + std::to_string(i) + "].";
            printf("Setting up light %d\n", i);
            shader->set(prefix + "type", (int)light->type);
            printf("Type: %d\n", (int)light->type);
            shader->set(prefix + "color", light->color);
            printf("Color: %f %f %f\n", light->color.x, light->color.y, light->color.z);
            shader->set(prefix + "diffuse", light->diffuse);
            printf("Diffuse: %f %f %f\n", light->diffuse.x, light->diffuse.y, light->diffuse.z);
            shader->set(prefix + "specular", light->specular);
            printf("Specular: %f %f %f\n", light->specular.x, light->specular.y, light->specular.z);
            shader->set(prefix + "ambient", light->ambient);
            printf("Ambient: %f %f %f\n", light->ambient.x, light->ambient.y, light->ambient.z);
            r3d::Vector3 position = light->getOwner()->localTransform.getPosition();
            glm::vec3 positionVec = glm::vec3(position.x, position.y, position.z);
            switch (light->type) {
                case LightComponent::Type::DIRECTIONAL:
                    shader->set(prefix + "direction", glm::normalize(light->direction));
                    printf("Direction: %f %f %f\n", light->direction.x, light->direction.y, light->direction.z);
                    break;
                case LightComponent::Type::POINT:
                    shader->set(prefix + "position", positionVec);
                    printf("Position: %f %f %f\n", light->position.x, light->position.y, light->position.z);
                    shader->set(prefix + "attenuation.constant", light->attenuation.constant);
                    shader->set(prefix + "attenuation.linear", light->attenuation.linear);
                    shader->set(prefix + "attenuation.quadratic", light->attenuation.quadratic);
                    printf("Attenuation: %f %f %f\n", light->attenuation.constant, light->attenuation.linear, light->attenuation.quadratic);
                    break;
                case LightComponent::Type::SPOT:
                    shader->set(prefix + "position", positionVec);
                    printf("Position: %f %f %f\n", light->position.x, light->position.y, light->position.z);
                    shader->set(prefix + "direction", glm::normalize(light->direction));
                    printf("Direction: %f %f %f\n", light->direction.x, light->direction.y, light->direction.z);
                    shader->set(prefix + "attenuation.constant", light->attenuation.constant);
                    shader->set(prefix + "attenuation.linear", light->attenuation.linear);
                    shader->set(prefix + "attenuation.quadratic", light->attenuation.quadratic);
                    printf("Attenuation: %f %f %f\n", light->attenuation.constant, light->attenuation.linear, light->attenuation.quadratic);
                    shader->set(prefix + "spot_angle.inner", light->spot_angle.inner);
                    printf("Spot Angle Inner: %f\n", light->spot_angle.inner);
                    shader->set(prefix + "spot_angle.outer", light->spot_angle.outer);
                    printf("Spot Angle Outer: %f\n", light->spot_angle.outer);
                    break;
            }
        }
        shader->set("lightCount", (int)lights.size());
        printf("Light count: %d\n", (int)lights.size());
    }

    void ForwardRenderer::render(World* world){
        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent* camera = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();
        for(auto entity : world->getEntities()){
            // If we hadn't found a camera yet, we look for a camera in this entity
            if(!camera) camera = entity->getComponent<CameraComponent>();
            // If this entity has a mesh renderer component
            if(auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer){
                // We construct a command from it
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
                // if it is transparent, we add it to the transparent commands list
                if(command.material->transparent){
                    transparentCommands.push_back(command);
                } else {
                // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }
            // If this entity has a light component
            if(auto light = entity->getComponent<LightComponent>(); light){
                lights.push_back(light);
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if(camera == nullptr) return;

        //TODO: (Req 9) Modify the following line such that "cameraForward" contains a vector pointing the camera forward direction
        // HINT: See how you wrote the CameraComponent::getViewMatrix, it should help you solve this one

        // Getting the camera forward vector by multiplying the camera's local to world matrix by -1 in the z direction
        glm::vec3 cameraForward = camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        std::sort(transparentCommands.begin(), transparentCommands.end(), [cameraForward](const RenderCommand& first, const RenderCommand& second){
            //TODO: (Req 9) Finish this function
            // HINT: the following return should return true "first" should be drawn before "second".

            // Getting the distance of the first and second objects from the camera 
            float distance1 = glm::dot(cameraForward, first.center);
            float distance2 = glm::dot(cameraForward, second.center);

            // If the distance of the first object is greater than the distance of the second object, return true
            // We sort the objects from farthest to nearest to draw the transparent objects in the correct order
            return distance1 > distance2;
        });

        //TODO: (Req 9) Get the camera ViewProjection matrix and store it in VP
        // P * V = PV
        // PV*p will follow the order going from the world space to the view space then to the projection space (Homogeneous clip space)
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();
        //TODO: (Req 9) Set the OpenGL viewport using viewportStart and viewportSize
        // Lower left corner of the viewport to be (0, 0) and the size of the viewport to be the window size
        glViewport(0, 0, windowSize.x, windowSize.y);
        //TODO: (Req 9) Set the clear color to black and the clear depth to 1
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        //TODO: (Req 9) Set the color mask to true and the depth mask to true (to ensure the glClear will affect the framebuffer)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        // If there is a postprocess material, bind the framebuffer
        if(postprocessMaterial){
            //TODO: (Req 11) bind the framebuffer
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->postprocessFrameBuffer);
        }

        //TODO: (Req 9) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //TODO: (Req 9) Draw all the `opaque commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        glEnable(GL_DEPTH_TEST);
        for(auto command : opaqueCommands){
            // Setup the material
            command.material->setup();

            if(dynamic_cast<LitMaterial*>(command.material))
            {
                if (this->initialized == false) {
                    setupLights(lights, command.material->shader);
                }

                command.material->shader->set("M", command.localToWorld);
                command.material->shader->set("VP", VP);
                command.material->shader->set("M_IT", glm::transpose(glm::inverse(command.localToWorld)));
                glm::vec4 camera_position = (camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                command.material->shader->set("camera_position", glm::vec3(camera_position.x, camera_position.y, camera_position.z));
            } else {
                glm::mat4 transform = VP * command.localToWorld;
                command.material->shader->set("transform", transform);
            }

            // Draw the mesh
            command.mesh->draw();
        }
        // If there is a sky material, draw the sky
        if(this->skyMaterial){
            //TODO: (Req 10) setup the sky material
            this->skyMaterial->setup();
            //TODO: (Req 10) Get the camera position
            glm::vec4 cameraPosition = camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            //TODO: (Req 10) Create a model matrix for the sy such that it always follows the camera (sky sphere center = camera position)
            // translate sky with camera
           glm::mat4 skyModelMatrix = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                cameraPosition[0], cameraPosition[1], cameraPosition[2], 1.0f);
            //TODO: (Req 10) We want the sky to be drawn behind everything (in NDC space, z=1)
            // We can acheive the is by multiplying by an extra matrix after the projection but what values should we put in it?
            // [x, y, 1, 1]
            glm::mat4 alwaysBehindTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 1.0f
            );
            //TODO: (Req 10) set the "transform" uniform
            this->skyMaterial->shader->set("transform", alwaysBehindTransform * VP * skyModelMatrix);
            //TODO: (Req 10) draw the sky sphere
            this->skySphere->draw();
        }
        //TODO: (Req 9) Draw all the transparent commands
        // Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for (auto command : transparentCommands) {
            // Setup the material
            command.material->setup();

            if(dynamic_cast<LitMaterial*>(command.material))
            {
                if (this->initialized == false) {
                    setupLights(lights, command.material->shader);
                }

                command.material->shader->set("M", command.localToWorld);
                command.material->shader->set("VP", VP);
                command.material->shader->set("M_IT", glm::transpose(glm::inverse(command.localToWorld)));
                glm::vec4 camera_position = (camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                command.material->shader->set("camera_position", glm::vec3(camera_position.x, camera_position.y, camera_position.z));
            } else {
                glm::mat4 transform = VP * command.localToWorld;
                command.material->shader->set("transform", transform);
            }

            // Draw the mesh
            command.mesh->draw();
        }

        // If there is a postprocess material, apply postprocessing
        if(postprocessMaterial){
            //TODO: (Req 11) Return to the default framebuffer
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            //TODO: (Req 11) Setup the postprocess material and draw the fullscreen triangle
            this->postprocessMaterial->setup();
            glBindVertexArray(this->postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // We have finished rendering
        this->initialized = true;
    }
}