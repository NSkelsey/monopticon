#include "evenbettercap.h"

using namespace Magnum;
using namespace Monopticon;

 Movement::Ray Movement::getCameraToViewportRay(SceneGraph::Camera3D& camera, const Vector2& screenPoint) {
  Matrix4 inverseVP = (camera.projectionMatrix() * camera.cameraMatrix()).inverted();

  Float nx = (2.0f * screenPoint.x()) - 1.0f;
  Float ny = 1.0f - (2.0f * screenPoint.y());

  Vector3 nearPoint(nx, ny, -1.f);
  Vector3 midPoint (nx, ny,  0.0f);

  Vector3 rayOrigin = inverseVP.transformPoint(nearPoint);
  Vector3 rayTarget = inverseVP.transformPoint(midPoint);
  Vector3 rayDirection = (rayTarget - rayOrigin).normalized();

  return { rayOrigin, rayDirection };
}

class ObjectRenderer : SceneGraph::Drawable3D {
  public:
    ObjectRenderer(Object3D &parent, SceneGraph::DrawableGroup3D *group, std::function<bool(void)> drawCheck) : SceneGraph::Drawable3D{parent, group}, _drawCheck(drawCheck) {

      /* Create the mesh */
      GL::Buffer vertexBuffer{GL::Buffer::TargetHint::Array};
      GL::Buffer indexBuffer{GL::Buffer::TargetHint::ElementArray};

      auto data = Primitives::axis3D();

      vertexBuffer.setData(MeshTools::interleave(data.positions(0), data.colors(0)), GL::BufferUsage::StaticDraw);
      indexBuffer.setData(MeshTools::compressIndicesAs<UnsignedByte>(data.indices()), GL::BufferUsage::StaticDraw);

      _mesh.setPrimitive(GL::MeshPrimitive::Lines)
        .setCount(data.indices().size())
        .addVertexBuffer(std::move(vertexBuffer), 0, Shaders::VertexColor3D::Position(), Shaders::VertexColor3D::Color4{})
        .setIndexBuffer(std::move(indexBuffer), 0, GL::MeshIndexType::UnsignedByte, 0, data.positions(0).size());
    }

  private:
    void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
      if (_drawCheck()) {
        _shader.setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        _mesh.draw(_shader);
      }
    }

    Shaders::VertexColor3D _shader;
    GL::Mesh _mesh;
    std::function<bool(void)> _drawCheck;
};

Movement::TranslateController::TranslateController(Object3D *parent, SceneGraph::DrawableGroup3D *group) : Object3D(parent)  {
  new ObjectRenderer{*this, group, [this](){
    return children().first();
  }};
}

void Movement::TranslateController::move(const Ray &cameraRay) {
  if (_missed) {
    return;
  }

  Float t = Math::Intersection::planeLine(_plane, cameraRay.origin, cameraRay.direction);

  if (Magnum::Math::isInf(t) || Magnum::Math::isNan(t)) {
    return;
  }

  Vector3 currentPoint = cameraRay.origin + cameraRay.direction * t;

  Matrix4 tr = transformation();
  tr.translation() = _startPosition + _dir * (currentPoint - _startPoint);
  setTransformation(tr);
}

void Movement::TranslateController::grab(const Ray &cameraRay) {
  if (!children().first()) {
    return;
  }

  _startPosition = transformation().translation();
  _missed = true;

  // check x-z
  _plane = Math::planeEquation(Vector3::yAxis(1), _startPosition);
  Float t = Math::Intersection::planeLine(_plane, cameraRay.origin, cameraRay.direction);

  if (Magnum::Math::isInf(t) || Magnum::Math::isNan(t)) {
    return;
  }

  _startPoint = cameraRay.origin + cameraRay.direction * t;
  Vector3 p = _startPoint - _startPosition;

  if (Math::abs(p.z()) < _axisThreshold && p.x() > 0 && p.x() <= 1) {
    _dir = Vector3::xAxis(1);
    _missed = false;
    return;
  }

  if (Math::abs(p.x()) < _axisThreshold && p.z() > 0 && p.z() <= 1) {
    _dir = Vector3::zAxis(1);
    _missed = false;
    return;
  }

  // check y
  _plane = Math::planeEquation(Vector3::xAxis(1), _startPosition);
  t = Math::Intersection::planeLine(_plane, cameraRay.origin, cameraRay.direction);

  if (Magnum::Math::isInf(t) || Magnum::Math::isNan(t)) {
    return;
  }

  _startPoint = cameraRay.origin + cameraRay.direction * t;
  p = _startPoint - _startPosition;

  if (Math::abs(p.z()) < _axisThreshold && p.y() > 0 && p.y() <= 1) {
    _dir = Vector3::yAxis(1);
    _missed = false;
    return;
  }
}

void Movement::TranslateController::release() {
  _missed = true;
}
