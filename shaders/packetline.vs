uniform highp mat4 transformationProjectionMatrix;

layout(location = 0) in highp vec4 position;

out highp vec4 nposition;

void main() {
    gl_Position = transformationProjectionMatrix*position;
    nposition = position;
}
