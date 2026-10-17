#ifndef ISLE_CART_ANIMATION_REFERENCE_HPP
#define ISLE_CART_ANIMATION_REFERENCE_HPP
#include "menu_resources.hpp"
#include "rom_snapshot.hpp"
#include "cart_animation_reference_numeric.h"
#include <array>
#include <vector>

namespace isle::player::reference {
float camera_cubic(float initial, float target, float rate, float target_rate, float duration, float time);
float model_cubic(float initial, float target, float rate, float target_rate, float duration, float time);
enum class PathDomain { model, eye, at };
struct PathSample { menu::Vec3 position; float mapped_parameter; };
class CartNumericReference {
public:
    explicit CartNumericReference(const rom::Snapshot&);
    PathSample sample(bool intro, PathDomain, float normalized_animation_parameter) const;
private:
    struct Path {
        int kind = 0, count = 0;
        float tension = 0, duration = 0;
        std::vector<IsleReferencePoint> points;
        std::vector<float> times;
        std::vector<IsleReferenceSpeed> speeds;
    };
    std::array<Path,6> paths_;
};
}
#endif
