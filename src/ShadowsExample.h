#if !defined(SHADOWSEXAMPLE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Joaqim Planstedt $
*/
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Assert.h>
#include <Corrade/Utility/Directory.h>

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/Renderer.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>

#pragma warning (push)
#pragma warning (disable : 4127)
#include <Magnum/MeshTools/Interleave.h>
#pragma warning (pop)
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Compile.h>

//#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Platform/GlfwApplication.h>

#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Capsule.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/AbstractObject.h>
#include <Magnum/SceneGraph/AbstractFeature.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Camera.h>

#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Trade/MeshObjectData3D.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/Resource.h>
using namespace Corrade;

#include "configure.h"
#include "Types.h"
#include "Shadows.h"


#include <entityplus/entity.h>
using namespace entityplus;


using namespace Magnum;

typedef ResourceManager<Buffer, Mesh, Texture2D, Shaders::Phong, Trade::PhongMaterialData> ViewerResourceManager;

using namespace Math::Literals;

class CachingObject: public Object3D, SceneGraph::AbstractFeature3D {
public:
        explicit CachingObject(Object3D* parent): Object3D{parent}, SceneGraph::AbstractFeature3D{*this} {
            setCachedTransformations(SceneGraph::CachedTransformation::Absolute);
        }

    protected:
        void clean(const Matrix4& absoluteTransformation) override {
            _absolutePosition = absoluteTransformation.translation();
            //Debug{} << "Cleaned!";
        }

    private:
        Vector3 _absolutePosition;
};

class ColoredObject: public Object3D,  SceneGraph::Drawable3D {
public:
    explicit ColoredObject(ResourceKey meshId, ResourceKey materialId, Object3D* parent, SceneGraph::DrawableGroup3D* group);

private:
    void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;
protected:
    void clean(const Matrix4& absoluteTransformation) override;
private:

    Resource<Mesh> _mesh;
    Resource<Shaders::Phong> _shader;
    Vector3 _ambientColor,
    _diffuseColor,
    _specularColor;
    Float _shininess;
    Vector3 _absolutePosition;
};

void ColoredObject::clean(const Matrix4& absoluteTransformation) {
    _absolutePosition = absoluteTransformation.translation();
    Debug{} << "Cleaned!";
}

class TexturedObject: public Object3D, SceneGraph::Drawable3D {
public:
    explicit TexturedObject(ResourceKey meshId, ResourceKey materialId, ResourceKey diffuseTextureId, Object3D* parent, SceneGraph::DrawableGroup3D* group);

private:
    void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;
protected:
    void clean(const Matrix4& absoluteTransformation) override;
private:

    Resource<Mesh> _mesh;
    Resource<Texture2D> _diffuseTexture;
    Resource<Shaders::Phong> _shader;
    Vector3 _ambientColor,
    _specularColor;
    Float _shininess;
    Vector3 _absolutePosition;
};


void TexturedObject::clean(const Matrix4& absoluteTransformation) {
    _absolutePosition = absoluteTransformation.translation();
};

using CompList = component_list<CachingObject*>;
using TagList = tag_list<struct TSuzanne, struct TCube>;

using entity_manager_t = entity_manager<CompList, TagList>;
using entity_t = entity_manager_t::entity_t;

class EntityManager : public entity_manager_t {

};

class ShadowsExample: public Platform::Application {
public:
    explicit ShadowsExample(const Arguments& arguments);

private:

    struct Keys {
        static constexpr auto Up    = KeyEvent::Key::W;
        static constexpr auto Down  = KeyEvent::Key::S;
        static constexpr auto Left  = KeyEvent::Key::A;
        static constexpr auto Right     = KeyEvent::Key::D;

        static constexpr auto Q     = KeyEvent::Key::Q;
        static constexpr auto E     = KeyEvent::Key::E;

        static constexpr auto H     = KeyEvent::Key::J;
        static constexpr auto J     = KeyEvent::Key::C;
        static constexpr auto K     = KeyEvent::Key::V;
        static constexpr auto L     = KeyEvent::Key::P;
    };

    void drawEvent() override;
    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void keyPressEvent(KeyEvent &event) override;
    void keyReleaseEvent(KeyEvent &event) override;

    void rotateCamera(Object3D* cameraObject, const Vector2 delta, float deltaZ=1.0f);
    void globalViewportEvent(const Vector2i& size);

    void addModel(const Trade::MeshData3D& meshData3D);
    void renderDebugLines();
    Object3D* createSceneObject(Model& model, bool makeCaster, bool makeReceiver);
    Object3D* createSceneObjectComp(Model& model, bool makeCaster, bool makeReceiver);
    entity_t createEnemy(CachingObject* object);

    Scene3D _scene;
    Shadows _shadows;
    
    ViewerResourceManager _resourceManager;
    SceneGraph::DrawableGroup3D _drawables;
    CachingObject* _root;
    void addObject(Trade::AbstractImporter& importer, Object3D* parent, UnsignedInt i);

    DebugLines _debugLines;

    Object3D _mainCameraObject;
    SceneGraph::Camera3D _mainCamera;
    Object3D _debugCameraObject;
    SceneGraph::Camera3D _debugCamera;

    Object3D* _activeCameraObject;
    SceneGraph::Camera3D* _activeCamera;

    std::vector<Model> _models;
    Utility::Resource _resource;

    Vector3 _mainCameraVelocity;
    Vector3 _mainCameraRotation{0.0f, 0.0f, 1.0f};
    Vector2i _previousMousePosition{0,0};

    entity_manager_t _entityManager;
};


#endif

