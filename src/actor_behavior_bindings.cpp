#include "actor_behavior_bindings.hpp"

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
} // namespace

BehaviorBindings::BehaviorBindings(const beach::BehaviorResources& resources,
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
    registry.reserve(bindings.size());
    for (const auto& binding:bindings) {
        if (!binding.identity || !binding.function)
            throw std::invalid_argument("Native callback binding requires nonzero identity and typed function");
        registry.push_back(&binding);
    }
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
            callbacks_.push_back({identity,(*found)->function});
            work.push_back({Kind::callback,index});
        }
        return (*found)->function;
    };
    for (const auto identity:graph_heads) graph(identity);
    for (const auto identity:weighted_heads) weighted(identity);
    for (const auto identity:callback_heads) callback(identity,0,0);
    for (std::size_t cursor=0;cursor<work.size();++cursor) {
        const auto current=work[cursor];
        if (current.kind==Kind::callback) {
            const auto& binding=*registry[current.index];
            for (const auto identity:binding.callbacks) callback(identity,binding.identity,0);
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
} // namespace isle::actor
