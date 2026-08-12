#include "icons.h"
#include <cmath>

namespace Icons {

static ImVec2 Add(ImVec2 a, ImVec2 b) { return ImVec2(a.x + b.x, a.y + b.y); }
static ImVec2 Rot(ImVec2 v, float rad) {
    float c = cosf(rad), s = sinf(rad);
    return ImVec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

void Draw(ImDrawList* dl, Type type, ImVec2 c, float size, ImU32 color, float th) {
    float r = size * 0.5f;

    switch (type) {
    case Type::Link: {
        for (int i = -1; i <= 1; i += 2) {
            ImVec2 offset = Rot(ImVec2(size * 0.20f * i, 0), 0.78f);
            ImVec2 center = Add(c, offset);
            ImVec2 half = Rot(ImVec2(size * 0.24f, 0), 0.78f);
            ImVec2 halfPerp = Rot(ImVec2(0, size * 0.14f), 0.78f);
            ImVec2 p0 = Add(center, ImVec2(-half.x - halfPerp.x, -half.y - halfPerp.y));
            ImVec2 p1 = Add(center, ImVec2(half.x - halfPerp.x, half.y - halfPerp.y));
            ImVec2 p2 = Add(center, ImVec2(half.x + halfPerp.x, half.y + halfPerp.y));
            ImVec2 p3 = Add(center, ImVec2(-half.x + halfPerp.x, -half.y + halfPerp.y));
            dl->AddQuad(p0, p1, p2, p3, color, th);
        }
        break;
    }
    case Type::Unlink: {
        Draw(dl, Type::Link, c, size, color, th);
        dl->AddLine(Add(c, ImVec2(-r, r)), Add(c, ImVec2(r, -r)), color, th);
        break;
    }
    case Type::Phone: {
        ImVec2 p0 = Add(c, ImVec2(-r * 0.5f, -r));
        ImVec2 p1 = Add(c, ImVec2(r * 0.5f, r));
        dl->AddRect(p0, p1, color, 3.0f, 0, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.15f, r * 0.72f)), Add(c, ImVec2(r * 0.15f, r * 0.72f)), color, th);
        break;
    }
    case Type::Gamepad: {
        ImVec2 p0 = Add(c, ImVec2(-r, -r * 0.35f));
        ImVec2 p1 = Add(c, ImVec2(r, r * 0.45f));
        dl->AddRect(p0, p1, color, r * 0.5f, 0, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.55f, -r * 0.08f)), Add(c, ImVec2(-r * 0.55f, r * 0.18f)), color, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.7f, r * 0.05f)), Add(c, ImVec2(-r * 0.4f, r * 0.05f)), color, th);
        dl->AddCircle(Add(c, ImVec2(r * 0.55f, -r * 0.02f)), r * 0.14f, color, 0, th);
        dl->AddCircle(Add(c, ImVec2(r * 0.25f, r * 0.18f)), r * 0.14f, color, 0, th);
        break;
    }
    case Type::Grid: {
        float cell = size * 0.42f;
        float gap = size * 0.16f;
        for (int gy = 0; gy < 2; ++gy)
            for (int gx = 0; gx < 2; ++gx) {
                ImVec2 p0 = Add(c, ImVec2(-cell - gap * 0.5f + gx * (cell + gap), -cell - gap * 0.5f + gy * (cell + gap)));
                ImVec2 p1 = Add(p0, ImVec2(cell, cell));
                dl->AddRect(p0, p1, color, 2.0f, 0, th);
            }
        break;
    }
    case Type::Folder: {
        ImVec2 p0 = Add(c, ImVec2(-r, -r * 0.35f));
        ImVec2 p1 = Add(c, ImVec2(r, r * 0.75f));
        dl->AddRect(p0, p1, color, 2.5f, 0, th);
        ImVec2 t0 = Add(c, ImVec2(-r, -r * 0.35f));
        ImVec2 t1 = Add(c, ImVec2(-r * 0.15f, -r * 0.35f));
        ImVec2 t2 = Add(c, ImVec2(r * 0.05f, -r * 0.7f));
        ImVec2 t3 = Add(c, ImVec2(-r * 0.85f, -r * 0.7f));
        dl->AddLine(t3, t0, color, th);
        dl->AddLine(t3, t2, color, th);
        dl->AddLine(t2, t1, color, th);
        break;
    }
    case Type::Terminal: {
        ImVec2 p0 = Add(c, ImVec2(-r, -r * 0.75f));
        ImVec2 p1 = Add(c, ImVec2(r, r * 0.75f));
        dl->AddRect(p0, p1, color, 3.0f, 0, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.55f, -r * 0.15f)), Add(c, ImVec2(-r * 0.15f, 0)), color, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.55f, r * 0.15f)), Add(c, ImVec2(-r * 0.15f, 0)), color, th);
        dl->AddLine(Add(c, ImVec2(0, r * 0.32f)), Add(c, ImVec2(r * 0.45f, r * 0.32f)), color, th);
        break;
    }
    case Type::Gear: {
        dl->AddCircle(c, r * 0.42f, color, 0, th);
        for (int i = 0; i < 8; ++i) {
            float a = (float)i / 8.0f * 6.2831853f;
            ImVec2 inner = Add(c, ImVec2(cosf(a) * r * 0.6f, sinf(a) * r * 0.6f));
            ImVec2 outer = Add(c, ImVec2(cosf(a) * r * 0.95f, sinf(a) * r * 0.95f));
            dl->AddLine(inner, outer, color, th);
        }
        break;
    }
    case Type::Info: {
        dl->AddCircle(c, r * 0.9f, color, 0, th);
        dl->AddCircleFilled(Add(c, ImVec2(0, -r * 0.38f)), th * 0.7f, color);
        dl->AddLine(Add(c, ImVec2(0, -r * 0.05f)), Add(c, ImVec2(0, r * 0.45f)), color, th);
        break;
    }
    case Type::Camera: {
        ImVec2 p0 = Add(c, ImVec2(-r, -r * 0.5f));
        ImVec2 p1 = Add(c, ImVec2(r, r * 0.6f));
        dl->AddRect(p0, p1, color, 3.0f, 0, th);
        dl->AddRect(Add(c, ImVec2(-r * 0.3f, -r * 0.85f)), Add(c, ImVec2(r * 0.2f, -r * 0.5f)), color, 2.0f, 0, th);
        dl->AddCircle(Add(c, ImVec2(0, r * 0.05f)), r * 0.32f, color, 0, th);
        break;
    }
    case Type::VideoRec: {
        dl->AddCircle(c, r * 0.85f, color, 0, th);
        dl->AddCircleFilled(c, r * 0.4f, color);
        break;
    }
    case Type::ApkBox: {
        ImVec2 p0 = Add(c, ImVec2(-r * 0.85f, -r * 0.15f));
        ImVec2 p1 = Add(c, ImVec2(r * 0.85f, r * 0.85f));
        dl->AddRect(p0, p1, color, 2.0f, 0, th);
        dl->AddLine(Add(c, ImVec2(0, -r * 0.9f)), Add(c, ImVec2(0, -r * 0.05f)), color, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.3f, -r * 0.35f)), Add(c, ImVec2(0, -r * 0.05f)), color, th);
        dl->AddLine(Add(c, ImVec2(r * 0.3f, -r * 0.35f)), Add(c, ImVec2(0, -r * 0.05f)), color, th);
        break;
    }
    case Type::Trash: {
        dl->AddLine(Add(c, ImVec2(-r * 0.7f, -r * 0.55f)), Add(c, ImVec2(r * 0.7f, -r * 0.55f)), color, th);
        ImVec2 p0 = Add(c, ImVec2(-r * 0.5f, -r * 0.55f));
        ImVec2 p1 = Add(c, ImVec2(r * 0.5f, r * 0.85f));
        dl->AddRect(p0, p1, color, 2.0f, 0, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.25f, -r * 0.75f)), Add(c, ImVec2(r * 0.25f, -r * 0.75f)), color, th);
        dl->AddLine(Add(c, ImVec2(-r * 0.15f, -r * 0.15f)), Add(c, ImVec2(-r * 0.15f, r * 0.5f)), color, th * 0.8f);
        dl->AddLine(Add(c, ImVec2(r * 0.15f, -r * 0.15f)), Add(c, ImVec2(r * 0.15f, r * 0.5f)), color, th * 0.8f);
        break;
    }
    case Type::Refresh: {
        dl->PathArcTo(c, r * 0.75f, -1.2f, 4.2f, 16);
        dl->PathStroke(color, 0, th);
        ImVec2 tip = Add(c, ImVec2(cosf(4.2f) * r * 0.75f, sinf(4.2f) * r * 0.75f));
        ImVec2 dir = Rot(ImVec2(r * 0.28f, 0), 4.2f + 1.6f);
        ImVec2 dir2 = Rot(ImVec2(r * 0.28f, 0), 4.2f - 1.0f);
        dl->AddLine(tip, Add(tip, dir), color, th);
        dl->AddLine(tip, Add(tip, dir2), color, th);
        break;
    }
    case Type::Power: {
        dl->PathArcTo(c, r * 0.8f, -2.0f, 4.28f, 20);
        dl->PathStroke(color, 0, th);
        dl->AddLine(Add(c, ImVec2(0, -r * 0.95f)), Add(c, ImVec2(0, -r * 0.15f)), color, th);
        break;
    }
    case Type::Rotate: {
        dl->PathArcTo(c, r * 0.7f, 0.3f, 5.0f, 16);
        dl->PathStroke(color, 0, th);
        ImVec2 tip = Add(c, ImVec2(cosf(0.3f) * r * 0.7f, sinf(0.3f) * r * 0.7f));
        dl->AddLine(tip, Add(tip, ImVec2(-r * 0.28f, -r * 0.05f)), color, th);
        dl->AddLine(tip, Add(tip, ImVec2(-r * 0.02f, -r * 0.3f)), color, th);
        break;
    }
    case Type::Volume: {
        ImVec2 p0 = Add(c, ImVec2(-r * 0.9f, -r * 0.25f));
        ImVec2 p1 = Add(c, ImVec2(-r * 0.35f, r * 0.25f));
        dl->AddRect(p0, p1, color, 1.0f, 0, th);
        ImVec2 t0 = Add(c, ImVec2(-r * 0.35f, -r * 0.25f));
        ImVec2 t1 = Add(c, ImVec2(r * 0.05f, -r * 0.65f));
        ImVec2 t2 = Add(c, ImVec2(r * 0.05f, r * 0.65f));
        ImVec2 t3 = Add(c, ImVec2(-r * 0.35f, r * 0.25f));
        dl->AddLine(t0, t1, color, th);
        dl->AddLine(t1, t2, color, th);
        dl->AddLine(t2, t3, color, th);
        dl->PathArcTo(Add(c, ImVec2(r * 0.05f, 0)), r * 0.5f, -0.6f, 0.6f, 8);
        dl->PathStroke(color, 0, th);
        break;
    }
    case Type::ChevronDown: {
        dl->AddLine(Add(c, ImVec2(-r * 0.5f, -r * 0.2f)), Add(c, ImVec2(0, r * 0.3f)), color, th);
        dl->AddLine(Add(c, ImVec2(r * 0.5f, -r * 0.2f)), Add(c, ImVec2(0, r * 0.3f)), color, th);
        break;
    }
    case Type::Dots: {
        for (int i = -1; i <= 1; ++i)
            dl->AddCircleFilled(Add(c, ImVec2(0, i * r * 0.55f)), th * 0.9f, color);
        break;
    }
    case Type::Close: {
        dl->AddLine(Add(c, ImVec2(-r * 0.45f, -r * 0.45f)), Add(c, ImVec2(r * 0.45f, r * 0.45f)), color, th);
        dl->AddLine(Add(c, ImVec2(r * 0.45f, -r * 0.45f)), Add(c, ImVec2(-r * 0.45f, r * 0.45f)), color, th);
        break;
    }
    case Type::Minimize: {
        dl->AddLine(Add(c, ImVec2(-r * 0.5f, 0)), Add(c, ImVec2(r * 0.5f, 0)), color, th);
        break;
    }
    case Type::Maximize: {
        ImVec2 p0 = Add(c, ImVec2(-r * 0.42f, -r * 0.42f));
        ImVec2 p1 = Add(c, ImVec2(r * 0.42f, r * 0.42f));
        dl->AddRect(p0, p1, color, 0, 0, th);
        break;
    }
    case Type::Search: {
        dl->AddCircle(Add(c, ImVec2(-r * 0.12f, -r * 0.12f)), r * 0.5f, color, 0, th);
        ImVec2 dir = ImVec2(0.72f, 0.72f);
        ImVec2 start = Add(c, ImVec2(dir.x * r * 0.32f, dir.y * r * 0.32f));
        ImVec2 end = Add(c, ImVec2(dir.x * r * 0.9f, dir.y * r * 0.9f));
        dl->AddLine(start, end, color, th);
        break;
    }
    }
}

} // namespace Icons
