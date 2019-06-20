out highp vec4 fragmentColor;
uniform lowp float tParam;

void main() {

    float alpha = 0.7 - tParam/2.0;

    fragmentColor = vec4(0.0, 1.0, 1.0, alpha);
}
