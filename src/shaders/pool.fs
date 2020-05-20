out highp vec4 fragmentColor;
uniform lowp vec3 color;

void main() {
    //float alpha = 0.7 - tParam/2.0;

    fragmentColor = vec4(color.rgb, 0.7);
}
