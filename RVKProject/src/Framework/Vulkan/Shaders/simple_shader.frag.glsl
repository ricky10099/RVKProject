#version 450
#pragma shader_stage(fragment)
#extension GL_KHR_vulkan_glsl: enable

#include "../SharedDefines.h"

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragUV;

layout (location = 0) out vec4 outColor;

struct PointLight {
  vec4 position; // ignore w
  vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 invView;
  vec4 ambientLightColor; // w is intensity
  PointLight pointLights[MAX_LIGHTS];
  int numLights;
} ubo;

layout (set = 1, binding = 0) uniform MaterialUbo {
    int features;
    float roughness;
    float metallic;

    // byte 16 to 31
    vec4 diffuseColor;

    // byte 32 to 47
    vec3 emissiveColor;
    float emissiveStrength;

    // byte 48 to 63
    float normalMapIntensity;
}matUbo;

layout (set = 1, binding = 1) uniform sampler2D diffuseMap;


layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

void main() {
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.view[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    vec4 textureColor;
    if(bool(matUbo.features & GLSL_HAS_DIFFUSE_MAP)) {
        textureColor = texture(diffuseMap, fragUV) * matUbo.diffuseColor;
    }else{
        textureColor = fragColor;
    }
    if(textureColor.a < 0.5) {
        discard;
    }

    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        // specular lighting
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 512.0); // higher values -> sharper highlight
        specularLight += intensity * blinnTerm;
    }
  
    outColor = vec4(diffuseLight, 1.0) * textureColor + vec4(specularLight, 1.0);
}
