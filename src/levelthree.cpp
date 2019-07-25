#include "evenbettercap.h"

using namespace Monopticon::Level3;


Address::Address(UnsignedByte id, Object3D& object, Figure::PhongIdShader& shader, Color3 &color, GL::Mesh& mesh, SceneGraph::DrawableGroup3D& group):
    Object3D{&object},
    SceneGraph::Drawable3D{object, &group},
    _id{id},
    _color{color},
    _shader{shader},
    _mesh{mesh}
{

}


void Address::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    auto tm = transformationMatrix;

    _shader.setTransformationMatrix(tm)
           .setNormalMatrix(tm.rotationScaling())
           .setProjectionMatrix(camera.projectionMatrix())
           .setAmbientColor(_color*0.2)
           //.setTimeIntensity(1.0)
           .setColor(_color)
           /* relative to the camera */
           .setLightPosition({0.0f, 4.0f, 3.0f})
           .setObjectId(_id+1);

    _mesh.draw(_shader);
}

Vector3 Address::getTranslation() {
    // TODO use _absoluteTransformationCache
    return Vector3{};
}


Object3D& Address::getObj() {
    return *this;
}
