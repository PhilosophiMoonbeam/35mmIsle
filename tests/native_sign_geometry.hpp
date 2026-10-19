#ifndef ISLE_TEST_NATIVE_SIGN_GEOMETRY_HPP
#define ISLE_TEST_NATIVE_SIGN_GEOMETRY_HPP

#include "actor_resources.hpp"
#include "borrowed_model_kernel.hpp"
#include <array>

namespace isle::test {

// Private input-aim observer, never focus/capture evidence. Own one per trip and
// reset before its immutable Resources owner is destroyed or replaced.
class SignGeometry final {
public:
    static constexpr std::size_t model_capacity = 1;
    // Exact maxima from all six admitted sign models/DL membership audits.
    static constexpr std::size_t node_capacity = 34;
    static constexpr std::size_t membership_capacity = 37;
    // Visibility here means native object/node gates, not camera/DL culling,
    // backface/alpha coverage or the actual detector. ready is an aim hint only.
    enum class Status { ready, no_visible_geometry, rejected };
    enum class TargetKind { bounds_center, invariant_origin, optical_axis_hint };
    enum class Rejection {
        none, capacity, initialization, topology, render_type, matrix_recipe,
        display_list, vertex_descriptor, geometry_command, incoming_vertices,
        arithmetic, pose, world_view, camera_view
    };
    struct Result {
        Status status = Status::rejected;
        Rejection rejection = Rejection::none;
        // minimum/maximum are meaningful only for bounds_center. Other kinds
        // claim no billboard extent or GPU quantization parity. optical_axis_hint
        // is a steering fixed point, NOT the current projected mesh centre.
        TargetKind kind = TargetKind::bounds_center;
        menu::Vec3 target{}, minimum{}, maximum{};
        std::size_t visible_nodes = 0;
        std::size_t node = menu::no_index;
        std::uint32_t command_identity = 0;
    };
    struct BillboardView {
        // Actual same-Frame camera peek records (indices0/1, kinds3/14), and
        // the selected native model's placement. All are scoped read-only borrows.
        // Peek alone permits stale payloads: these MUST be from the same
        // successful preparation/exclusion interval as world/native/placement.
        // Use camera_peek_matrix, never the computing camera_matrix API.
        const IsleGameCameraMatrix* perspective = nullptr;
        const IsleGameCameraMatrix* view = nullptr;
        const IsleGameModelPlacement* placement = nullptr;
    };

    // The initialization must be the actual entry borrowed from resources.
    // world/native are same-instance, same-pose views from the existing actor
    // Producer after its successful ordinary preparation; the caller must keep
    // native/GPU lifetime control and forbid mutation throughout this call.
    // object_visible includes actual native object/pass visibility. No render,
    // prepare, binding, Source callback, or native matrix API is invoked here.
    // No native pointer, span, handle or instance identity survives this call.
    // Supports Source28/27 mathematical world views and A/B/I/D/J hierarchy.
    // Bounds conservatively cover potentially drawable TRI1/TRI2 vertices,
    // including nested static DL calls and both CULLDL return/continue paths.
    // Exact validated generated-material calls are geometry-free and skipped.
    // Exact18+44 supports origin-centred geometry, or XY-centred planar
    // geometry as optical_axis_hint with a BillboardView proving front W.
    // Descendants require zero live translation; all retained billboard
    // geometry must share one anchor. No billboard extent is claimed.
    // Other contextual recipes, BRANCH_Z, pre-list geometry and incoming-node/
    // lane vertex dependencies reject.
    Result observe(const actor::Resources& resources,
                   const actor::ActorInitialization& initialization,
                   std::span<const menu::model::WorldMatrix> world,
                   std::span<const IsleGameModelNode> native,
                   bool object_visible,
                   const BillboardView* billboard = nullptr) noexcept;
    void reset() noexcept;

private:
    struct Bounds {
        menu::Vec3 minimum{}, maximum{};
        bool present = false;
    };
    struct Node {
        menu::Vec3 center{}, half_extent{};
        bool geometry = false;
        bool origin_only = false;
        bool billboard44 = false;
        std::size_t parent = menu::no_index, subtree_end = 0;
        IsleGameModelMatrixKind matrix_kind = ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE;
    };
    struct Membership { std::uint32_t list = 0; std::uint16_t node = 0; };
    struct Model {
        const actor::Resources* resources = nullptr;
        const actor::ActorInitialization* initialization = nullptr;
        std::size_t count = 0, memberships = 0;
        bool root_only = false;
        bool axis_camera = false;
        Result admission;
        std::array<Node, node_capacity> nodes{};
        // Relational membership is (node,list), not a global list-address union:
        // a shared nested list may legitimately belong to several animated nodes.
        std::array<Membership, membership_capacity> lists{};
    };
    struct Builder;
    static void include(Bounds&, menu::Vec3) noexcept;
    std::array<Model, model_capacity> models_{};
    std::size_t count_ = 0;
};

} // namespace isle::test
#endif
