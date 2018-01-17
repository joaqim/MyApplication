#if !defined(SHADOWS_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Joaqim Planstedt $
   ======================================================================== */

#define SHADOWS_H

#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Texture.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/AbstractObject.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/Renderer.h>
#include <Corrade/Utility/Debug.h>
#include <memory>

#include "DebugLines.h"
#include "ShadowCasterShader.h"
#include "ShadowReceiverShader.h"
#include "ShadowLight.h"
#include "ShadowCasterDrawable.h"
#include "ShadowReceiverDrawable.h"
#include "Types.h"

constexpr const float MainCameraNear = 0.01f;
constexpr const float MainCameraFar = 100.0f * 2.f;

using namespace Magnum;

class Shadows {
public:
    explicit Shadows(Scene3D *scene);

    void recompileReceiverShader(std::size_t numLayers);
    void setShadowMapSize(const Vector2i& shadowMapSize);
    void setShadowSplitExponent(float power);
    void addDrawable(Object3D *object, Model &model, bool makeCaster, bool makeReceiver);
    void draw(SceneGraph::Camera3D *camera, const Vector3 transformation);

    void changeCullMode();
    void toggleStaticAlignment();
    void setShadowBias(Float value);
    void increaseShadowBias(Float value);
    void decreaseShadowBias(Float value);
    void increaseShadowRecieverBias(Float value);
    void decreaseShadowRecieverBias(Float value);

    ShadowLight* getShadowLight();

    void setShadowLightTarget(SceneGraph::Camera3D *camera, const Vector3 transformation);

private:

    SceneGraph::DrawableGroup3D _shadowCasterDrawables;
    SceneGraph::DrawableGroup3D _shadowReceiverDrawables;
    ShadowCasterShader _shadowCasterShader;
    std::unique_ptr<ShadowReceiverShader> _shadowReceiverShader;

    Object3D _shadowLightObject;
    ShadowLight _shadowLight;

    Float _shadowBias;
    Float _layerSplitExponent;
    Vector2i _shadowMapSize;
    Int _shadowMapFaceCullMode;
    bool _shadowStaticAlignment;
};

#endif
