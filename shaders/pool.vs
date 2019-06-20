uniform highp mat4 transformationProjectionMatrix;
uniform highp vec3 originPos;
uniform lowp float tParam;

layout(location = 0) in highp vec4 position;

vec3 calc_pos(vec3 a, vec3 b, float t) {
    return a*(t)*(t);
}

void main() {
    if (distance(position.xyz, originPos) < 0.001) {
        gl_Position = transformationProjectionMatrix*position;
    } else {

        vec3 new_pos = calc_pos(position.xyz, originPos, tParam);

        gl_Position = transformationProjectionMatrix*vec4(new_pos, 1.0);
    }
}
