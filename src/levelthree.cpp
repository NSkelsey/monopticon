#include "evenbettercap.h"

using namespace Monopticon::Level3;


Address::Address(UnsignedByte id, Object3D& object, Shaders::Flat3D& shader, Color3 &color, GL::Mesh& mesh, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& group):
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
    auto cm = camera.projectionMatrix();
    auto sphericalBBoardMatrix = Matrix4::from(cm.rotation(), tm.scaling()*tm.translation());
    auto b = camera.projectionMatrix()*sphericalBBoardMatrix;

    _shader.setColor(_color)
           .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix*_primitiveTransformation);
    //         .setTransformationProjectionMatrix(b);
    _mesh.draw(_shader);
}
