#ifndef _INCL_ebc
#define _INCL_ebc

#include <array>
#include <cstdio>
#include <ctime>
#include <iomanip>
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
#include <map>
#include <unistd.h>

#include <SDL.h>

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
#include <Magnum/Math/Intersection.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Axis.h>
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
#include <Magnum/Shaders/VertexColor.h>
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

  namespace Parse {
      class BrokerCtx;
  }

// Definitions
namespace Util {

    struct L2Summary {
        int ipv4_cnt;
        int ipv6_cnt;
        int arp_cnt;
        int unknown_cnt;
    };

    //L2Summary parseL2Summary(broker::vector *l2summary);

    int SumTotal(L2Summary struct_l2);

    Vector2 randCirclePoint();
    Vector2 paramCirclePoint(int num_elem, int pos);
    Vector2 randOffset(float z);
    Figure::RingDrawable* createLayoutRing(Object3D &parent, SceneGraph::DrawableGroup3D &group, float r, Vector3 trans);


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

class Selectable {
  public:
    virtual ~Selectable();
    void addHighlight(Shaders::Flat3D& shader, SceneGraph::DrawableGroup3D &group);
    bool isSelected();

    virtual Vector3 getTranslation() {return Vector3{-3.0};}
    virtual Object3D& getObj() {
        // TODO remove
        std::cerr << "Called virtual getObj" << std::endl; exit(1); return *(new Object3D{});
    };
    virtual int rightClickActions() {
        std::cerr << "Called virtual rclickActions" << std::endl; exit(1); return -1;
    }

    Figure::UnitBoardDrawable *_highlight{nullptr};

    void deleteHighlight();
};


class Stats: public Selectable {
  public:
    Stats(std::string macAddr, Vector3 pos, Figure::DeviceDrawable *dev);
    ~Stats();

    std::string create_device_string();
    void renderText();
    void updateMaps(std::string ip_src_addr, std::string mac_dst);

    Vector3 getTranslation();

    std::string makeIpLabel();

    std::string                mac_addr;
    Figure::DeviceDrawable     *_drawable;
    Object3D& getObj();
    int rightClickActions();

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
        void empty();

        unsigned int arr_len;
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

class Address: public Device::Selectable, public Object3D, public SceneGraph::Drawable3D {
  public:
    explicit Address(UnsignedByte id, Object3D& object, Shaders::Phong& shader, Color3 &color, GL::Mesh& mesh, SceneGraph::DrawableGroup3D& drawables);

    Object3D& getObj() override;
    Vector3 getTranslation() override;
    int rightClickActions() override;

    std::string value;

  private:
    void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) override;

    UnsignedByte _id;
    Color3 _color;
    Shaders::Phong& _shader;
    GL::Mesh& _mesh;
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

class DeviceDrawable: public Object3D, public SceneGraph::Drawable3D {
    public:
        explicit DeviceDrawable(UnsignedByte id, Object3D& object, Shaders::Phong& shader, Color3 &color, GL::Mesh& mesh, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& drawables);

        Device::Stats * _deviceStats;
        void resetTParam();

    private:
        void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) override;

        UnsignedByte _id;
        Color3 _color;
        Shaders::Phong& _shader;
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
        //Shaders::MeshVisualizer3D _shader = Shaders::MeshVisualizer3D{Shaders::MeshVisualizer3D::Flag::Wireframe};
        Shaders::Flat3D _shader = Shaders::Flat3D{};
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

namespace Context {
    class Store {
        public:
            Store();

            Vector2 nextVlanPos(const int vlan);
            UnsignedByte newObjectId();

            // Scene objects
            std::vector<Device::Selectable*> _selectable_objects{};
            std::set<Figure::PacketLineDrawable*> _packet_line_queue{};

            std::map<std::string, Device::Stats*> _device_map{};
            std::map<std::string, Device::PrefixStats*> _dst_prefix_group_map{};
            std::map<std::string, Device::PrefixStats*> _prefix_group_map{};

            int ring_level{0};
            int pos_in_ring{0};
    };

    class Graphic {
        public:
            Graphic();

            void prepare3DFont();
            void prepareGLBuffers(const Range2Di& viewport);

            void draw3DElements();

            Device::Stats* createSphere(Store *sCtx, const std::string mac);
            Device::PrefixStats* createBroadcastPool(const std::string, Vector3);

            void createPoolHits(Store *sCtx, Device::Stats* tran_d_s, Device::PrefixStats *dp_s, Util::L2Summary sum);
            void createPoolHit(Device::PrefixStats *dp_s, Color3 c);
            Level3::Address* createIPv4Address(Store *sCtx, const std::string ipv4_addr, Vector3 pos);

            void createLines(Store *sCtx, Vector3, Vector3, Util::L3Type, int num);
            void createLine(Store *sCtx, Vector3, Vector3, Util::L3Type);

            void addDirectLabels(Device::Stats *d_s);
            void addL2ConnectL3(Vector3 a, Vector3 b);

            void destroyGLBuffers();

            // Graphic fields
            GL::Mesh _sphere{}, _poolCircle{NoCreate}, _cubeMesh{};
            Color4 _clearColor = 0x002b36_rgbf;
            //Color4 _clearColor = 0xffffff_rgbf;
            Color3 _pickColor = 0xffffff_rgbf;

            Shaders::Phong _phong_shader;
            Figure::PhongIdShader _phong_id_shader;
            Figure::ParaLineShader _line_shader;
            Figure::PoolShader _pool_shader;
            Figure::WorldLinkShader _link_shader;
            Shaders::Flat3D _bbitem_shader;

            Scene3D _scene;
            SceneGraph::Camera3D* _camera;
            SceneGraph::DrawableGroup3D _drawables;
            SceneGraph::DrawableGroup3D _permanent_drawables;
            SceneGraph::DrawableGroup3D _selectable_drawables;
            SceneGraph::DrawableGroup3D _billboard_drawables;
            SceneGraph::DrawableGroup3D _text_drawables;

            Object3D *_cameraRig, *_cameraObject;

            GL::Framebuffer _objselect_framebuffer{NoCreate};
            GL::Renderbuffer _color, _objectId, _depth;

            // Font graphics fields
            PluginManager::Manager<Text::AbstractFont> _manager;
            Containers::Pointer<Text::AbstractFont> _font;

            Text::DistanceFieldGlyphCache _glyphCache;
            Shaders::DistanceFieldVector3D _text_shader;
    };

}

namespace Parse {

class BrokerCtx {
    public:
        // Zeek broker components
        //broker::endpoint _ep;

        //broker::subscriber subscriber;
        //broker::status_subscriber status_subscriber;
        bool peer_connected = false;

        std::chrono::duration<int64_t, std::nano> curr_pkt_lag;

        // Custom ImGui interface components
        Device::ChartMgr ifaceChartMgr{240, 3.0f};
        Device::ChartMgr ifaceLongChartMgr{300, 3.0f};

        int tot_pkt_drop{0};
        int tot_epoch_drop{0};
        int event_cnt{0};

        int inv_sample_rate{1};
        int epoch_packets_sum{0};

        BrokerCtx(std::string addr, uint16_t port);
	/*
        int parse_epoch_step(Context::Store *sCtx, Context::Graphic *gCtx, broker::zeek::Event event);
        void parse_enter_l3_addr(Context::Store *sCtx, Context::Graphic *gCtx, std::map<broker::data, broker::data> *addr_map);
        void parse_arp_table(Context::Store *sCtx, Context::Graphic *gCtx, std::map<broker::data, broker::data> *arp_table);

        void parse_stats_update(broker::zeek::Event event);
        void parse_bcast_summaries(Context::Store *sCtx, Context::Graphic *gCtx, broker::vector *dComm, Device::Stats* tran_d_s);
        void parse_single_mcast(Context::Store *sCtx, Context::Graphic *gCtx, int pos, std::string v, broker::vector *dComm, Device::Stats* tran_d_s);
	*/

        void processNetworkEvents(Context::Store *sCtx, Context::Graphic *gCtx);

        void StatsGui();
    };
}

}

#endif
