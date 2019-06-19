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
    vec4 x = vec4(tParam);

    float t = tParam;

    float x_t = abs(bPos.x - aPos.x)*t+aPos.x;
    float y_t = abs(bPos.y - aPos.y)*t+aPos.y;
    float z_t = abs(bPos.z - aPos.z)*t+aPos.z;

    vec4 g1 = transformationProjectionMatrix*vec4(x_t, y_t, z_t, 1.0);
    //vec4 g1 = transformationProjectionMatrix*vec4(aPos, 1.0);
    vec3 g1_ndc = g1.xyz / g1.w;

    float t2 = tParam + 0.4;
    float x_t2 = abs(bPos.x - aPos.x)*t2 + aPos.x;
    float y_t2 = abs(bPos.y - aPos.y)*t2 + aPos.y;
    float z_t2 = abs(bPos.z - aPos.z)*t2 + aPos.z;

    vec4 g2 = transformationProjectionMatrix*vec4(x_t2, y_t2, z_t2, 1.0);
    //vec4 g2 = transformationProjectionMatrix*vec4(bPos, 1.0);
    vec3 g2_ndc = g2.xyz / g2.w;

    vec4 p = gl_FragCoord;

    vec2 viewSize = vec2(1400.0, 1000.0);
    vec2 viewOffset = vec2(2.0, 0.5);

    //vec3 ndcSpacePos = clipSpacePos.xyz / clipSpacePos.w;
    vec2 g1_win = ((g1_ndc.xy + 1.0) / 2.0) * viewSize + viewOffset;
    vec2 g2_win = ((g2_ndc.xy + 1.0) / 2.0) * viewSize + viewOffset;
    //vec2 p_win =  ((p_ndc.xy + 1.0) / 2.0) * viewSize + viewOffset;

    if ( within(g1_win.x, p.x, g2_win.x)) {
        discard;
    } else {
        fragmentColor = vec4(p.xy / viewSize, 0.0, 1.0);
    }
}
