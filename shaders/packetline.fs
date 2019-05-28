#extension GL_ARB_explicit_uniform_location: require
layout (location = 0) uniform lowp float tParam;
layout (location = 1) uniform lowp vec3 color;

out highp vec4 fragmentColor;

void main() {
    vec4 x = vec4(tParam);
    fragmentColor = vec4(color.x, color.y, color.z, 0.0) + vec4(tParam);
}
