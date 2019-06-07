#extension GL_ARB_explicit_uniform_location: require
uniform lowp float tParam;
uniform lowp vec3 color;
uniform highp vec3 aPos;
uniform highp vec3 bPos;

in highp vec4 nposition;

out highp vec4 fragmentColor;

void main() {
    vec4 x = vec4(tParam);

    float x_t = (bPos.x - aPos.x)*tParam+aPos.x;
    float m = (bPos.z - aPos.z)/(bPos.x - aPos.x);
    float q = aPos.z - m*aPos.x;
    float y_t = m*x_t;

    if (nposition.x > x_t && nposition.x < (x_t + 0.6)) {
        fragmentColor = vec4(color.x, color.y, color.z, 1.0);
    } else {
        discard;
    }
}
