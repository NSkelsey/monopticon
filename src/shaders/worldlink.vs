uniform highp mat4 transformationProjectionMatrix;
uniform highp vec3 originPos;
uniform highp vec2 screenPos;

layout(location = 0) in highp vec4 position;


void main() {

    float d1 = distance(originPos, position.xyz);

    if (d1 == 0) {
        gl_Position = transformationProjectionMatrix*position;
    } else {
        gl_Position = vec4(screenPos.x, screenPos.y, 0.0, 1.0);
    }
}
