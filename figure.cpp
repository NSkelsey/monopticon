#include "evenbettercap.h"

using namespace Monopticon::Figure;


PhongIdShader::PhongIdShader() {
    Utility::Resource rs("monopticon");

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


RingDrawable::RingDrawable(Object3D& object, const Color4& color, SceneGraph::DrawableGroup3D& group):
        Object3D{&object},
        SceneGraph::Drawable3D{object, &group}
{
    _mesh = MeshTools::compile(Primitives::circle3DWireframe(70));
    _color = color;
    _shader = Shaders::MeshVisualizer{Shaders::MeshVisualizer::Flag::Wireframe|Shaders::MeshVisualizer::Flag::NoGeometryShader};
}

RingDrawable& RingDrawable::setMesh(Trade::MeshData3D mesh) {
    _mesh = MeshTools::compile(mesh);
    return *this;
}

void RingDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader.setColor(0xffffff_rgbf)
           .setWireframeColor(_color)
           .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix);
    _mesh.draw(_shader);
}


ParaLineShader::ParaLineShader() {
    Utility::Resource rs("monopticon");

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
    // TODO more efficient to initialize in caller
    _mesh = MeshTools::compile(Primitives::line3D(a,b));
    _expired = false;
    _shader.setColor(c);
}

void PacketLineDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    if (_t <= 1.000f && _t >= 0.0f) {
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


UnitBoardDrawable::UnitBoardDrawable(Object3D& object, Shaders::Flat3D& shader, SceneGraph::DrawableGroup3D& group, Color3 c):
    SceneGraph::Drawable3D{object, &group},
    _object{object},
    _shader{shader}
{
    _mesh = MeshTools::compile(Primitives::planeWireframe());
    _shader.setColor(c);
}

void UnitBoardDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    auto tm = transformationMatrix;
    auto cm = camera.projectionMatrix();
    auto sphericalBBoardMatrix = Matrix4::from(cm.rotation(), tm.scaling()*tm.translation());
    auto b = camera.projectionMatrix()*sphericalBBoardMatrix;

    _shader.setTransformationProjectionMatrix(b);
    _mesh.draw(_shader);
}


TextDrawable::TextDrawable(std::string msg,
        Color3 c,
        Containers::Pointer<Text::AbstractFont> &font,
        Text::DistanceFieldGlyphCache *cache,
        Shaders::DistanceFieldVector3D& shader,
        Object3D& parent,
        SceneGraph::DrawableGroup3D& drawables):
    _c{c},
    Object3D{&parent},
    SceneGraph::Drawable3D{*this, &drawables},
    _shader(shader)
{

    _textRenderer.reset(new Text::Renderer3D(*font.get(), *cache, 0.030f, Text::Alignment::LineCenter));
    // TODO Note hardcoded limit:
    _textRenderer->reserve(1000, GL::BufferUsage::DynamicDraw, GL::BufferUsage::StaticDraw);

    updateText(msg);
}

void TextDrawable::updateText(std::string s) {
    _textRenderer->render(s);
}

void TextDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    auto tm = transformationMatrix;
    auto cm = camera.projectionMatrix();
    auto sphericalBBoardMatrix = Matrix4::from(cm.rotation(), tm.scaling()*tm.translation());
    auto b = cm*sphericalBBoardMatrix;

    _shader.setTransformationProjectionMatrix(cm*tm)
           .setOutlineColor(_c)
           .setColor(_c)
           .setSmoothness(0.025f/transformationMatrix.uniformScaling());

    _textRenderer->mesh().draw(_shader);

    _shader.setTransformationProjectionMatrix(b);
    _textRenderer->mesh().draw(_shader);
}


RouteDrawable::RouteDrawable(Object3D& object, Vector3& a, Vector3& b, Shaders::Flat3D& shader, SceneGraph::DrawableGroup3D& group):
    SceneGraph::Drawable3D{object, &group},
    _object{object},
    _shader{shader},
    _a{a},
    _b{b}
{
    Vector3 int_point = (_b + _a)/3.0f;

    _mesh = MeshTools::compile(Primitives::line3D(a,int_point));
}

void RouteDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader.setColor(0xffffff_rgbf)
           .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix);
    _mesh.draw(_shader);
}



PoolShader::PoolShader() {
    Utility::Resource rs("monopticon");

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex},
        frag{GL::Version::GL330, GL::Shader::Type::Fragment};
    vert.addSource(rs.get("shaders/pool.vs"));
    frag.addSource(rs.get("shaders/pool.fs"));
    CORRADE_INTERNAL_ASSERT(GL::Shader::compile({vert, frag}));
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _originPosUniform = uniformLocation("originPos");
    _colorUniform = uniformLocation("color");
    _tParamUniform = uniformLocation("tParam");
    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
}

PoolShader& PoolShader::setColor(const Color3& color) {
    setUniform(_colorUniform, color);
    return *this;
}

PoolShader& PoolShader::setOriginPos(const Vector3& position) {
    setUniform(_originPosUniform, position);
    return *this;
}

PoolShader& PoolShader::setTParam(const float t) {
    setUniform(_tParamUniform, t);
    return *this;
}

PoolShader& PoolShader::setTransformationProjectionMatrix(const Matrix4& matrix) {
    setUniform(_transformationProjectionMatrixUniform, matrix);
    return *this;
}

MulticastDrawable::MulticastDrawable(Object3D& object, Color3 c, Vector3& origin, PoolShader& shader, SceneGraph::DrawableGroup3D& group, GL::Mesh& mesh):
    SceneGraph::Drawable3D{object, &group},
    expired{false},
    _object{object},
    _shader{shader},
    _c{c},
    _origin{origin},
    _mesh{mesh}
{
    _t = -1.0f;
}

void MulticastDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    if (expired || _t > 1.4f) {
        expired = true;
        return;
    }

    _t += 0.04f;

    _shader.setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
           .setOriginPos(_origin)
           .setColor(_c)
           .setTParam(_t);
    _mesh.draw(_shader);
}
