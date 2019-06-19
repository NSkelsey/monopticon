uniform highp mat4 transformationProjectionMatrix;
uniform highp vec3 aPos;
uniform highp vec3 bPos;
uniform highp float tParam;

layout(location = 0) in highp vec4 position;

vec3 calc_pos(vec3 a, vec3 b, float t) {
    vec3 c = vec3(0.0, -4.0, 0.0);

    vec3 pos = (1-t)*((1-t)*a + t*c) + t*((1-t)*c + t*b);

    return pos;
}


void main() {
    vec3 n_pos = position.xyz;
    float d1 = distance(n_pos, aPos);
    float d2 = distance(n_pos, bPos);

    vec4 choice;
    if (d1 > d2) {
        choice = vec4(calc_pos(aPos, bPos, tParam), 1.0);
    } else {
        float t2 = tParam + 0.03;
        choice = vec4(calc_pos(aPos, bPos, t2), 1.0);
    }


    gl_Position = transformationProjectionMatrix*choice;
}
