#ifndef ISLE_BORROWED_MODEL_KERNEL_HPP
#define ISLE_BORROWED_MODEL_KERNEL_HPP

#include "35mmisle/engine.h"
#include "menu_animation.hpp"

namespace isle::menu::model {
struct Command { std::uint32_t w0 = 0, w1 = 0; };
struct ListRange { std::uint32_t address = 0, commands = 0; };
struct WorldMatrix { float m[4][4]{}; };
enum class Error { none, invalid_input, extent, reservation, relocation_required, native, allocation, retired };
struct Status {
    Error error = Error::none;
    std::uint32_t address = 0;
    explicit operator bool() const noexcept { return error == Error::none; }
};
// Effective configured kinds are selected by PokemonInitData flags before tree
// decoding. High tree flags then decompose/override that sequence, in source order.
std::array<std::uint8_t,3> select_source_matrix_kinds(std::uint32_t flags,
    std::array<std::uint8_t,3> stored) noexcept;
enum class RootRecipe { source, translate };
struct MatrixRecipe {
    std::array<std::uint8_t,3> operations{};
    std::uint8_t count = 0;
    IsleGameModelMatrixKind native_kind = ISLE_GAME_MODEL_ROTATE_RPY_TRANSLATE_SCALE;
};
Status decode_recipe(std::array<std::uint8_t,3>, std::uint32_t depth_flags,
                     MatrixRecipe&) noexcept;
Status decode_model_recipe(const Model&, std::size_t node, MatrixRecipe&) noexcept;
// Scratch is dedicated caller-owned capacity, allocated before spawning. Only
// its active prefix changes on rejection; no vector allocation or second
// attachment implementation. translate is the established World root policy.
Status attach_source_model(IsleGame*, IsleGameObject*, const Model&, animation::ModelState,
    RootRecipe, std::span<IsleGameModelNodeInit> scratch, IsleGameModel**) noexcept;
// Explicit values: no camera pointer survives preparation. Basis starts at
// source BSS zero, not identity/perspective. The caller publishes the staged
// result only after its whole camera/model/list transaction succeeds.
struct RenderContext { WorldMatrix perspective, basis48; };
struct CameraBasisState {
    RenderContext value{};
    unsigned selector = 0;
    void reset_selector() noexcept { selector = 0; } // Source reset retains matrices.
};
struct CameraMatrixOperation { std::uint8_t kind = 0, source_cache_state = 0; };
Status prepare_render_context(const RenderContext& retained,
    const IsleGameCameraParameters&, std::span<const CameraMatrixOperation>,
    unsigned selector, RenderContext& output) noexcept;
// Bounded source renLoadTextures encoder. No output publication on rejection.
struct MaterialCommands { std::array<Command,32> words{}; std::size_t count = 0; };
Status encode_material(const Material&, const animation::MaterialState&, MaterialCommands&) noexcept;
// Shared source B/I immediate and D/J deferred hierarchy. Scratch alone may
// change on rejection. Topology spans are admitted parent-before-child, forward
// siblings; the kernel builds them once per resource/incarnation binding.
struct HierarchyInput {
    const Model& source;
    std::span<const animation::NodeState> nodes;
    std::span<const std::uint32_t> first_child, next_sibling, material_bases;
    const IsleGameModelPlacement& placement;
    const RenderContext& context;
    std::uint32_t matrices = 0, placement_address = 0;
    // Empty for animated pose; World borrows its synchronized native pose.
    std::span<const IsleGameModelNode> native_nodes{};
    std::span<const MatrixRecipe> recipes;
    std::span<IsleGameFixedMatrix> contextual_fixed;
    std::span<unsigned char> contextual_consumed;
};
struct HierarchyBuffer {
    std::array<std::span<Command>,4> lanes;
    std::array<std::size_t,4>& counts;
    std::span<Command> pending;
};
Status encode_hierarchy(const HierarchyInput&, HierarchyBuffer&) noexcept;

// The owner supplies actual render order, fresh Runtime views and real native
// attachments. Neither the kernel nor its destructor creates/deletes owners.
enum class PoseSource { animation, native };
struct BorrowedBinding {
    std::size_t slot = 0;
    animation::ModelState state;
    IsleGameObject* object = nullptr;
    IsleGameModel* model = nullptr;
    PoseSource pose = PoseSource::animation;
    RootRecipe root = RootRecipe::source;
    bool draw = true;
    // Re-resolved during stage: a previously successful copied view cannot
    // authorize a retired/reused/wrong-pool instance.
    const animation::Runtime* runtime = nullptr;
};
struct KernelSlot {
    animation::SlotCapacity capacity;
    std::uint32_t matrices = 0, materials = 0;
};
// One immutable ordered plan and one renderer-owned contextual fixed scratch
// per slot; no context-free native fixed payload exists for source54.
class BorrowedKernel final {
public:
    BorrowedKernel(IsleGame*, std::span<const Model>, std::span<const Material>,
                   std::span<const KernelSlot>, RamRange placement_payloads);
    ~BorrowedKernel();
    BorrowedKernel(const BorrowedKernel&) = delete;
    BorrowedKernel& operator=(const BorrowedKernel&) = delete;
    // Complete bounded preflight. Scratch alone may change on failure. Every
    // active binding, including hidden/pass-filtered ones, must be supplied.
    // No mutation/callback may intervene before commit_pose/upload.
    Status validate_upload(std::span<std::uint32_t>, std::uint64_t epoch) const noexcept;
    Status stage(std::span<const BorrowedBinding>, std::size_t placement_prefix_bytes = 0) noexcept;
    Status encode(std::size_t slot, const RenderContext&, HierarchyBuffer&) const noexcept;
    bool lists_changed() const noexcept;
    bool draws(std::size_t slot) const noexcept;
    // Legal only after all owner camera/list/capacity preflight succeeds.
    void commit_pose() noexcept;
    // Writes ordinary/native or consumed renderer-contextual matrices,
    // placement, branch tables and materials. Never uploads contextual native fixed.
    // Pointer+nonzero epoch identifies the exclusive RAM installation. Hidden
    // bindings retain their old residency key, so reveal always republishes.
    void upload(std::span<std::uint32_t>, std::uint64_t epoch) noexcept;
    // Mathematical local RTS x parent/placement, not rendered MVP and not a
    // claim of source54 actor FX or placement/collision equivalence.
    std::span<const WorldMatrix> world_nodes(animation::InstanceHandle) const noexcept;
    std::span<const IsleGameModelNode> native_nodes(animation::InstanceHandle) const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace isle::menu::model
#endif
