#ifndef _INCL_ebc
#define _INCL_ebc

#include <array>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <math.h>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unistd.h>

#include <imgui.h>

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pointer.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Resource.h>
#include <Corrade/Utility/FormatStl.h>

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Shaders/MeshVisualizer.h>

#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/GL/Context.h>
#include <Magnum/Timeline.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Image.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/DistanceFieldVector.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/MeshVisualizer.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>
#include <Magnum/Trade/MeshData3D.h>

#include "broker/broker.hh"
#include "broker/message.hh"
#include "broker/zeek.hh"


using namespace Magnum;
using namespace Math::Literals;

typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D> Object3D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation3D> Scene3D;

// Generic hoists for intra-object references
namespace Monopticon {

namespace Figure {

    class DeviceDrawable;
    class PhongIdShader;
    class UnitBoardDrawable;
    class TextDrawable;
    class RouteDrawable;

}

namespace Device {

    class Stats;
    class WindowMgr;
    class ChartMgr;
    class RouteMgr;

}

// Definitions
namespace Util {

    Vector2 randCirclePoint();
    Vector2 paramCirclePoint(int num_elem, int pos);
    Vector2 randOffset(float z);
    void createLayoutRing(Scene3D &scene, SceneGraph::DrawableGroup3D &group, float r);

    void print_peer_subs();
    std::string exec_output(std::string cmd);
    std::vector<std::string> get_iface_list();

    enum L3Type {
        ARP = 'P',
        IPV4 = '4',
        IPV6 = '6',
        UNKNOWN = 'N'
    };

}

namespace Device {

class Stats {
  public:
     Stats(std::string macAddr, Vector2 pos, Figure::DeviceDrawable *dev);

     std::string create_device_string();
     void renderText();

     void setSelected(bool selected);
     bool isSelected();

     void updateMaps(std::string mac_src, std::string ip_src_addr, std::string mac_dst, std::string ip_dst_addr);
     std::string makeIpLabel();

     std::string                mac_addr;
     Figure::DeviceDrawable     *_drawable;
     Figure::UnitBoardDrawable  *_highlightedDrawable;

     Figure::TextDrawable   *_ip_label;
     Figure::TextDrawable   *_mac_label;
     WindowMgr              *_windowMgr;
     bool                   _selected;

     //map<std::string, std::string>    src_arp_map;
     std::vector<std::string>              _emitted_src_ips;
     std::map<std::string, RouteMgr>       _dst_arp_map;

     Vector2 circPoint;
     int num_pkts_sent;
     int num_pkts_recv;
     int health;
};

class WindowMgr {
    public:
        WindowMgr(Stats *d_s);
        void draw();

        Stats *_stats;
        std::vector<ChartMgr*> chartMgrList;
        ChartMgr* txChart;
        ChartMgr* rxChart;
        std::vector<std::string> announced_ips;

        int last_frame_tx;
        int last_frame_rx;
};

class ChartMgr {
    public:
        ChartMgr(int len, float f);
        void draw();
        void clear();
        void resize(int len);
        void push(float new_val);

        long unsigned int arr_len;
        std::vector<float> vec;

        float moving_avg;
        float max_val;
        float scaling_factor;
};

class RouteMgr {
    public:
        Device::Stats *src;
        Device::Stats *dst;

        Figure::TextDrawable  *label;
        Figure::RouteDrawable *path;
};

}

namespace Figure {

class PhongIdShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;
        typedef GL::Attribute<1, Vector3> Normal;

        enum: UnsignedInt {
            ColorOutput = 0,
            ObjectIdOutput = 1
        };

        explicit PhongIdShader();

        PhongIdShader& setObjectId(UnsignedInt id);
        PhongIdShader& setLightPosition(const Vector3& position);
        PhongIdShader& setAmbientColor(const Color3& color);
        PhongIdShader& setColor(const Color3& color);
        PhongIdShader& setTimeIntensity(const float t);
        PhongIdShader& setTransformationMatrix(const Matrix4& matrix);
        PhongIdShader& setNormalMatrix(const Matrix3x3& matrix);
        PhongIdShader& setProjectionMatrix(const Matrix4& matrix);

    private:
        Int _objectIdUniform,
        _lightPositionUniform,
        _ambientColorUniform,
        _colorUniform,
        _timeIntensityUniform,
        _transformationMatrixUniform,
        _normalMatrixUniform,
        _projectionMatrixUniform;
};

class DeviceDrawable: public SceneGraph::Drawable3D {
    public:
        explicit DeviceDrawable(UnsignedByte id, Object3D& object, PhongIdShader& shader, Color3 &color, GL::Mesh& mesh, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& drawables);

        Device::Stats * _deviceStats;
        void resetTParam();

    private:
        void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) override;

        UnsignedByte _id;
        Color3 _color;
        PhongIdShader& _shader;
        GL::Mesh& _mesh;
        Matrix4 _primitiveTransformation;
        float _t;
};

class RingDrawable: public SceneGraph::Drawable3D {
    public:
        explicit RingDrawable(Object3D& object, const Color4& color, SceneGraph::DrawableGroup3D& group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Matrix4 scaling = Matrix4::scaling(Vector3{10});
        GL::Mesh _mesh;
        Color4 _color;
        Shaders::MeshVisualizer _shader;
};

class ParaLineShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;

        explicit ParaLineShader();

        ParaLineShader& setColor(const Color3& color);

        ParaLineShader& setAPos(const Vector3& position);

        ParaLineShader& setBPos(const Vector3& position);

        ParaLineShader& setTParam(const float t);

        ParaLineShader& setTransformationProjectionMatrix(const Matrix4& matrix);

    private:
        Int _colorUniform,
            _aPosUniform,
            _bPosUniform,
            _tParamUniform,
            _transformationProjectionMatrixUniform;
};

class PacketLineDrawable: public SceneGraph::Drawable3D {
    public:
        explicit PacketLineDrawable(Object3D& object, ParaLineShader& shader, Vector3& a, Vector3& b, SceneGraph::DrawableGroup3D& group, Color3 c);

        Object3D &_object;
        bool _expired;

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        GL::Mesh _mesh;
        ParaLineShader& _shader;
        Vector3 _a;
        Vector3 _b;
        float _t;
};

class UnitBoardDrawable: public SceneGraph::Drawable3D {
    public:
        explicit UnitBoardDrawable(Object3D& object, Shaders::Flat3D& shader, SceneGraph::DrawableGroup3D& group, Color3 c);

        Object3D &_object;

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        GL::Mesh _mesh;
        Shaders::Flat3D& _shader;
};

class TextDrawable: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit TextDrawable(std::string msg,
                Color3 c,
                Containers::Pointer<Text::AbstractFont> &font,
                Text::DistanceFieldGlyphCache *cache,
                Shaders::DistanceFieldVector3D& shader,
                Object3D& parent,
                SceneGraph::DrawableGroup3D& drawables);

        void updateText(std::string s);

    private:
        virtual void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera);

        Color3 _c;
        Containers::Pointer<Text::Renderer3D> _textRenderer;
        Shaders::DistanceFieldVector3D& _shader;
};

class RouteDrawable: public SceneGraph::Drawable3D {
    public:
        explicit RouteDrawable(Object3D& object, Vector3& a, Vector3& b, Shaders::Flat3D& shader, SceneGraph::DrawableGroup3D& group);

        Object3D &_object;

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        GL::Mesh _mesh;
        Shaders::Flat3D& _shader;
        Vector3 _a;
        Vector3 _b;
};


}
}

#endif
