#include "evenbettercap.h"

using namespace Monopticon::Level3;


Address::Address(UnsignedByte id, Object3D& object, Shaders::Phong& shader, Color3 &color, GL::Mesh& mesh, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& group):
    SceneGraph::Drawable3D{object, &group},
    _id{id},
    _color{color},
    _shader{shader},
    _mesh{mesh},
    _primitiveTransformation{primitiveTransformation}
{

}


void Address::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    auto tm = transformationMatrix*_primitiveTransformation;

    _shader.setTransformationMatrix(tm)
           .setNormalMatrix(tm.rotationScaling())
           .setProjectionMatrix(camera.projectionMatrix())
           .setAmbientColor(_color*0.2)
           //.setTimeIntensity(1.0)
           .setLightColor(_color)
           /* relative to the camera */
           .setLightPosition({0.0f, 4.0f, 3.0f});
           //.setObjectId(255);

    _mesh.draw(_shader);
}
