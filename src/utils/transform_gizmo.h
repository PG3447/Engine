#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include "core/gameobject.h"
#include "core/component.h"
#include "utils/camera_helper.h"
#include "utils/transform_helper.h"
#include "systems/DebugDrawSystem.h"

namespace TransformGizmo
{
    enum class Handle
    {
        None,
        AxisX, AxisY, AxisZ,
        PlaneXY, PlaneXZ, PlaneYZ
    };

    struct Ray
    {
        glm::vec3 origin;
        glm::vec3 dir; // znormalizowany
    };

    struct State
    {
        Handle hoveredHandle  = Handle::None;
        Handle draggedHandle  = Handle::None;

        glm::vec3 dragStartHitPoint = glm::vec3(0.0f);
        glm::vec3 dragStartObjectPos = glm::vec3(0.0f);

        bool leftMouseWasDown = false;

        float screenSizePixelsTarget = 200.0f;


        float axisPickRadiusFactor = 0.1f;   // * gizmoScale
        float planeHandleSizeFactor = 0.28f;
        float planeHandleOffsetFactor = 0.35f;// * gizmoScale
    };

    inline State& Get()
    {
        static State state;
        return state;
    }

    inline bool ScreenToWorldRay(CameraComponent& cam, TransformComponent& camTransform,
        double mouseX, double mouseY, int windowW, int windowH, Ray& outRay)
    {
        float vpX = cam.viewport.x * windowW;
        float vpY = cam.viewport.y * windowH;
        float vpW = cam.viewport.width * windowW;
        float vpH = cam.viewport.height * windowH;

        if (mouseX < vpX || mouseX > vpX + vpW || mouseY < vpY || mouseY > vpY + vpH)
            return false;

        float localX = (float)(mouseX - vpX);
        float localY = (float)(mouseY - vpY);

        float ndcX = (2.0f * localX) / vpW - 1.0f;
        float ndcY = 1.0f - (2.0f * localY) / vpH;

        glm::mat4 proj = CameraHelper::getProjectionMatrix(cam, windowW, windowH);
        glm::mat4 view = CameraHelper::getViewMatrix(cam, camTransform);
        glm::mat4 invVP = glm::inverse(proj * view);

        glm::vec4 nearPoint = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farPoint  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
        nearPoint /= nearPoint.w;
        farPoint  /= farPoint.w;

        outRay.origin = glm::vec3(nearPoint);
        outRay.dir    = glm::normalize(glm::vec3(farPoint - nearPoint));
        return true;
    }

    inline float ClosestTOnRay(const Ray& ray, const glm::vec3& point)
    {
        return glm::dot(point - ray.origin, ray.dir);
    }

    inline float DistanceRayToSegment(const Ray& ray, const glm::vec3& segA, const glm::vec3& segB,
        float& outTRay, float& outTSeg)
    {
        glm::vec3 d1 = ray.dir;
        glm::vec3 d2 = segB - segA;
        glm::vec3 r  = ray.origin - segA;

        float a = glm::dot(d1, d1);
        float e = glm::dot(d2, d2);
        float f = glm::dot(d2, r);
        float c = glm::dot(d1, r);
        float b = glm::dot(d1, d2);

        float denom = a * e - b * b;

        float tRay, tSeg;
        if (glm::abs(denom) > 1e-6f)
        {
            tRay = (b * f - c * e) / denom;
            tSeg = (a * f - b * c) / denom;
        }
        else
        {
            tRay = 0.0f;
            tSeg = f / e;
        }

        tSeg = glm::clamp(tSeg, 0.0f, 1.0f);
        tRay = glm::max(tRay, 0.0f);

        glm::vec3 closestOnRay = ray.origin + ray.dir * tRay;
        glm::vec3 closestOnSeg = segA + d2 * tSeg;

        outTRay = tRay;
        outTSeg = tSeg;
        return glm::length(closestOnRay - closestOnSeg);
    }

    inline bool RayVsPlane(const Ray& ray, const glm::vec3& planeOrigin, const glm::vec3& planeNormal,
        glm::vec3& outHitPoint)
    {
        float denom = glm::dot(ray.dir, planeNormal);
        if (glm::abs(denom) < 1e-6f) return false;

        float t = glm::dot(planeOrigin - ray.origin, planeNormal) / denom;
        if (t < 0.0f) return false;

        outHitPoint = ray.origin + ray.dir * t;
        return true;
    }

    inline float ComputeGizmoScale(CameraComponent& cam, const glm::vec3& cameraPos,
        const glm::vec3& gizmoPos, int windowHeightPixelsOfViewport)
    {
        float dist = glm::length(gizmoPos - cameraPos);
        float fovRad = glm::radians(cam.fov);
        float worldHeightAtDist = 2.0f * dist * glm::tan(fovRad * 0.5f);
        float worldPerPixel = worldHeightAtDist / glm::max(1, windowHeightPixelsOfViewport);
        return worldPerPixel * Get().screenSizePixelsTarget;
    }

    inline void DrawGizmo(const glm::vec3& origin, float scale, Handle hovered, Handle dragged)
    {
        glm::vec4 colX = glm::vec4(0.15f, 0.85f, 0.85f, 1.0f);
        glm::vec4 colY = glm::vec4(0.15f, 0.85f, 0.15f, 1.0f);
        glm::vec4 colZ = glm::vec4(0.15f, 0.45f, 0.95f, 1.0f);
        glm::vec4 colHighlight = glm::vec4(0.85f, 1.0f, 1.0f, 1.0f);

        Handle active = (dragged != Handle::None) ? dragged : hovered;

        glm::vec4 finalX = (active == Handle::AxisX) ? colHighlight : colX;
        glm::vec4 finalY = (active == Handle::AxisY) ? colHighlight : colY;
        glm::vec4 finalZ = (active == Handle::AxisZ) ? colHighlight : colZ;

        DebugDrawSystem::AddLine(origin, origin + glm::vec3(scale, 0, 0), finalX);
        DebugDrawSystem::AddLine(origin, origin + glm::vec3(0, scale, 0), finalY);
        DebugDrawSystem::AddLine(origin, origin + glm::vec3(0, 0, scale), finalZ);

        float off  = scale * Get().planeHandleOffsetFactor;
        float half = scale * Get().planeHandleSizeFactor * 0.5f;

        auto drawPlaneQuad = [&](const glm::vec3& a1, const glm::vec3& a2, Handle h, glm::vec4 baseColor)
        {
            glm::vec3 center = origin + a1 * off + a2 * off;
            glm::vec3 p0 = center - a1 * half - a2 * half;
            glm::vec3 p1 = center + a1 * half - a2 * half;
            glm::vec3 p2 = center + a1 * half + a2 * half;
            glm::vec3 p3 = center - a1 * half + a2 * half;

            glm::vec4 color = (active == h) ? colHighlight : baseColor;
            DebugDrawSystem::AddLine(p0, p1, color);
            DebugDrawSystem::AddLine(p1, p2, color);
            DebugDrawSystem::AddLine(p2, p3, color);
            DebugDrawSystem::AddLine(p3, p0, color);
        };

        drawPlaneQuad(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), Handle::PlaneXY, colZ); // plaszczyzna XY -> kolor "trzeciej" osi (konwencja Unity)
        drawPlaneQuad(glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), Handle::PlaneXZ, colY);
        drawPlaneQuad(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), Handle::PlaneYZ, colX);
    }

    inline Handle PickHandle(const Ray& ray, const glm::vec3& origin, float scale, glm::vec3& outHitPoint)
    {
        float pickRadius = scale * Get().axisPickRadiusFactor;

        struct AxisCandidate { Handle h; glm::vec3 dir; };
        AxisCandidate axes[3] = {
            { Handle::AxisX, glm::vec3(1,0,0) },
            { Handle::AxisY, glm::vec3(0,1,0) },
            { Handle::AxisZ, glm::vec3(0,0,1) },
        };

        Handle best = Handle::None;
        float bestRayT = 1e30f;
        glm::vec3 bestPoint(0.0f);

        for (auto& a : axes)
        {
            glm::vec3 segA = origin;
            glm::vec3 segB = origin + a.dir * scale;

            float tRay, tSeg;
            float dist = DistanceRayToSegment(ray, segA, segB, tRay, tSeg);

            if (dist <= pickRadius && tRay < bestRayT)
            {
                bestRayT = tRay;
                best = a.h;
                bestPoint = segA + (segB - segA) * tSeg;
            }
        }

        float off  = scale * Get().planeHandleOffsetFactor;
        float half = scale * Get().planeHandleSizeFactor * 0.5f;

        struct PlaneCandidate { Handle h; glm::vec3 a1; glm::vec3 a2; glm::vec3 normal; };
        PlaneCandidate planes[3] = {
            { Handle::PlaneXY, glm::vec3(1,0,0), glm::vec3(0,1,0), glm::vec3(0,0,1) },
            { Handle::PlaneXZ, glm::vec3(1,0,0), glm::vec3(0,0,1), glm::vec3(0,1,0) },
            { Handle::PlaneYZ, glm::vec3(0,1,0), glm::vec3(0,0,1), glm::vec3(1,0,0) },
        };

        for (auto& p : planes)
        {
            glm::vec3 center = origin + p.a1 * off + p.a2 * off;
            glm::vec3 hit;
            if (!RayVsPlane(ray, center, p.normal, hit)) continue;

            glm::vec3 rel = hit - center;
            float u = glm::dot(rel, p.a1);
            float v = glm::dot(rel, p.a2);

            if (glm::abs(u) <= half && glm::abs(v) <= half)
            {
                float tRay = ClosestTOnRay(ray, hit);
                if (tRay >= 0.0f && tRay < bestRayT)
                {
                    bestRayT = tRay;
                    best = p.h;
                    bestPoint = hit;
                }
            }
        }

        outHitPoint = bestPoint;
        return best;
    }

    inline void HandleAxes(Handle h, glm::vec3& outAxisA, glm::vec3& outAxisB, bool& outIsPlane)
    {
        outIsPlane = false;
        switch (h)
        {
            case Handle::AxisX:   outAxisA = glm::vec3(1,0,0); outAxisB = glm::vec3(0,0,0); break;
            case Handle::AxisY:   outAxisA = glm::vec3(0,1,0); outAxisB = glm::vec3(0,0,0); break;
            case Handle::AxisZ:   outAxisA = glm::vec3(0,0,1); outAxisB = glm::vec3(0,0,0); break;
            case Handle::PlaneXY: outAxisA = glm::vec3(1,0,0); outAxisB = glm::vec3(0,1,0); outIsPlane = true; break;
            case Handle::PlaneXZ: outAxisA = glm::vec3(1,0,0); outAxisB = glm::vec3(0,0,1); outIsPlane = true; break;
            case Handle::PlaneYZ: outAxisA = glm::vec3(0,1,0); outAxisB = glm::vec3(0,0,1); outIsPlane = true; break;
            default:              outAxisA = glm::vec3(0); outAxisB = glm::vec3(0); break;
        }
    }

    inline bool UpdateAndDraw(GameObject* selected, CameraComponent& camP1, TransformComponent& camTransformP1,
        GLFWwindow* window, bool focused, int windowW, int windowH)
    {
        State& st = Get();

        if (!selected || focused)
        {
            st.hoveredHandle = Handle::None;
            st.draggedHandle = Handle::None;
            st.leftMouseWasDown = false;
            return false;
        }

        auto* tr = selected->GetComponent<TransformComponent>();
        if (!tr) return false;

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        Ray ray;
        bool rayValid = ScreenToWorldRay(camP1, camTransformP1, mouseX, mouseY, windowW, windowH, ray);

        glm::vec3 cameraPos = TransformHelper::getGlobalPosition(camTransformP1);

        float vpHeightPixels = camP1.viewport.height * windowH;
        float scale = ComputeGizmoScale(camP1, cameraPos, tr->position, (int)vpHeightPixels);

        bool leftDown = rayValid && (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        bool leftJustPressed = leftDown && !st.leftMouseWasDown;
        bool leftJustReleased = !leftDown && st.leftMouseWasDown;

        if (st.draggedHandle == Handle::None)
        {
            glm::vec3 hitPoint;
            st.hoveredHandle = rayValid ? PickHandle(ray, tr->position, scale, hitPoint) : Handle::None;

            if (leftJustPressed && st.hoveredHandle != Handle::None)
            {
                st.draggedHandle = st.hoveredHandle;
                st.dragStartHitPoint = hitPoint;
                st.dragStartObjectPos = tr->position;
            }
        }
        else
        {
            if (!leftDown)
            {
                st.draggedHandle = Handle::None;
            }
            else if (rayValid)
            {
                glm::vec3 axisA, axisB;
                bool isPlane;
                HandleAxes(st.draggedHandle, axisA, axisB, isPlane);

                glm::vec3 planeNormal;
                if (isPlane)
                {
                    planeNormal = glm::normalize(glm::cross(axisA, axisB));
                }
                else
                {
                    glm::vec3 toCam = glm::normalize(cameraPos - st.dragStartObjectPos);
                    glm::vec3 perp = glm::cross(axisA, toCam);
                    if (glm::length(perp) < 1e-4f)
                    {
                        perp = glm::cross(axisA, glm::vec3(0, 1, 0));
                        if (glm::length(perp) < 1e-4f) perp = glm::vec3(1, 0, 0);
                    }
                    planeNormal = glm::normalize(glm::cross(axisA, perp));
                }

                glm::vec3 hit;
                if (RayVsPlane(ray, st.dragStartObjectPos, planeNormal, hit))
                {
                    glm::vec3 delta = hit - st.dragStartHitPoint;

                    glm::vec3 movement;
                    if (isPlane)
                    {
                        float du = glm::dot(delta, axisA);
                        float dv = glm::dot(delta, axisB);
                        movement = axisA * du + axisB * dv;
                    }
                    else
                    {
                        float d = glm::dot(delta, axisA);
                        movement = axisA * d;
                    }

                    tr->position = st.dragStartObjectPos + movement;
                    tr->isDirty = true;

                    if (auto* col = selected->GetComponent<ColliderComponent>())
                        col->Recalculate(selected);
                }
            }
        }

        st.leftMouseWasDown = leftDown;

        DrawGizmo(tr->position, scale, st.hoveredHandle, st.draggedHandle);

        return st.draggedHandle != Handle::None;
    }
}