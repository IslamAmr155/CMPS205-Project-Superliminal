#version 330 core

#define DIRECTIONAL 0
#define POINT 1
#define SPOT 2

#define MAX_LIGHTS 10

in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 world;
    vec3 view;
    vec3 normal;
} fs_in;

out vec4 frag_color;

struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};

struct Spot_angle {
    float inner;
    float outer;
};

struct Light {
    int type;   // 0 = Directional, 1 = Point, 2 = Spot
    vec3 color;

    // These define the position and direction of the light
    vec3 direction; // Directional light is only defined by a direction. In spot light, the direction is the direction of the spot light's cone axis.
    vec3 position;  // Point light is defined only by a position. In spot light, the position is the position of the light source.

    // These define the colors and intensities of the light
    vec3 diffuse;
    vec3 ambient;
    vec3 specular;

    // The attenuation is used to control how the light dims out as we go further from it.
    Attenuation attenuation;  

    // The angles define the spot light cone shape.
    // Inside the inner cone, the light intensity is full. Outside the outer angle, the light intensity is 0.
    // In between we use a smooth step to compute the light intensity.
    Spot_angle spot_angle;
};

struct Material {
    vec3 diffuse;
    vec3 ambient;
    vec3 specular;
    vec3 emissive;  // Extra property which is used when the pixel itself emits light.
    float shininess;
};

struct TexturedMaterial {
    sampler2D albedo_map;
    sampler2D specular_map;
    sampler2D roughness_map;
    sampler2D ambient_occlusion_map;
    sampler2D emissive_map;
    vec3 albedo_tint;
    vec3 specular_tint;
    vec2 roughness_range;
    vec3 emissive_tint;
};

// Receive the material and the light as uniforms.
uniform Light lights[MAX_LIGHTS];    // Maximum of 10 lights
uniform TexturedMaterial material;
uniform int lightCount;

// This will be used to compute the diffuse factor.
float calculate_lambert(vec3 normal, vec3 light_direction){
    return max(0.0f, dot(normal, -light_direction));
}

// This will be used to compute the phong specular.
float calculate_phong(vec3 normal, vec3 light_direction, vec3 view, float shininess){
    vec3 reflected = reflect(light_direction, normal);
    return pow(max(0.0f, dot(view, reflected)), shininess);
}

// This function samples the texture maps from the textured material and calculates the equivalent material at the given texture coordinates.
Material sample_material(TexturedMaterial tex_mat, vec2 tex_coord){
    Material mat;
    // Albedo is used to sample the diffuse
    mat.diffuse = tex_mat.albedo_tint * texture(tex_mat.albedo_map, tex_coord).rgb;
    // Specular is used to sample the specular... obviously
    mat.specular = tex_mat.specular_tint * texture(tex_mat.specular_map, tex_coord).rgb;
    // Emissive is used to sample the Emissive... once again "obviously"
    mat.emissive = tex_mat.emissive_tint * texture(tex_mat.emissive_map, tex_coord).rgb;
    // Ambient is computed by multiplying the diffuse by the ambient occlusion factor. This allows occluded crevices to look darker.
    mat.ambient = mat.diffuse * texture(tex_mat.ambient_occlusion_map, tex_coord).r;

    // Roughness is used to compute the shininess (specular power).
    float roughness = mix(tex_mat.roughness_range.x, tex_mat.roughness_range.y, texture(tex_mat.roughness_map, tex_coord).r);

    // We clamp the roughness to prevent its value from ever becoming 0 or 1 to prevent lighting artifacts.
    mat.shininess = 2.0f/pow(clamp(roughness, 0.001f, 0.999f), 4.0f) - 2.0f;

    return mat;
}

void main(){
    // Sample the material at the current pixel.
    Material sampled = sample_material(material, fs_in.tex_coord);

    // Normalize the normal and view since it can change during interpolation.
    vec3 normal = normalize(fs_in.normal);
    vec3 view = normalize(fs_in.view);

    // Initially, the accumulated light will hold the ambient and the emissive light.
    vec3 accumulated_light = sampled.emissive + sampled.ambient;

    int count = min(lightCount, MAX_LIGHTS);

    for (int i = 0; i < count; i++) {

        vec3 light_direction;
        float attenuation = 1.0f;

        if(lights[i].type == DIRECTIONAL){
            light_direction = lights[i].direction;
        } else {
            // Get the light direction and distance relative to the pixel location in the world space.
            light_direction = fs_in.world - lights[i].position;
            float distance = length(light_direction);
            light_direction /= distance;

            // Calculate the attenuation factor based on the light distance from the pixel.
            attenuation *= 1.0f / (lights[i].attenuation.constant + lights[i].attenuation.linear * distance + lights[i].attenuation.quadratic * distance * distance);

            if(lights[i].type == SPOT){
                // Calculate the angle between the pixel and the cone axis.
                float angle = acos(dot(lights[i].direction, light_direction));

                // Calculate the attenuation based on the angle.
                float angle_attenuation = smoothstep(lights[i].spot_angle.outer, lights[i].spot_angle.inner, angle);

                // Combine the attenuation and the angle attenuation.
                attenuation *= angle_attenuation;
            }
        }

        vec3 diffuse = sampled.diffuse * lights[i].color * calculate_lambert(normal, light_direction);
        vec3 specular = sampled.specular * lights[i].color * calculate_phong(normal, light_direction, view, sampled.shininess);

        accumulated_light += attenuation * (diffuse + specular);
    }

    frag_color = fs_in.color * vec4(accumulated_light, 1.0f);
}