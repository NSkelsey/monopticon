#include "evenbettercap.h"

using namespace Monopticon::Figure;

PhongIdShader::PhongIdShader() {
    Utility::Resource rs("picking-data");

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex},
        frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(rs.get("shaders/phongid.vs"));
    frag.addSource(rs.get("shaders/phongid.fs"));
    CORRADE_INTERNAL_ASSERT(GL::Shader::compile({vert, frag}));
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _objectIdUniform = uniformLocation("objectId");
    _lightPositionUniform = uniformLocation("light");
    _ambientColorUniform = uniformLocation("ambientColor");
    _colorUniform = uniformLocation("color");
    _timeIntensityUniform = uniformLocation("timeIntensity");
    _transformationMatrixUniform = uniformLocation("transformationMatrix");
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _normalMatrixUniform = uniformLocation("normalMatrix");
}

PhongIdShader& PhongIdShader::setObjectId(UnsignedInt id) {
    setUniform(_objectIdUniform, id);
    return *this;
}

PhongIdShader& PhongIdShader::setLightPosition(const Vector3& position) {
    setUniform(_lightPositionUniform, position);
    return *this;
}

PhongIdShader& PhongIdShader::setAmbientColor(const Color3& color) {
    setUniform(_ambientColorUniform, color);
    return *this;
}

PhongIdShader& PhongIdShader::setColor(const Color3& color) {
    setUniform(_colorUniform, color);
    return *this;
}

PhongIdShader& PhongIdShader::setTimeIntensity(const float t) {
    setUniform(_timeIntensityUniform, t);
    return *this;
}

PhongIdShader& PhongIdShader::setTransformationMatrix(const Matrix4& matrix) {
    setUniform(_transformationMatrixUniform, matrix);
    return *this;
}

PhongIdShader& PhongIdShader::setNormalMatrix(const Matrix3x3& matrix) {
    setUniform(_normalMatrixUniform, matrix);
    return *this;
}

PhongIdShader& PhongIdShader::setProjectionMatrix(const Matrix4& matrix) {
    setUniform(_projectionMatrixUniform, matrix);
    return *this;
}

DeviceDrawable::DeviceDrawable(UnsignedByte id, Object3D& object, PhongIdShader& shader, Color3 &color, GL::Mesh& mesh, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& drawables):
    SceneGraph::Drawable3D{object, &drawables},
    _id{id},
    _color{color},
    _shader(shader),
    _mesh(mesh),
    _primitiveTransformation{primitiveTransformation},
    _t{0.0f} {}

void DeviceDrawable::resetTParam() {
    _t = 1.0f;
}

void DeviceDrawable::draw(const Matrix4& transformation, SceneGraph::Camera3D& camera){
    if (_t > 0.0f) {
        _t = _t - 0.01f;
    }

    if (_deviceStats->health > 0) {
        _deviceStats->health --;
    _shader.setTransformationMatrix(transformation*_primitiveTransformation)
           .setNormalMatrix(transformation.rotationScaling())
           .setProjectionMatrix(camera.projectionMatrix())
           .setAmbientColor(_deviceStats->_selected ? _color*0.2f : Color3{})
           .setColor(_color*(_deviceStats->_selected ? 1.5f : 0.9f))
           .setTimeIntensity(_t)
           /* relative to the camera */
           .setLightPosition({0.0f, 4.0f, 3.0f})
           .setObjectId(_id);
    _mesh.draw(_shader);
    }
}

RingDrawable::RingDrawable(Object3D& object, const Color4& color, SceneGraph::DrawableGroup3D& group):
            SceneGraph::Drawable3D{object, &group}
{
    _mesh = MeshTools::compile(Primitives::circle3DWireframe(70));
    _color = color;
    _shader = Shaders::MeshVisualizer{Shaders::MeshVisualizer::Flag::Wireframe|Shaders::MeshVisualizer::Flag::NoGeometryShader};
}

void RingDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    using namespace Math::Literals;

    _shader.setColor(0xffffff_rgbf)
           .setWireframeColor(_color)
           .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix);
    _mesh.draw(_shader);
}

ParaLineShader::ParaLineShader() {
    Utility::Resource rs("picking-data");

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex},
        frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(rs.get("shaders/packetline.vs"));
    frag.addSource(rs.get("shaders/packetline.fs"));
    CORRADE_INTERNAL_ASSERT(GL::Shader::compile({vert, frag}));
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _aPosUniform = uniformLocation("aPos");
    _bPosUniform = uniformLocation("bPos");
    _colorUniform = uniformLocation("color");
    _tParamUniform = uniformLocation("tParam");
    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");

    //CORRADE_INTERNAL_ASSERT(_bPosUniform >= 0);
}


ParaLineShader& ParaLineShader::setColor(const Color3& color) {
    setUniform(_colorUniform, color);
    return *this;
}

ParaLineShader& ParaLineShader::setAPos(const Vector3& position) {
    setUniform(_aPosUniform, position);
    return *this;
}

ParaLineShader& ParaLineShader::setBPos(const Vector3& position) {
    setUniform(_bPosUniform, position);
    return *this;
}

ParaLineShader& ParaLineShader::setTParam(const float t) {
    setUniform(_tParamUniform, t);
    return *this;
}

ParaLineShader& ParaLineShader::setTransformationProjectionMatrix(const Matrix4& matrix) {
    setUniform(_transformationProjectionMatrixUniform, matrix);
    return *this;
}

PacketLineDrawable::PacketLineDrawable(Object3D& object, ParaLineShader& shader, Vector3& a, Vector3& b, SceneGraph::DrawableGroup3D& group, Color3 c):
    SceneGraph::Drawable3D{object, &group},
    _object{object},
    _shader{shader},
    _a{a},
    _b{b}
{
    _t = 0.0f;
    _mesh = MeshTools::compile(Primitives::line3D(a,b));
    _expired = false;
    _shader.setColor(c);
}

void PacketLineDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    if (_t < 1.001f && _t >= 0.0f) {
        _t += 0.04f;
        //Vector3 v = Vector3{0.02f, 0, 0.02f};
        //_object.translate(v);
    }
    if (_t > 1.0f) {
        _expired=true;
        return;
    }

    _shader.setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
           .setBPos(_b)
           .setAPos(_a)
           .setTParam(_t);
    _mesh.draw(_shader);
}
