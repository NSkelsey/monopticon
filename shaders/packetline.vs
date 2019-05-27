uniform highp mat4 transformationProjectionMatrix;

layout(location = 0) in highp vec4 position;

void main() {
    gl_Position = transformationProjectionMatrix*position;
}
