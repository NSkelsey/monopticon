uniform highp mat4 transformationProjectionMatrix;
uniform highp vec3 aPos;
uniform highp vec3 bPos;
uniform highp float tParam;

layout(location = 0) in highp vec4 position;

out highp vec4 nposition;


vec3 calc_pos(vec3 a, vec3 b, float t) {
    vec3 c = vec3(0.0, -4.0, 0.0);

    //vec3 pos = (b-a)*(3*(1-t)*(1-t)*t) + a;

    vec3 pos = (1-t)*((1-t)*a + t*c) + t*((1-t)*c + t*b);

    /*
    float x_t = abs(b.x - a.x)*t+a.x;
    float y_t = abs(b.y - a.y)*t+a.y;
    float z_t = abs(b.z - a.z)*t+a.z;
    */


    return pos;
}


void main() {
    vec3 n_pos = position.xyz;
    float d1 = distance(n_pos, aPos);
    float d2 = distance(n_pos, bPos);

    vec4 choice;
    if (d1 > d2) {
        choice = vec4(calc_pos(aPos, bPos, tParam), 1.0);

        //choice = vec4(aPos, 1.0);
    } else {
        float t2 = tParam + 0.03;
        choice = vec4(calc_pos(aPos, bPos, t2), 1.0);
        //choice = vec4(4.0,4.0,4.0, 1.0);
    }


    gl_Position = transformationProjectionMatrix*choice;
}
