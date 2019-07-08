uniform lowp vec3 color;

out highp vec4 fragmentColor;

void main() {
    fragmentColor = vec4(color, 0.5);
}
