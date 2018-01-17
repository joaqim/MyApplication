/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Joaqim $
   ======================================================================== */

#include "Shadows.h"


Shadows::Shadows(Scene3D *scene):
_shadowLightObject{scene},
_shadowLight{_shadowLightObject},
_shadowBias{0.003f},
_layerSplitExponent{3.0f},
_shadowMapSize{1024*2, 1024*2},
_shadowMapFaceCullMode{1},
_shadowStaticAlignment{false}
{

    _shadowLight.setupShadowmaps(3, _shadowMapSize);
    _shadowReceiverShader.reset(new ShadowReceiverShader(_shadowLight.layerCount()));
    _shadowReceiverShader->setShadowBias(_shadowBias);

    _shadowLight.setupSplitDistances(MainCameraNear, MainCameraFar, _layerSplitExponent);

    _shadowLightObject.setTransformation(Matrix4::lookAt(
                                             {3.0f, 1.0f, 2.0f}, {}, Vector3::yAxis()));
};

void Shadows::setShadowSplitExponent(const Float power) {
    _shadowLight.setupSplitDistances(MainCameraNear, MainCameraFar, power);
    std::string buf;
    for(std::size_t layer = 0; layer != _shadowLight.layerCount(); ++layer) {
        if(layer) buf += ", ";
        buf += std::to_string(_shadowLight.cutDistance(MainCameraNear, MainCameraFar, layer));
    }

    Debug() << "Shadow splits power=" << power << "cut points:" << buf;
}

void Shadows::setShadowMapSize(const Vector2i& shadowMapSize) {
    if((shadowMapSize >= Vector2i{1}).all() && (shadowMapSize <= Texture2D::maxSize()).all()) {
        _shadowMapSize = shadowMapSize;
        _shadowLight.setupShadowmaps(_shadowLight.layerCount(), _shadowMapSize);
        Debug() << "Shadow map size" << shadowMapSize << "x" << _shadowLight.layerCount() << "layers";
    }
}

void Shadows::recompileReceiverShader(const std::size_t numLayers) {
    _shadowReceiverShader.reset(new ShadowReceiverShader(numLayers));
    _shadowReceiverShader->setShadowBias(_shadowBias);
    for(std::size_t i = 0; i != _shadowReceiverDrawables.size(); ++i) {
        auto& drawable = static_cast<ShadowReceiverDrawable&>(_shadowReceiverDrawables[i]);
        drawable.setShader(*_shadowReceiverShader);
    }
}

void Shadows::addDrawable(Object3D *object, Model &model, bool makeCaster, bool makeReceiver) {

if(makeCaster) {
        auto caster = new ShadowCasterDrawable(*object, &_shadowCasterDrawables);
        caster->setShader(_shadowCasterShader);
        caster->setMesh(model.mesh, model.radius);
    }

    if(makeReceiver) {
        auto receiver = new ShadowReceiverDrawable(*object, &_shadowReceiverDrawables);
        receiver->setShader(*_shadowReceiverShader);
        receiver->setMesh(model.mesh);
    }
}

void Shadows::setShadowLightTarget(SceneGraph::Camera3D *camera, const Vector3 transformation) {
    const Vector3 screenDirection = _shadowStaticAlignment ? Vector3::zAxis() : transformation;
    /* You only really need to do this when your camera moves */
    _shadowLight.setTarget({3, 2, 3}, screenDirection, *camera);
};

ShadowLight* Shadows::getShadowLight() {
    return &_shadowLight;
};

void Shadows::draw(SceneGraph::Camera3D *camera, const Vector3 transformation) {

    /* You can use face culling, depending on your geometry. You might want to
       render only back faces for shadows. */
    switch(_shadowMapFaceCullMode) {
        case 0:
            Renderer::disable(Renderer::Feature::FaceCulling);
            break;
        case 2:
            Renderer::setFaceCullingMode(Renderer::PolygonFacing::Front);
            break;
    }

    /* Create the shadow map textures. */
    _shadowLight.render(_shadowCasterDrawables);

    switch(_shadowMapFaceCullMode) {
        case 0:
            Renderer::enable(Renderer::Feature::FaceCulling);
            break;
        case 2:
            Renderer::setFaceCullingMode(Renderer::PolygonFacing::Back);
            break;
    }
    Containers::Array<Matrix4> shadowMatrices{Containers::NoInit, _shadowLight.layerCount()};
    for(std::size_t layerIndex = 0; layerIndex != _shadowLight.layerCount(); ++layerIndex)
        shadowMatrices[layerIndex] = _shadowLight.layerMatrix(layerIndex);

    _shadowReceiverShader->setShadowmapMatrices(shadowMatrices)
        .setShadowmapTexture(_shadowLight.shadowTexture())
        .setLightDirection(_shadowLightObject.transformation().backward());

    camera->draw(_shadowReceiverDrawables);

}

void Shadows::changeCullMode(){
        _shadowMapFaceCullMode = (_shadowMapFaceCullMode + 1) % 3;
        Debug() << "Face cull mode:"
                << (_shadowMapFaceCullMode == 0 ? "no cull" : _shadowMapFaceCullMode == 1 ? "cull back" : "cull front");
}

void Shadows::toggleStaticAlignment(){
    _shadowStaticAlignment = !_shadowStaticAlignment;
    Debug() << "Shadow alignment:"
            << (_shadowStaticAlignment ? "static" : "camera direction");

}

void Shadows::increaseShadowBias(Float value) {
    setShadowSplitExponent(_layerSplitExponent *= value); 
}
void Shadows::decreaseShadowBias(Float value) {
        setShadowSplitExponent(_layerSplitExponent /= value);
}
void Shadows::increaseShadowRecieverBias(Float value) {
        _shadowReceiverShader->setShadowBias(_shadowBias *= value);
        Debug() << "Shadow bias" << _shadowBias;
}
void Shadows::decreaseShadowRecieverBias(Float value) {
        _shadowReceiverShader->setShadowBias(_shadowBias /= value);
        Debug() << "Shadow bias" << _shadowBias;
}
