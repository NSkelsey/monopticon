#extension GL_ARB_explicit_uniform_location: require
layout (location = 0) uniform lowp float tParam;
layout (location = 1) uniform lowp vec3 color;

out highp vec4 fragmentColor;

void main() {
    fragmentColor = vec4(color.x*tParam*0.5, color.y*tParam, color.z*tParam, tParam);
}
