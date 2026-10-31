#include "ship/config/Config.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldevice/controller/ControllerStick.h"
#include <array>
#include <cfenv>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
struct Scratch {
    std::filesystem::path path;
    bool owned = false;
    ~Scratch() {
        if (owned) {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    }
};

// Only the raw device source is synthetic; normalization and settings are LUS.
class Axis final : public Ship::ControllerAxisDirectionMapping {
public:
    explicit Axis(Ship::Direction direction)
        : Ship::ControllerInputMapping(Ship::Keyboard),
          Ship::ControllerAxisDirectionMapping(Ship::Keyboard, 0, Ship::LEFT_STICK, direction) {}
    float GetNormalizedAxisDirectionValue() override { return value; }
    std::string GetAxisDirectionMappingId() override { return "endpoint-input"; }
    void SaveToConfig() override { throw std::logic_error("axis fixture persistence is not exercised"); }
    void EraseFromConfig() override { throw std::logic_error("axis fixture persistence is not exercised"); }
    float value = 0;
};
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("expected build scratch parent directory");
        Scratch scratch{std::filesystem::path(argv[1]) /
            ("35mmisle-controller-stick-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()))};
        if (!std::filesystem::create_directory(scratch.path))
            throw std::runtime_error("controller stick scratch directory already exists");
        scratch.owned = true;
        int result = 0;
        {
            auto config = std::make_shared<Ship::Config>((scratch.path / "controls.json").string());
            auto variables = std::make_shared<Ship::ConsoleVariable>(config);
            Ship::ControllerStick stick(0, Ship::LEFT_STICK, variables);
            stick.SetSensitivity(100);
            stick.SetNotchSnapAngle(0);
            std::array<std::shared_ptr<Axis>, 4> axes;
            for (unsigned i = 0; i < axes.size(); ++i) {
                axes[i] = std::make_shared<Axis>(static_cast<Ship::Direction>(i));
                stick.AddAxisDirectionMapping(static_cast<Ship::Direction>(i), axes[i]);
            }
            bool passed = true;
            auto check = [&](const char* label, std::uint8_t deadzone, float left, float right,
                             float up, float down, int expected_x, int expected_y, int tolerance = 0) {
                stick.SetDeadzone(deadzone);
                axes[Ship::LEFT]->value = left; axes[Ship::RIGHT]->value = right;
                axes[Ship::UP]->value = up; axes[Ship::DOWN]->value = down;
                std::int8_t x = 0, y = 0;
                std::feclearexcept(FE_ALL_EXCEPT);
                stick.UpdatePad(x, y);
                const auto invalid = std::fetestexcept(FE_INVALID | FE_DIVBYZERO);
                const bool okay = invalid == 0 && std::abs(int(x) - expected_x) <= tolerance &&
                                  std::abs(int(y) - expected_y) <= tolerance;
                std::cout << label << " x=" << int(x) << " y=" << int(y)
                          << " invalid_or_divzero=" << invalid << " passed=" << okay << '\n';
                passed = passed && okay;
            };
            check("zero_deadzone_neutral", 0, 0, 0, 0, 0, 0, 0);
            check("zero_deadzone_right", 0, 0, 85, 0, 0, 85, 0);
            check("full_deadzone_right", 100, 0, 85, 0, 0, 0, 0);
            check("full_deadzone_left", 100, 85, 0, 0, 0, 0, 0);
            check("full_deadzone_up", 100, 0, 0, 85, 0, 0, 0);
            check("full_deadzone_down", 100, 0, 0, 0, 85, 0, 0);
            check("interior_exact_threshold", 20, 0, 17, 0, 0, 0, 0);
            check("interior_rescaling", 20, 0, 34, 0, 0, 21, 0);
            check("interior_left_rescaling", 20, 34, 0, 0, 0, -21, 0);
            check("octagonal_diagonal", 20, 0, 85, 85, 0, 69, 69, 1);
            result = passed ? 0 : 1;
        }
        std::filesystem::remove_all(scratch.path);
        scratch.owned = false;
        return result;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
