#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal; // Now we need the surface normal to compute the light so we will send it as an attribute.

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 world; // We will need to send the vertex position in the world space, so we can calculate the light direction in the fragment shader for point and spot lights
    vec3 view; // the view vector (vertex to eye vector in the world space),
    vec3 normal; // and the surface normal in the world space.
} vs_out;

// We will need to do the light processing in the world space so we will break our transformations into 2 stages:
// 1- Object to World.
uniform mat4 M;
uniform mat4 M_IT; // The inverse transpose will be used to transform the surface normal.
// 2- World to Homogenous Clipspace.
uniform mat4 VP;
// The camera position will be used for specular computation.
uniform vec3 camera_position;

void main(){
    // First we compute the world position.
    vs_out.world = (M * vec4(position, 1.0)).xyz;
    // Then we compute the view vector (vertex to eye vector in the world space) to be used for specular computation later.
    vs_out.view = camera_position - vs_out.world;
    // Then we compute normal in the world space (Note that w=0 since this is a vector).
    vs_out.normal = normalize(M_IT * vec4(normal, 0.0)).xyz;

    // Finally, we compute the position in the homogenous clip space and send the rest of the data.
    gl_Position = VP * vec4(vs_out.world, 1.0);
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
}