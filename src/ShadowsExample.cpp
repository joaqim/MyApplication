#include "ShadowsExample.h"

void ShadowsExample::globalViewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    _activeCamera->setViewport(size);
}

ShadowsExample::ShadowsExample(const Arguments& arguments):
        Platform::Application{arguments, 
        Configuration{}.setTitle("Magnum Shadows Example")
                            .setWindowFlags(Configuration::WindowFlag::Resizable)
                            .setCursorMode(Configuration::CursorMode::Disabled)
                            .setSampleCount(8)},
_mainCameraObject{&_scene},
_mainCamera{_mainCameraObject},
_debugCameraObject{&_scene},
_debugCamera{_debugCameraObject},
_resource{"shadow-data"},
_shadows{&_scene}
{
    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "file to load")
        .setHelp("Loads and displays 3D scene file (such as OpenGEX or "
                 "COLLADA one) provided on command-line.")
        .parse(arguments.argc, arguments.argv);
    // Corrade Resources
    //Debug{} << _resource.get("ShadowCaster.vert");
    
    // Viewer :
    /* - Phong Shader instances */
    _resourceManager
        .set("color", new Shaders::Phong)
        .set("texture", new Shaders::Phong{Shaders::Phong::Flag::DiffuseTexture});


    /* Fallback material, texture and mesh in case the data are not present or
       cannot be loaded */
    auto material = new Trade::PhongMaterialData{{}, 50.0f};
    material->ambientColor() = 0x000000_rgbf;
    material->diffuseColor() = 0xe5e5e5_rgbf;
    material->specularColor() = 0xffffff_rgbf;
    _resourceManager
        .setFallback(material)
        .setFallback(new Texture2D)
        .setFallback(new Mesh);

    Debug{} << MAGNUM_PLUGINS_IMPORTER_DIR;
    /* Load scene importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager{MAGNUM_PLUGINS_IMPORTER_DIR};
    std::unique_ptr<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnySceneImporter");
    if(!importer) std::exit(1);

    //std::string fileName = "scene2.blend";
    std::string fileName = "scene2.ogex";
    //auto fileName = args.value("file");
    Debug{} << "Opening file" << fileName;

    /* Load file */
    if(!importer->openFile(fileName)){
        Error{} << "Failed to open file" << fileName;
        std::exit(4);
    }
    Debug{} << "Opened file" << fileName;

    /* Load all materials */
    for(UnsignedInt i = 0; i != importer->materialCount(); ++i) {
        Debug{} << "Importing material" << i << importer->materialName(i);

        std::unique_ptr<Trade::AbstractMaterialData> materialData = importer->material(i);
        if(!materialData || materialData->type() != Trade::MaterialType::Phong) {
            Warning{} << "Cannot load material, skipping";
            continue;
        }

        /* Save the material */
        _resourceManager.set(ResourceKey{i}, static_cast<Trade::PhongMaterialData*>(materialData.release()));
    }

    /* Load all textures */
    for(UnsignedInt i = 0; i != importer->textureCount(); ++i) {
        Debug{} << "Importing texture" << i << importer->textureName(i);

        std::optional<Trade::TextureData> textureData = importer->texture(i);
        if(!textureData || textureData->type() != Trade::TextureData::Type::Texture2D) {
            Warning{} << "Cannot load texture, skipping";
            continue;
        }

        Debug{} << "Importing image" << textureData->image() << importer->image2DName(textureData->image());
    
        const std::string imageName = "textures/viewer.png";
        Debug{} << Utility::Directory::fileExists(imageName);
        //importer->openFile(imageName);

        std::optional<Trade::ImageData2D> imageData = importer->image2D(textureData->image());

        if(!imageData || (imageData->format() != PixelFormat::RGB
#ifndef MAGNUM_TARGET_GLES
                          && imageData->format() != PixelFormat::BGR
#endif
                          ))
        {
            Warning{} << "Cannot load texture image, skipping";
            continue;
        }

        /* Configure texture */
        auto texture = new Texture2D;
        texture->setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy())
            .setStorage(1, TextureFormat::RGB8, imageData->size())
            .setSubImage(0, {}, *imageData)
            .generateMipmap();

        /* Save it */
        _resourceManager.set(ResourceKey{i}, texture, ResourceDataState::Final, ResourcePolicy::Manual);
    }

    /* Load all meshes */
    for(UnsignedInt i = 0; i != importer->mesh3DCount(); ++i) {
        Debug{} << "Importing mesh" << i << importer->mesh3DName(i);

        std::optional<Trade::MeshData3D> meshData = importer->mesh3D(i);
        if(!meshData || !meshData->hasNormals() || meshData->primitive() != MeshPrimitive::Triangles) {
            Warning{} << "Cannot load mesh, skipping";
            continue;
        }

        /* Compile the mesh */
        Mesh mesh{NoCreate};
        std::unique_ptr<Buffer> buffer, indexBuffer;
        std::tie(mesh, buffer, indexBuffer) = MeshTools::compile(*meshData, BufferUsage::StaticDraw);

        /* Save things */
        _resourceManager.set(ResourceKey{i}, new Mesh{std::move(mesh)}, ResourceDataState::Final, ResourcePolicy::Manual);
        _resourceManager.set(std::to_string(i) + "-vertices", buffer.release(), ResourceDataState::Final, ResourcePolicy::Manual);
        if(indexBuffer)
            _resourceManager.set(std::to_string(i) + "-indices", indexBuffer.release(), ResourceDataState::Final, ResourcePolicy::Manual);
    }


    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    addModel(Primitives::Cube::solid());
    addModel(Primitives::Capsule3D::solid(1, 1, 4, 1.0f));
    addModel(Primitives::Capsule3D::solid(6, 1, 9, 1.0f));



    Object3D* ground = createSceneObject(_models[0], false, true);
    ground->setTransformation(Matrix4::scaling({100,1,100}));

    for(std::size_t i = 0; i != 200; ++i) {
        Model& model = _models[std::rand()%_models.size()];
        Object3D* object = createSceneObject(model, true, true);
        object->setTransformation(Matrix4::translation({
                    std::rand()*100.0f/RAND_MAX - 50.0f,
                        std::rand()*5.0f/RAND_MAX,
                        std::rand()*100.0f/RAND_MAX - 50.0f}));
        //Matrix4::scaling({1, 1, 1}));
        //object->setTransformation(Matrix4::scaling({2, 1, 2}));
/*  
    object->setTransformation(Matrix4::scaling({
    std::rand()*2.0f/RAND_MAX - 1.0f,
    std::rand()*2.0f/RAND_MAX - 1.0f,
    std::rand()*2.0f/RAND_MAX - 1.0f}));
*/
    }

    /* Default object, parent of all (for manipulation) */
    //_root = new Object3D{&_scene};
    _root = new CachingObject{&_scene};

    //_root = createSceneObject(_models[0], true, true);
    //_root->setTransformation(Matrix4::scaling({2,2,2}) + Matrix4::translation({0,10,0})); 

    auto e = _entityManager.create_entity<TCube>(_root);
    auto comp = e.get_component<CachingObject*>();
    comp->setTransformation(Matrix4::scaling({2,2,2}) + Matrix4::translation({0,10,0}));
    /* Load the scene */
    if(importer->defaultScene() != -1) {
        Debug{} << "Adding default scene" << importer->sceneName(importer->defaultScene());

        std::optional<Trade::SceneData> sceneData = importer->scene(importer->defaultScene());
        if(!sceneData) {
            Error{} << "Cannot load scene, exiting";
            return;
        }

        /* Recursively add all children */
        for(UnsignedInt objectId: sceneData->children3D())
            addObject(*importer, _root, objectId);

        /* The format has no scene support, display just the first loaded mesh with
           default material and be done with it */
    } else if(_resourceManager.state<Mesh>(ResourceKey{0}) == ResourceState::Final)
        new ColoredObject{ResourceKey{(size_t)0}, ResourceKey((size_t)-1), _root, &_drawables};

    /* Materials were consumed by objects and they are not needed anymore. Also
       free all texture/mesh data that weren't referenced by any object. */
    _resourceManager.setFallback<Trade::PhongMaterialData>(nullptr)
        .clear<Trade::PhongMaterialData>()
        .free<Texture2D>()
        .free<Mesh>();


    _mainCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf,
                                                                   Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(),
                                                                   MainCameraNear, MainCameraFar));
    _mainCameraObject.setTransformation(Matrix4::translation(Vector3::yAxis(3.0f)));

    _debugCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf,
                                                                    Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(),
                                                                    MainCameraNear/4.0f, MainCameraFar*4.0f));
    _debugCameraObject.setTransformation(Matrix4::lookAt(
                                             {100.0f, 50.0f, 0.0f}, Vector3::zAxis(-30.0f), Vector3::yAxis()));

    _activeCamera = &_mainCamera;
    _activeCameraObject = &_mainCameraObject;

    _shadows.setShadowLightTarget(_activeCamera, _activeCameraObject->transformation()[2].xyz());
}

void ShadowsExample::addObject(Trade::AbstractImporter& importer, Object3D* parent, UnsignedInt i) {
    Debug{} << "Importing object" << i << importer.object3DName(i);

    Object3D* object = nullptr;
    std::unique_ptr<Trade::ObjectData3D> objectData = importer.object3D(i);
    if(!objectData) {
        Error{} << "Cannot import object, skipping";
        return;
    }

    /* Only meshes for now */
    if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh) {
        Int materialId = static_cast<Trade::MeshObjectData3D*>(objectData.get())->material();

        /* Decide what object to add based on material type */
        auto materialData = _resourceManager.get<Trade::PhongMaterialData>(ResourceKey(materialId));

        /* Color-only material */
        if(!materialData->flags()) {
            object = new ColoredObject(ResourceKey(objectData->instance()),
                                       ResourceKey(materialId),
                                       parent, &_drawables);

            object->setTransformation(objectData->transformation());


            /* Diffuse texture material */
        } else if(materialData->flags() == Trade::PhongMaterialData::Flag::DiffuseTexture) {
            object = new TexturedObject(ResourceKey(objectData->instance()),
                                        ResourceKey(materialId),
                                        ResourceKey(materialData->diffuseTexture()),
                                        parent, &_drawables);
            object->setTransformation(objectData->transformation());

            /* No other material types are supported yet */
        } else {
            Warning() << "Texture combination of material"
                      << materialId << importer.materialName(materialId)
                      << "is not supported, using default material instead";

            object = new ColoredObject(ResourceKey(objectData->instance()),
                                       ResourceKey((size_t)-1),
                                       parent, &_drawables);
            object->setTransformation(objectData->transformation());
        }
    }

    /* Create parent object for children, if it doesn't already exist */
    if(!object && !objectData->children().empty()) object = new Object3D(parent);

    /* Recursively add children */
    for(std::size_t id: objectData->children()) {
        addObject(importer, object, id);
    }
}

ColoredObject::ColoredObject(ResourceKey meshId, ResourceKey materialId, Object3D* parent, SceneGraph::DrawableGroup3D* group):
        Object3D{parent}, SceneGraph::Drawable3D{*this, group},
_mesh{ViewerResourceManager::instance().get<Mesh>(meshId)}, _shader{ViewerResourceManager::instance().get<Shaders::Phong>("color")}
        {
            auto material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialId);
            _ambientColor = material->ambientColor();
            _diffuseColor = material->diffuseColor();
            _specularColor = material->specularColor();
            _shininess = material->shininess();
        }

TexturedObject::TexturedObject(ResourceKey meshId, ResourceKey materialId, ResourceKey diffuseTextureId, Object3D* parent, SceneGraph::DrawableGroup3D* group):
        Object3D{parent}, SceneGraph::Drawable3D{*this, group},
                                _mesh{ViewerResourceManager::instance().get<Mesh>(meshId)}, _diffuseTexture{ViewerResourceManager::instance().get<Texture2D>(diffuseTextureId)}, _shader{ViewerResourceManager::instance().get<Shaders::Phong>("texture")}
        {
            auto material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialId);
            _ambientColor = material->ambientColor();
            _specularColor = material->specularColor();
            _shininess = material->shininess();
        }

void ColoredObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader->setAmbientColor(_ambientColor)
        .setDiffuseColor(_diffuseColor)
        .setSpecularColor(_specularColor)
        .setShininess(_shininess)
        .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix());

    _mesh->draw(*_shader);
}

void TexturedObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader->setAmbientColor(_ambientColor)
        .setDiffuseTexture(*_diffuseTexture)
        .setSpecularColor(_specularColor)
        .setShininess(_shininess)
        .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix());

    _mesh->draw(*_shader);
}


Object3D* ShadowsExample::createSceneObject(Model& model, bool makeCaster, bool makeReceiver) {
    auto* object = new CachingObject(&_scene);
    _shadows.addDrawable(object, model, makeCaster, makeReceiver);
    return object;
}

Object3D* ShadowsExample::createSceneObjectComp(Model& model, bool makeCaster, bool makeReceiver) {
    auto* object = new CachingObject(&_scene);
    _shadows.addDrawable(object, model, makeCaster, makeReceiver);
    _entityManager.create_entity(object);
    return object;
}

entity_t ShadowsExample::createEnemy(CachingObject* object) {
    return _entityManager.create_entity(object);
}

void ShadowsExample::addModel(const Trade::MeshData3D& meshData3D) {
    _models.emplace_back();
    Model& model = _models.back();

    model.vertexBuffer.setData(MeshTools::interleave(meshData3D.positions(0), meshData3D.normals(0)),
                               BufferUsage::StaticDraw);

    Float maxMagnitudeSquared = 0.0f;
    for(Vector3 position: meshData3D.positions(0)) {
        Float magnitudeSquared = position.dot();

        if(magnitudeSquared > maxMagnitudeSquared) {
            maxMagnitudeSquared = magnitudeSquared;
        }
    }
    model.radius = std::sqrt(maxMagnitudeSquared);

    Containers::Array<char> indexData;
    Mesh::IndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(meshData3D.indices());
    model.indexBuffer.setData(indexData, BufferUsage::StaticDraw);

    model.mesh.setPrimitive(meshData3D.primitive())
        .setCount(meshData3D.indices().size())
        .addVertexBuffer(model.vertexBuffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
        .setIndexBuffer(model.indexBuffer, 0, indexType, indexStart, indexEnd);
}

void ShadowsExample::drawEvent() {
    if(!_mainCameraVelocity.isZero()) {
        Matrix4 transform = _activeCameraObject->transformation();
        transform.translation() += transform.rotation()*_mainCameraVelocity*0.3f;

        _entityManager.for_each<CachingObject*>([transform] (auto ent_, auto &object) {
                Matrix4 objtrans = object->transformation();
                object->setTransformation(Matrix4::lookAt(objtrans.translation(), transform.translation(), Vector3::zAxis()));
            });

        _activeCameraObject->setTransformation(transform);
        _shadows.setShadowLightTarget(_activeCamera, transform[2].xyz());
        redraw();
    }

    if(!_mainCameraRotation.isZero()) {
        rotateCamera(_activeCameraObject, Vector2{_mainCameraRotation.x(), _mainCameraRotation.y()}, _mainCameraRotation.z());
        _shadows.setShadowLightTarget(_activeCamera, _activeCameraObject->transformation()[2].xyz());
        redraw();
    }

_entityManager.for_each<CachingObject*>([] (auto ent_, auto &object) {
                Matrix4 objtrans = object->transformation();
                objtrans.translation() += objtrans.rotation()*Vector3::zAxis(-0.2f)*0.3f;
                object->setTransformation(objtrans);
                object->setClean();
    });

    Renderer::setClearColor({0.1f, 0.1f, 0.4f, 1.0f});
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    _shadows.draw(_activeCamera, _activeCameraObject->transformation()[2].xyz()); 
    _activeCamera->draw(_drawables);

    renderDebugLines();

    swapBuffers();
}


void ShadowsExample::renderDebugLines() {
    if(_activeCamera != &_debugCamera)
        return;
#if 1
    constexpr const Matrix4 unbiasMatrix{{ 2.0f,  0.0f,  0.0f, 0.0f},
        { 0.0f,  2.0f,  0.0f, 0.0f},
        { 0.0f,  0.0f,  2.0f, 0.0f},
        {-1.0f, -1.0f, -1.0f, 1.0f}};

    auto shadowLight = _shadows.getShadowLight();
    _debugLines.reset();
    const Matrix4 imvp = (_mainCamera.projectionMatrix()*_mainCamera.cameraMatrix()).inverted();
    for(std::size_t layerIndex = 0; layerIndex != shadowLight->layerCount(); ++layerIndex) {
        const Matrix4 layerMatrix = shadowLight->layerMatrix(layerIndex);
        const Deg hue = layerIndex*360.0_degf/shadowLight->layerCount();
        _debugLines.addFrustum((unbiasMatrix*layerMatrix).inverted(),
                               Color3::fromHsv(hue, 1.0f, 0.5f));
        _debugLines.addFrustum(imvp,
                               Color3::fromHsv(hue, 1.0f, 1.0f),
                               layerIndex == 0 ? 0 : shadowLight->cutZ(layerIndex - 1), shadowLight->cutZ(layerIndex));
    }

    _debugLines.draw(_activeCamera->projectionMatrix()*_activeCamera->cameraMatrix());
#endif
}

void ShadowsExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    event.setAccepted();
}

void ShadowsExample::mouseReleaseEvent(MouseEvent& event) {

    event.setAccepted();
    redraw();
}

void ShadowsExample::rotateCamera(Object3D* cameraObject, const Vector2 delta, const float deltaZ) {
    constexpr const Float angleScale = 1.0f;
    const Float angleX = delta.x()*angleScale;
    const Float angleY = delta.y()*angleScale;
    const Float angleZ = deltaZ*angleScale;
    const Matrix4 transform = cameraObject->transformation();
    if(angleX != 0.0f || angleY != 0.0f) {
        cameraObject->setTransformation(Matrix4::lookAt(transform.translation(),
                                                        transform.translation() - transform.rotationScaling()*Vector3{-angleX, angleY, angleZ},
                                                        Vector3::yAxis()));
    }
}

void ShadowsExample::mouseMoveEvent(MouseMoveEvent& event) {
    //if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;
    
    
    const Vector2 delta = 
        Vector2{event.position() - _previousMousePosition}/
        Vector2{defaultFramebuffer.viewport().size()};

        /*
          const Matrix4 transform = _activeCameraObject->transformation();

          constexpr const Float angleScale = 1.0f;
          const Float angleX = delta.x()*angleScale;
          const Float angleY = delta.y()*angleScale;
          if(angleX != 0.0f || angleY != 0.0f) {
          _activeCameraObject->setTransformation(Matrix4::lookAt(transform.translation(),
          transform.translation() - transform.rotationScaling()*Vector3{-angleX, angleY, 1.0f},
          Vector3::yAxis()));
          }
        */
        rotateCamera(_activeCameraObject, delta);

        _previousMousePosition = event.position();
        event.setAccepted();
        redraw();
}

#define CAMERA_VELOCITY 0.5f
#define CAMERA_ROTATION 0.01f
#define UNREFERENCED_PARAMETER(x) x
#define UNUSED_ENTITY( x ) ( &reinterpret_cast< const entity_t& >( x ) )

void ShadowsExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == Keys::Up) {
        _mainCameraVelocity.z() = -CAMERA_VELOCITY;

    } else if(event.key() == Keys::Down) {
        _mainCameraVelocity.z() = CAMERA_VELOCITY;
    } else if(event.key() == KeyEvent::Key::Space) {
        _entityManager.for_each<CachingObject*>([this]( auto ent, auto &object) {
                //UNUSED_ENTITY(ent);
                UNREFERENCED_PARAMETER(ent);
                Matrix4 transform = object->transformation();
        transform.translation() += Vector3::yAxis(3.0f);
        object->setTransformation(transform);
            });
    } else if(event.key() == Keys::Q) {
        _mainCameraVelocity.y() = -CAMERA_VELOCITY;

    } else if(event.key() == Keys::E) {
        _mainCameraVelocity.y() = CAMERA_VELOCITY;

    } else if(event.key() == Keys::Right) {
        _mainCameraVelocity.x() = CAMERA_VELOCITY;

    } else if(event.key() == Keys::Left) {
        _mainCameraVelocity.x() = -CAMERA_VELOCITY;
    } else if(event.key() == Keys::H) {
        _mainCameraRotation.x() = -CAMERA_ROTATION;
    } else if(event.key() == Keys::L) {
        _mainCameraRotation.x() = CAMERA_ROTATION;
    } else if(event.key() == Keys::J) {
        _mainCameraRotation.y() = CAMERA_ROTATION;
    } else if(event.key() == Keys::K) {
        _mainCameraRotation.y() = -CAMERA_ROTATION;
    } else if(event.key() == KeyEvent::Key::Esc) {
        delete _root;
        _resourceManager.clear();
        exit();
    } else if(event.key() == KeyEvent::Key::F1) {
        _activeCamera = &_mainCamera;
        _activeCameraObject = &_mainCameraObject;

    } else if(event.key() == KeyEvent::Key::F2) {
        _activeCamera = &_debugCamera;
        _activeCameraObject = &_debugCameraObject;
    } else if(event.key() == KeyEvent::Key::F3) {
        _shadows.changeCullMode();
    } else if(event.key() == KeyEvent::Key::F4) {
        _shadows.toggleStaticAlignment();
    } else if(event.key() == KeyEvent::Key::F5) {
        _shadows.increaseShadowBias(1.125f); 
    } else if(event.key() == KeyEvent::Key::F6) {
        _shadows.decreaseShadowBias(1.125f); 
    } else if(event.key() == KeyEvent::Key::F7) {
        _shadows.increaseShadowRecieverBias(1.125f);
    } else if(event.key() == KeyEvent::Key::F8) {
        _shadows.decreaseShadowRecieverBias(1.125f);
#if 0

    } else if(event.key() == KeyEvent::Key::F9) {
        std::size_t numLayers = _shadowLight.layerCount() - 1;
        if(numLayers >= 1) {
            _shadowLight.setupShadowmaps(numLayers, _shadowMapSize);
            recompileReceiverShader(numLayers);
            _shadowLight.setupSplitDistances(MainCameraNear, MainCameraFar, _layerSplitExponent);
            Debug() << "Shadow map size" << _shadowMapSize << "x" << _shadowLight.layerCount() << "layers";
        } else return;

    } else if(event.key() == KeyEvent::Key::F10) {
        std::size_t numLayers = _shadowLight.layerCount() + 1;
        if(numLayers <= 32) {
            _shadowLight.setupShadowmaps(numLayers, _shadowMapSize);
            recompileReceiverShader(numLayers);
            _shadowLight.setupSplitDistances(MainCameraNear, MainCameraFar, _layerSplitExponent);
            Debug() << "Shadow map size" << _shadowMapSize << "x" << _shadowLight.layerCount() << "layers";
        } else return;

    } else if(event.key() == KeyEvent::Key::F11) {
        setShadowMapSize(_shadowMapSize/2);

    } else if(event.key() == KeyEvent::Key::F12) {
        setShadowMapSize(_shadowMapSize*2);
#endif
    } else return;

    event.setAccepted();
    redraw();
}


void ShadowsExample::keyReleaseEvent(KeyEvent &event) {
    if(event.key() == Keys::Up || event.key() == Keys::Down) {
        _mainCameraVelocity.z() = 0.0f;

    } else if (event.key() == Keys::Q || event.key() == Keys::E) {
        _mainCameraVelocity.y() = 0.0f;

    } else if (event.key() == Keys::Right || event.key() == Keys::Left) {
        _mainCameraVelocity.x() = 0.0f;
    } else if (event.key() == Keys::H || event.key() == Keys::L) {
        _mainCameraRotation.x() = 0.0f;
    } else if (event.key() == Keys::J || event.key() == Keys::K) {
        _mainCameraRotation.y() = 0.0f;

    } else return;

    event.setAccepted();
    redraw();
}


MAGNUM_APPLICATION_MAIN(ShadowsExample)
