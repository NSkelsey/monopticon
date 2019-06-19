#extension GL_ARB_explicit_uniform_location: require
uniform lowp float tParam;
uniform lowp vec3 color;
uniform highp vec3 aPos;
uniform highp vec3 bPos;
uniform highp mat4 transformationProjectionMatrix;

in highp vec4 nposition;
in vec4 gl_FragCoord;

out highp vec4 fragmentColor;

bool within(float a, float b, float c) {
    return ((a <= b) && (b <= c)) || ((a >= b) && (b >= c));
}

void main() {
    fragmentColor = vec4(color, 1.0);
}
