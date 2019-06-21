uniform lowp vec3 color;

out highp vec4 fragmentColor;

void main() {
    fragmentColor = vec4(color, 1.0);
}
