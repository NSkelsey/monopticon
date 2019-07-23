#ifndef _INCL_ebc
#define _INCL_ebc

#include <array>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <math.h>
#include <memory>
#include <vector>
#include <set>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unistd.h>

#include "broker/broker.hh"
#include "broker/message.hh"
#include "broker/zeek.hh"

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pointer.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Resource.h>

#include <Magnum/Image.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Primitives/Grid.h>
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
#include <Magnum/Timeline.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>
#include <Magnum/Trade/MeshData3D.h>

#include <imgui.h>

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
    class RingDrawable;
    class MulticastDrawable;
    class WorldScreenLink;

  }

  namespace Device {

    class Stats;
    class WindowMgr;
    class ChartMgr;
    class RouteMgr;

  }

  namespace Level3 {

    class Address;

  }

// Definitions
namespace Util {

    struct L2Summary {
        int ipv4_cnt;
        int ipv6_cnt;
        int arp_cnt;
        int unknown_cnt;
    };

    L2Summary parseL2Summary(broker::vector *l2summary);

    int SumTotal(L2Summary struct_l2);

    Vector2 randCirclePoint();
    Vector2 paramCirclePoint(int num_elem, int pos);
    Vector2 randOffset(float z);
    Figure::RingDrawable* createLayoutRing(Scene3D &scene, SceneGraph::DrawableGroup3D &group, float r, Vector3 trans);


    void print_peer_subs();
    std::string exec_output(std::string cmd);
    std::vector<std::string> get_iface_list();

    enum L3Type {
        ARP = 'P',
        IPV4 = '4',
        IPV6 = '6',
        UNKNOWN = 'N'
    };

    Color3 typeColor(L3Type t);
}

namespace Device {

class Stats {
  public:
     Stats(std::string macAddr, Vector3 pos, Figure::DeviceDrawable *dev);
     ~Stats();

     std::string create_device_string();
     void renderText();

     void setSelected(bool selected);
     bool isSelected();

     void updateMaps(std::string ip_src_addr, std::string mac_dst);
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
     std::map<std::string, RouteMgr*>       _dst_arp_map;

     Vector3 circPoint;
     int num_pkts_sent;
     int num_pkts_recv;
     int health;
};

class PrefixStats {
  public:
    PrefixStats(std::string macPrefix, Vector3 pos, Figure::RingDrawable* ring);

    std::vector<std::pair<Figure::MulticastDrawable*, Figure::MulticastDrawable*>> contacts;

    std::string _prefix;
    Vector3     _position;
    Figure::RingDrawable *ring;

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

        bool _win_open;

        Figure::WorldScreenLink* _lineDrawable;

        int last_frame_tx;
        int last_frame_rx;
};

class ChartMgr {
    public:
        ChartMgr(int len, float f);
        void draw();
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

namespace Level3 {

class Address: public Object3D,  public SceneGraph::Drawable3D  {
  public:
    explicit Address(UnsignedByte id, Object3D& object, Shaders::Phong& shader, Color3 &color, GL::Mesh& mesh, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& drawables);

    std::string value;

  private:
    void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) override;

    UnsignedByte _id;
    Color3 _color;
    Shaders::Phong& _shader;
    GL::Mesh& _mesh;
    Matrix4 _primitiveTransformation;
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
        bool _drop;
        float _t;
};

class RingDrawable: public Object3D, public SceneGraph::Drawable3D {
    public:
        explicit RingDrawable(Object3D& object, const Color4& color, SceneGraph::DrawableGroup3D& group);

        RingDrawable& setMesh(Trade::MeshData3D mesh);

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

        ParaLineShader& setColor(const Color4& color);
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
        explicit PacketLineDrawable(Object3D& object, ParaLineShader& shader, Vector3& a, Vector3& b, SceneGraph::DrawableGroup3D& group, Color4 c);

        Object3D &_object;
        bool _expired;

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        GL::Mesh _mesh;
        ParaLineShader& _shader;
        Vector3 _a;
        Vector3 _b;
        float _t;
        Color4 _c;
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

        ~TextDrawable();

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

class PoolShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;

        explicit PoolShader();

        PoolShader& setColor(const Color3& color);
        PoolShader& setOriginPos(const Vector3& position);
        PoolShader& setTParam(const float t);
        PoolShader& setTransformationProjectionMatrix(const Matrix4& matrix);

    private:
        Int _colorUniform,
            _originPosUniform,
            _tParamUniform,
            _transformationProjectionMatrixUniform;
};

class MulticastDrawable: public SceneGraph::Drawable3D {
    public:
        explicit MulticastDrawable(Object3D& object, Color3 c, Vector3& origin, PoolShader& shader, SceneGraph::DrawableGroup3D& group, GL::Mesh& mesh);

        Object3D &_object;
        bool expired;

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        PoolShader& _shader;
        Color3 _c;
        Vector3 _origin;
        GL::Mesh &_mesh;
        float _t;
};

class WorldLinkShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;

        explicit WorldLinkShader();

        WorldLinkShader& setColor(const Color3& color);
        WorldLinkShader& setOriginPos(const Vector3& position);
        WorldLinkShader& setScreenPos(const Vector2& screenPos);
        WorldLinkShader& setTransformationProjectionMatrix(const Matrix4& matrix);

    private:
        Int _colorUniform,
            _originPosUniform,
            _screenPosUniform,
            _transformationProjectionMatrixUniform;
};

class WorldScreenLink: public SceneGraph::Drawable3D {
    public:
        explicit WorldScreenLink(Object3D& object, Color3 c, WorldLinkShader& shader, SceneGraph::DrawableGroup3D& group);

        Object3D &_object;

        WorldScreenLink& setCoords(Vector3& origin, Vector2& sp);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        WorldLinkShader& _shader;
        Color3 _c;
        Vector3 _origin;
        Vector2 _screenPos;
        GL::Mesh _mesh;
};

}

}

#endif
