#include "actor_behavior_bindings.hpp"
#include "rainbow_actor.hpp"
#include "beach_initial_species_source.h"
#include "tunnel_actor.hpp"
#include "volcano_actor.hpp"
#include "river_actor.hpp"
#include "cave_actor.hpp"
#include "valley_actor.hpp"


#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace isle::actor {
namespace {
bool executable_command(std::int32_t command) noexcept {
    // Union of the two explicit source switches. Unknown rows are data only;
    // neither their callback words nor the terminal's unused fields execute.
    return command >= 5 && command <= 57 && command != 11;
}
[[noreturn]] void unresolved(std::uint32_t identity, std::uint32_t graph, std::size_t row) {
    char message[160];
    std::snprintf(message,sizeof(message),
        "Unresolved native callback 0x%08x from graph/list 0x%08x row %zu",
        static_cast<unsigned>(identity),static_cast<unsigned>(graph),row);
    throw std::invalid_argument(message);
}
// Once-only common Original Egg imports: Source identities, not GObj callbacks.
// Their bodies live exclusively in the common-egg C provider.
constexpr std::uint32_t common_egg_dependencies[] = {
    0x800aafb0u, 0x800ab000u, 0x800ab050u,
    0x800ab138u, 0x800ab1f8u, 0x80362414u
};
// Provider dependencies retain these exact typed seams. They must never be
// resolved through the GObj callback registry or cast to its signature.
bool typed_rainbow_dependency(std::uint32_t identity) noexcept {
    if (identity == 0x803466c0u || identity == 0x80346898u ||
        identity == 0x803484f0u) return true;
    for (const auto& binding : rainbow::actor_factories())
        if (binding.factory == identity) return true;
    for (const auto& binding : rainbow::actor_animation_callbacks())
        if (binding.identity == identity) return true;
    for (const auto& binding : rainbow::actor_collision_callbacks())
        if (binding.identity == identity) return true;
    return false;
}
} // namespace

std::span<const BehaviorCallbackBinding> course_callbacks(std::int32_t course) noexcept {
    switch (course) {
    case 1: return tunnel::actor_callbacks();
    case 2: return volcano::actor_callbacks();
    case 3: return river::actor_callbacks();
    case 4: return cave::actor_callbacks();
    case 5: return valley::actor_callbacks();
    case 6: return rainbow::actor_callbacks();
    default: return {};
    }
}
std::span<const std::uint32_t> course_graphs(std::int32_t course) noexcept {
    switch (course) {
    case 1: return tunnel::actor_graphs();
    case 2: return volcano::actor_graphs();
    case 3: return river::actor_graphs();
    case 4: return cave::actor_graphs();
    case 5: return valley::actor_graphs();
    case 6: return rainbow::actor_graphs();
    default: return {};
    }
}
std::span<const std::uint32_t> course_weighted_states(std::int32_t course) noexcept {
    switch (course) {
    case 1: return tunnel::actor_weighted_states();
    case 2: return volcano::actor_weighted_states();
    case 3: return river::actor_weighted_states();
    case 4: return cave::actor_weighted_states();
    case 5: return valley::actor_weighted_states();
    case 6: return rainbow::actor_weighted_states();
    default: return {};
    }
}
std::span<const std::uint32_t> course_roots(std::int32_t course) noexcept {
    switch (course) {
    case 1: return tunnel::actor_roots();
    case 2: return volcano::actor_roots();
    case 3: return river::actor_roots();
    case 4: return cave::actor_roots();
    case 5: return valley::actor_roots();
    case 6: return rainbow::actor_roots();
    default: return {};
    }
}
bool course_typed_dependency(std::int32_t course, std::uint32_t identity) noexcept {
    // A callback is resolved through its typed GObj registry, even when another
    // descriptor also names it. Only genuine other ABIs bypass that registry.
    for (const auto& binding : course_callbacks(course))
        if (binding.identity == identity) return false;
    if (course>=1 && course<=5 && (identity==0x8035e0d4u ||
        identity==0x8035e1d4u || identity==0x8035e238u)) return true;
    if (course == 1 || course == 4)
        for (const auto dependency : common_egg_dependencies)
            if (dependency == identity) return true;
    switch (course) {
    case 1:
        for (const auto& row : tunnel::actor_dependencies())
            if (row.has_guest_identity && row.identity == identity)
                return row.abi != tunnel::SourceAbi::gobj_callback;
        break;
    case 2:
        for (const auto& row : volcano::actor_dependencies())
            if (row.identity == identity) return true;
        break;
    case 3:
        for (const auto& row : river::actor_dependencies())
            if (row.identity == identity) return row.abi != river::ActorSourceAbi::gobj_callback;
        break;
    case 4:
        for (const auto& row : cave::actor_dependencies())
            if (row.identity == identity) return row.abi != cave::ActorSourceAbi::callback;
        break;
    case 5:
        for (const auto& row : valley::actor_dependencies())
            if (row.identity == identity) return row.abi != valley::SourceAbi::gobj_callback;
        break;
    case 6: return typed_rainbow_dependency(identity);
    }
    return false;
}

BehaviorBindings::BehaviorBindings(const BehaviorResources& resources,
    std::span<const BehaviorCallbackBinding> bindings,
    std::span<const std::uint32_t> graph_heads,
    std::span<const std::uint32_t> weighted_heads,
    std::span<const std::uint32_t> callback_heads) : resources_(&resources) {
    resources.require_live();
    const auto source_graphs=resources.graphs();
    const auto source_weighted=resources.weighted_states();
    graphs_.reserve(source_graphs.size());
    weighted_.reserve(source_weighted.size());
    callbacks_.reserve(bindings.size());
    std::vector<const BehaviorCallbackBinding*> registry;
    registry.reserve(bindings.size()+(resources.course_id()>=1 && resources.course_id()<=5 ? 3u : 0u));
    for (const auto& binding:bindings) {
        if (!binding.identity || !binding.function)
            throw std::invalid_argument("Native callback binding requires nonzero identity and typed function");
        registry.push_back(&binding);
    }
    const std::uint32_t shared_dependencies[]={0x8035dfb0u,0x8035e034u};
    const BehaviorCallbackBinding shared[] = {
        {0x8035df20u,func_8035DF20_4FE330,shared_dependencies},
        {0x8035dfb0u,func_8035DFB0_4FE3C0},
        {0x8035e034u,func_8035E034_4FE444}
    };
    if (resources.course_id()>=1 && resources.course_id()<=5)
        for (const auto& binding : shared)
            if (std::none_of(registry.begin(),registry.end(),[&](const auto* row) {
                return row->identity==binding.identity;
            })) registry.push_back(&binding);
    std::sort(registry.begin(),registry.end(),[](const auto* a,const auto* b) {
        return a->identity < b->identity;
    });
    for (std::size_t i=1;i<registry.size();++i)
        if (registry[i-1]->identity==registry[i]->identity)
            throw std::invalid_argument("Duplicate native callback identity");

    enum class Kind { graph, weighted, callback };
    struct Work { Kind kind; std::size_t index; };
    std::vector<Work> work;
    work.reserve(source_graphs.size()+source_weighted.size()+registry.size());
    std::vector<unsigned char> graphs_seen(source_graphs.size()),
        weighted_seen(source_weighted.size()), callbacks_seen(registry.size());
    auto graph=[&](std::uint32_t identity) {
        const auto& source=resources.graph(identity);
        const auto index=static_cast<std::size_t>(&source-source_graphs.data());
        if (!graphs_seen[index]) { graphs_seen[index]=1; work.push_back({Kind::graph,index}); }
    };
    auto weighted=[&](std::uint32_t identity) {
        const auto& source=resources.weighted_states(identity);
        const auto index=static_cast<std::size_t>(&source-source_weighted.data());
        if (!weighted_seen[index]) { weighted_seen[index]=1; work.push_back({Kind::weighted,index}); }
    };
    auto callback=[&](std::uint32_t identity,std::uint32_t origin,std::size_t row) {
        const auto found=std::lower_bound(registry.begin(),registry.end(),identity,
            [](const auto* candidate,std::uint32_t value) { return candidate->identity<value; });
        if (found==registry.end() || (*found)->identity!=identity) unresolved(identity,origin,row);
        const auto index=static_cast<std::size_t>(found-registry.begin());
        if (!callbacks_seen[index]) {
            callbacks_seen[index]=1;
            callbacks_.push_back({identity,(*found)->function,{}});
            work.push_back({Kind::callback,index});
        }
        return (*found)->function;
    };
    // RandomState2 lists are counted, not terminated. Resolve NULL outcomes as
    // NULL and preserve every byte weight/order; the Original chooser owns RNG.
    counted_.reserve(resources.counted_state_options().size());
    for (const auto& source : resources.counted_state_options()) {
        Counted result;
        result.rows.reserve(source.entries.size());
        for (const auto& row : source.entries)
            result.rows.push_back({row.weight,row.callback_identity ?
                callback(row.callback_identity,source.wire.guest_address,result.rows.size()) : nullptr});
        result.view={source.wire.guest_address,result.rows};
        counted_.push_back(std::move(result));
    }
    if (resources.course_id()>=1 && resources.course_id()<=5) {
        // A native scene admits the whole selected Original provider, including
        // aux/path/helper callbacks absent from static graph rows. No narrowed
        // caller root list can accidentally publish an incomplete ACTIVE scene.
        for (const auto* binding:registry) callback(binding->identity,0,0);
        for (const auto identity:course_graphs(resources.course_id())) graph(identity);
        for (const auto identity:course_weighted_states(resources.course_id())) weighted(identity);
    }
    for (const auto identity:graph_heads) graph(identity);
    for (const auto identity:weighted_heads) weighted(identity);
    for (const auto identity:callback_heads) callback(identity,0,0);
    for (std::size_t cursor=0;cursor<work.size();++cursor) {
        const auto current=work[cursor];
        if (current.kind==Kind::callback) {
            const auto& binding=*registry[current.index];
            for (const auto identity:binding.callbacks)
                if (!course_typed_dependency(resources.course_id(),identity))
                    callback(identity,binding.identity,0);
            for (const auto identity:binding.graphs) graph(identity);
            for (const auto identity:binding.weighted_states) weighted(identity);
        } else if (current.kind==Kind::graph) {
            const auto& source=source_graphs[current.index];
            Graph result;
            result.rows.reserve(source.entries.size()+1);
            for (std::size_t i=0;i<source.entries.size();++i) {
                const auto& entry=source.entries[i];
                IsleActorInteraction row{entry.command,nullptr,entry.value,nullptr};
                if (executable_command(entry.command)) {
                    if (entry.state_identity) row.state=callback(entry.state_identity,source.wire.guest_address,i);
                    if (entry.aux_identity) row.aux=callback(entry.aux_identity,source.wire.guest_address,i);
                }
                result.rows.push_back(row);
            }
            result.rows.push_back({58,nullptr,0,nullptr});
            result.view={result.rows.data(),result.rows.size(),source.wire.guest_address};
            graphs_.push_back(std::move(result));
        } else {
            const auto& source=source_weighted[current.index];
            Weighted result;
            result.rows.reserve(source.entries.size()+1);
            for (std::size_t i=0;i<source.entries.size();++i) {
                const auto& entry=source.entries[i];
                result.rows.push_back({entry.weight,
                    callback(entry.state_identity,source.wire.guest_address,i)});
            }
            result.rows.push_back({0,nullptr});
            result.view={source.wire.guest_address,result.rows,source.total_weight};
            weighted_.push_back(std::move(result));
        }
    }
    std::sort(callbacks_.begin(),callbacks_.end(),[](const auto& a,const auto& b) {
        return a.identity<b.identity;
    });
    // Preserve the declared callable-data dependencies after the caller's
    // temporary binding spans expire. Admission later performs no graph walk
    // allocation, including for cycles and local mutable graph roots.
    for (auto& root:callbacks_) {
        std::vector<std::size_t> pending;
        std::vector<unsigned char> seen(callbacks_.size());
        auto enqueue=[&](std::uint32_t identity) {
            const auto found=std::lower_bound(callbacks_.begin(),callbacks_.end(),identity,
                [](const auto& candidate,std::uint32_t value) { return candidate.identity<value; });
            if (found==callbacks_.end() || found->identity!=identity) unresolved(identity,root.identity,0);
            const auto index=static_cast<std::size_t>(found-callbacks_.begin());
            if (!seen[index]) { seen[index]=1; pending.push_back(index); }
        };
        enqueue(root.identity);
        for (std::size_t cursor=0;cursor<pending.size();++cursor) {
            const auto& current=callbacks_[pending[cursor]];
            if (std::find(root.closure.begin(),root.closure.end(),current.function)==root.closure.end())
                root.closure.push_back(current.function);
            const auto found=std::lower_bound(registry.begin(),registry.end(),current.identity,
                [](const auto* candidate,std::uint32_t value) { return candidate->identity<value; });
            const auto& binding=**found; // Earlier closure construction resolved this identity.
            for (const auto identity:binding.callbacks)
                if (!course_typed_dependency(resources.course_id(),identity)) enqueue(identity);
            for (const auto identity:binding.graphs) {
                const auto& dependency=resources.graph(identity);
                for (const auto& row:dependency.entries) if (executable_command(row.command)) {
                    if (row.state_identity) enqueue(row.state_identity);
                    if (row.aux_identity) enqueue(row.aux_identity);
                }
            }
            for (const auto identity:binding.weighted_states)
                for (const auto& row:resources.weighted_states(identity).entries) enqueue(row.state_identity);
        }
    }
}
void BehaviorBindings::require_live() const { resources_->require_live(); }
const IsleActorInteractionGraph& BehaviorBindings::graph(std::uint32_t identity) const {
    require_live();
    const auto found=std::find_if(graphs_.begin(),graphs_.end(),[&](const auto& graph) {
        return graph.view.source_identity==identity;
    });
    if (found==graphs_.end()) throw std::out_of_range("Graph is outside admitted native callable closure");
    return found->view;
}
const BoundWeightedStates& BehaviorBindings::weighted_states(std::uint32_t identity) const {
    require_live();
    const auto found=std::find_if(weighted_.begin(),weighted_.end(),[&](const auto& list) {
        return list.view.source_identity==identity;
    });
    if (found==weighted_.end()) throw std::out_of_range("Weighted list is outside admitted native callable closure");
    return found->view;
}
const BoundCountedStates& BehaviorBindings::counted_state_options(std::uint32_t identity) const {
    require_live();
    for (const auto& list : counted_) if (list.view.source_identity==identity) return list.view;
    throw std::out_of_range("Counted options are outside admitted native callable closure");
}
IsleGameSourceCallback BehaviorBindings::callback(std::uint32_t identity) const {
    require_live();
    const auto found=std::lower_bound(callbacks_.begin(),callbacks_.end(),identity,
        [](const auto& candidate,std::uint32_t value) { return candidate.identity<value; });
    if (found==callbacks_.end() || found->identity!=identity)
        throw std::out_of_range("Callback is outside admitted native callable closure");
    return found->function;
}
bool BehaviorBindings::contains(IsleGameSourceCallback function) const {
    require_live();
    return function && std::any_of(callbacks_.begin(),callbacks_.end(),[&](const auto& callback) {
        return callback.function==function;
    });
}
bool BehaviorBindings::admits_native_graph(const IsleActorInteractionGraph& graph,
    bool (*admit)(IsleGameSourceCallback,void*),void* context) const {
    require_live();
    if (!graph.rows || !graph.count || !admit) return false;
    auto callback=[&](IsleGameSourceCallback function) {
        if (!function) return true;
        bool found=false;
        for (const auto& candidate:callbacks_) if (candidate.function==function) {
            found=true;
            for (const auto dependency:candidate.closure) if (!admit(dependency,context)) return false;
        }
        return found;
    };
    for (std::size_t i=0;i<graph.count;++i) {
        const auto& row=graph.rows[i];
        if (row.cmd==58) return true;
        if (executable_command(row.cmd) && (!callback(row.state) || !callback(row.aux))) return false;
    }
    return false;
}
} // namespace isle::actor
