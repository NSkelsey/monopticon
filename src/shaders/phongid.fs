uniform lowp vec3 ambientColor;
uniform lowp vec3 color;
uniform lowp float timeIntensity;
uniform lowp uint objectId;

in mediump vec3 transformedNormal;
in highp vec3 lightDirection;
in highp vec3 cameraDirection;

layout(location = 0) out lowp vec4 fragmentColor;
layout(location = 1) out lowp uint fragmentObjectId;

void main() {
    mediump vec3 normalizedTransformedNormal = normalize(transformedNormal);
    highp vec3 normalizedLightDirection = normalize(lightDirection);

    /* Add ambient color */
    lowp vec3 t = ambientColor * timeIntensity;
    //lowp vec3 t = ambientColor;
    fragmentColor.rgb = t;

    /* Add diffuse color */
    lowp float intensity = max(0.0, dot(normalizedTransformedNormal, normalizedLightDirection));
    fragmentColor.rgb += color*(intensity);

    /* Add specular color, if needed */
    if(intensity > 0.001) {
        highp vec3 reflection = reflect(-normalizedLightDirection, normalizedTransformedNormal);
        mediump float specularity = pow(max(0.0, dot(normalize(cameraDirection), reflection)), 80.0);
        fragmentColor.rgb += vec3(1.0)*specularity;
    }

    /* Force alpha to 1 */
    fragmentColor.a = 1.0;
    fragmentObjectId = objectId;
}
