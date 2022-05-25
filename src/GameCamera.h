#pragma once

#include "CommonTypes.h"

// debug draw flags for game camera
enum GameCameraDebugDrawFlags: unsigned short
{
    GameCameraDebugDrawFlags_None = 0,
    GameCameraDebugDrawFlags_Map = BIT(0),
    GameCameraDebugDrawFlags_Traffic = BIT(1),
    GameCameraDebugDrawFlags_Ai = BIT(2),
    GameCameraDebugDrawFlags_Particles = BIT(3),
    GameCameraDebugDrawFlags_All = 0xFFFF, // mask
};

decl_enum_as_flags(GameCameraDebugDrawFlags);

// defines camera in 3d space
class GameCamera final
{
public:
    // public for convenience, should not be modified directly
    glm::vec3 mPosition;
    glm::vec3 mFrontDirection;
    glm::vec3 mUpDirection;
    glm::vec3 mRightDirection;
    // before reading those matrices make sure to ComputeMatricesAndFrustum
    // should not be modified directly
    cxx::frustum_t mFrustum;
    glm::mat4 mViewMatrix; 
    glm::mat4 mViewProjectionMatrix;
    glm::mat4 mProjectionMatrix;

    eSceneCameraMode mCurrentMode;

    GameCameraDebugDrawFlags mDebugDrawFlags = GameCameraDebugDrawFlags_None;

    Rect mViewportRect;
    cxx::aabbox2d_t mOnScreenMapArea; // current visible map rectangle

    // projection parameters

    struct PerspectiveParams
    {
        float mAspect = 1.0f;
        float mFovy = 1.0f;
        float mNearPlane = 1.0;
        float mFarPlane = 1.0;
    };
    PerspectiveParams mPerspectiveParams;

    struct OrthographicParams
    {
        float mLeftP = 1.0f;
        float mRightP = 1.0;
        float mBottomP = 1.0;
        float mTopP = 1.0;
    };
    OrthographicParams mOrthographicParams;

public:
    GameCamera();

    // Setup perspective projection matrix
    // @param aspect: Screen aspect ratio
    // @param fovy: Field of view in degrees
    // @param nearPlane: Near clipping plane distance
    // @param farPlane: Far clipping plane distance
    void SetPerspectiveProjection(float aspect, float fovy, float nearPlane, float farPlane);

    // Setup orthographic projection matrix
    // @param leftp, rightp, bottomp, topp
    void SetOrthographicProjection(float leftp, float rightp, float bottomp, float topp);

    // Refresh view and projection matrices along with camera frustum
    // Will not do any unnecessary calculations if nothing changed
    void ComputeMatricesAndFrustum();

    // Compute on screen view area
    void ComputeViewBounds2();

    // Reset camera to initial state, but does not clear viewport
    void SetIdentity();

    // Set camera orientation vectors
    // @param dirForward, dirRight, dirUp: Axes, should be normalized 
    void SetOrientation(const glm::vec3& dirForward, const glm::vec3& dirRight, const glm::vec3& dirUp);

    // Setup camera orientation, look at specified point
    // @param point: Point world
    // @param upward: Up vector, should be normalized
    void FocusAt(const glm::vec3& point, const glm::vec3& upward);

    // Set camera position
    // @param position: Camera new position
    void SetPosition(const glm::vec3& position);

    // Set camera rotation
    // @param rotationAngles: Angles in degrees
    void SetRotationAngles(const glm::vec3& rotationAngles);

    // Move camera position
    // @param direction: Move direction
    void Translate(const glm::vec3& direction);

    // Cast ray in specific viewport coordinate, make sure to ComputeMatricesAndFrustum
    // @param screenCoordinate: 2d coordinates on screen
    // @param resultRay: Output ray info
    bool CastRayFromScreenPoint(const glm::ivec2& screenCoordinate, cxx::ray3d_t& resultRay);

    // Transform world space point to screen space point, make sure to ComputeMatricesAndFrustum
    bool ProjectPointToScreen(const glm::vec3& point, glm::vec2& resultScreenPoint);

    // Will swap Z and Y direction vectors
    void SetTopDownOrientation();

    bool CheckDebugDrawFlags(GameCameraDebugDrawFlags flags) const;
    bool CheckDebugDrawFlagsAll(GameCameraDebugDrawFlags flags) const;

private:
    bool mProjMatrixDirty; // projection matrix need recomputation
    bool mViewMatrixDirty; // view matrix need recomputation
};

// defines ui camera
class GameCamera2D final
{
public:
    // public for convenience, should not be modified directly
    glm::mat4 mProjectionMatrix;
    Rect mViewportRect;

public:
    GameCamera2D();

    // setup orthographic projection matrix
    // @param leftp, rightp, bottomp, topp
    void SetProjection(float leftp, float rightp, float bottomp, float topp);

    // Reset camera to initial state, but does not clear viewport
    void SetIdentity();
};