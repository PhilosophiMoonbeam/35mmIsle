#include "beach_initial_species.hpp"
#include "beach_initial_species_source.h"
#include "beach_ending_source.h"
#include <array>
namespace isle::beach {
namespace {
constexpr std::uint32_t ending_initial[] = {0x802ca474};
constexpr std::uint32_t ending_graph[] = {0x802cd9d8};
constexpr std::uint32_t ending_trigger[] = {0x802ca950};
constexpr std::uint32_t ending_outro[] = {0x802ca5dc, 0x802ca8c4};
constexpr std::uint32_t butterfly_initial[] = {0x802c7bb8, 0x802c7c7c};
constexpr std::uint32_t butterfly_wait[] = {0x802c7bb8};
constexpr std::uint32_t butterfly_hit[] = {0x802c7da0, 0x802c7dec, 0x802c7bb8};
constexpr std::uint32_t butterfly_graph[] = {0x802ccd60};
constexpr std::uint32_t doduo_initial[] = {0x802c8438};
constexpr std::uint32_t doduo_resume[] = {0x802c83d0};
constexpr std::uint32_t doduo_wait_graph[] = {0x802ccf44};
constexpr std::uint32_t doduo_run_graph[] = {0x802ccf14};
constexpr std::uint32_t entry_802c7e70_callbacks[] = {0x802c816c, 0x802c81c4, 0x802c821c, 0x802c7f1c};
constexpr std::uint32_t entry_802c7f1c_callbacks[] = {0x802c7f74};
constexpr std::uint32_t entry_802c7f1c_graphs[] = {0x802cce1c};
constexpr std::uint32_t entry_802c7f74_callbacks[] = {0x802c7f74};
constexpr std::uint32_t entry_802c8028_callbacks[] = {0x802c80bc, 0x802c80f0, 0x802c8124};
constexpr std::uint32_t entry_802c816c_callbacks[] = {0x802c7f1c};
constexpr std::uint32_t entry_802c81c4_callbacks[] = {0x802c7f1c};
constexpr std::uint32_t entry_802c821c_callbacks[] = {0x802c7f1c};
constexpr std::uint32_t entry_802c85b0_callbacks[] = {0x802c85e0};
constexpr std::uint32_t entry_802c85e0_callbacks[] = {0x802c87bc, 0x802c8c7c, 0x802c8cfc};
constexpr std::uint32_t entry_802c85e0_graphs[] = {0x802cd040};
constexpr std::uint32_t entry_802c86a8_callbacks[] = {0x802c8980, 0x802c8828};
constexpr std::uint32_t entry_802c86a8_graphs[] = {0x802cd010};
constexpr std::uint32_t entry_802c8828_callbacks[] = {0x802c8894};
constexpr std::uint32_t entry_802c8828_graphs[] = {0x802cd010};
constexpr std::uint32_t entry_802c8980_callbacks[] = {0x802c89a4};
constexpr std::uint32_t entry_802c89a4_callbacks[] = {0x802c8bc4, 0x802c8828};
constexpr std::uint32_t entry_802c89a4_graphs[] = {0x802cd064};
constexpr std::uint32_t entry_802c8aac_callbacks[] = {0x802c8b60};
constexpr std::uint32_t entry_802c8c7c_graphs[] = {0x802cd0b4};
constexpr std::uint32_t entry_802c8cc0_callbacks[] = {0x802c86a8};
constexpr std::uint32_t entry_802c8cfc_graphs[] = {0x802cd0e4};
constexpr std::uint32_t entry_802cac80_graphs[] = {0x802cdc60};
constexpr std::uint32_t entry_802cacd4_callbacks[] = {0x802cad58, 0x802cb2d8, 0x802cac80};
constexpr std::uint32_t entry_802cad58_callbacks[] = {0x802cad7c};
constexpr std::uint32_t entry_802cad7c_graphs[] = {0x802cdc80};
constexpr std::uint32_t entry_802cadcc_callbacks[] = {0x802caeb0, 0x802cae14};
constexpr std::uint32_t entry_802cae14_callbacks[] = {0x802caf90};
constexpr std::uint32_t entry_802cae14_graphs[] = {0x802cdca0};
constexpr std::uint32_t entry_802caf1c_callbacks[] = {0x802cae14};
constexpr std::uint32_t entry_802caf90_callbacks[] = {0x802caf90};
constexpr std::uint32_t entry_802caf90_graphs[] = {0x802cdcd0};
constexpr std::uint32_t entry_802cafec_callbacks[] = {0x802caf90};
constexpr std::uint32_t entry_802cb054_callbacks[] = {0x802cb128, 0x802cb08c};
constexpr std::uint32_t entry_802cb08c_callbacks[] = {0x802cb08c, 0x802caf90};
constexpr std::uint32_t entry_802cb08c_graphs[] = {0x802cdd40};
constexpr std::uint32_t entry_802cb2d8_callbacks[] = {0x802cb814};
constexpr std::uint32_t entry_802cb2d8_graphs[] = {0x802cdd80};
constexpr std::uint32_t entry_802cb2d8_weighted[] = {0x802cde30};
constexpr std::uint32_t entry_802cb36c_graphs[] = {0x802cdd80};
constexpr std::uint32_t entry_802cb36c_weighted[] = {0x802cde30};
constexpr std::uint32_t entry_802cb3dc_graphs[] = {0x802cdd80};
constexpr std::uint32_t entry_802cb3dc_weighted[] = {0x802cde30};
constexpr std::uint32_t entry_802cb44c_callbacks[] = {0x802cb3dc};
constexpr std::uint32_t entry_802cb44c_graphs[] = {0x802cde48};
constexpr std::uint32_t entry_802cb520_callbacks[] = {0x802cb6b4, 0x802cb594};
constexpr std::uint32_t entry_802cb520_graphs[] = {0x802cde78};
constexpr std::uint32_t entry_802cb594_callbacks[] = {0x802cb710, 0x802cb36c, 0x802cb3dc};
constexpr std::uint32_t entry_802cb594_graphs[] = {0x802cdef8};
constexpr std::uint32_t entry_802cb784_callbacks[] = {0x802cb128, 0x802cb784, 0x802cb3dc};
constexpr std::uint32_t entry_802cb874_callbacks[] = {0x802cbb34, 0x802cba48, 0x802cbb90};
constexpr std::uint32_t entry_802cb874_graphs[] = {0x802cdf98, 0x802cdfd8};
constexpr std::uint32_t entry_802cb9e0_callbacks[] = {0x802cb874};
constexpr std::uint32_t entry_802cbb90_callbacks[] = {0x802cbb90};
constexpr std::uint32_t entry_802cbb90_graphs[] = {0x802cdfd8};
constexpr std::uint32_t entry_802cbc4c_callbacks[] = {0x802cbb90};
constexpr std::uint32_t entry_802cbc4c_graphs[] = {0x802cdfd8};
const actor::BehaviorCallbackBinding callbacks[] = {
    {0x802c7b10, func_beach_802C7B10, butterfly_initial, {}, {}},
    {0x802c7bb8, func_beach_802C7BB8, butterfly_wait, butterfly_graph, {}},
    {0x802c7c7c, func_beach_802C7C7C, {}, {}, {}},
    {0x802c7cf0, func_beach_802C7CF0, butterfly_hit, butterfly_graph, {}},
    {0x802c7da0, func_beach_802C7DA0, {}, {}, {}},
    {0x802c7dec, func_beach_802C7DEC, {}, {}, {}},
    {0x802c82b0, func_beach_802C82B0, doduo_initial, doduo_wait_graph, {}},
    {0x802c8354, func_beach_802C8354, doduo_resume, doduo_run_graph, {}},
    {0x802c83d0, func_beach_802C83D0, {}, doduo_run_graph, {}},
    {0x802c8438, func_beach_802C8438, {}, {}, {}},
    {0x802c84a4, func_beach_802C84A4, doduo_resume, {}, {}},
    {0x802c7e70, func_beach_802C7E70, entry_802c7e70_callbacks, {}, {}},
    {0x802c7f1c, func_beach_802C7F1C, entry_802c7f1c_callbacks, entry_802c7f1c_graphs, {}},
    {0x802c7f74, func_beach_802C7F74, entry_802c7f74_callbacks, {}, {}},
    {0x802c8028, func_beach_802C8028, entry_802c8028_callbacks, {}, {}},
    {0x802c80bc, func_beach_802C80BC, {}, {}, {}},
    {0x802c80f0, func_beach_802C80F0, {}, {}, {}},
    {0x802c8124, func_beach_802C8124, {}, {}, {}},
    {0x802c816c, func_beach_802C816C, entry_802c816c_callbacks, {}, {}},
    {0x802c81c4, func_beach_802C81C4, entry_802c81c4_callbacks, {}, {}},
    {0x802c821c, func_beach_802C821C, entry_802c821c_callbacks, {}, {}},
    {0x802c85b0, func_beach_802C85B0, entry_802c85b0_callbacks, {}, {}},
    {0x802c85e0, func_beach_802C85E0, entry_802c85e0_callbacks, entry_802c85e0_graphs, {}},
    {0x802c86a8, func_beach_802C86A8, entry_802c86a8_callbacks, entry_802c86a8_graphs, {}},
    {0x802c87bc, func_beach_802C87BC, {}, {}, {}},
    {0x802c8828, func_beach_802C8828, entry_802c8828_callbacks, entry_802c8828_graphs, {}},
    {0x802c8894, func_beach_802C8894, {}, {}, {}},
    {0x802c8900, func_beach_802C8900, {}, {}, {}},
    {0x802c8980, func_beach_802C8980, entry_802c8980_callbacks, {}, {}},
    {0x802c89a4, func_beach_802C89A4, entry_802c89a4_callbacks, entry_802c89a4_graphs, {}},
    {0x802c8aac, func_beach_802C8AAC, entry_802c8aac_callbacks, {}, {}},
    {0x802c8b60, func_beach_802C8B60, {}, {}, {}},
    {0x802c8bc4, func_beach_802C8BC4, {}, {}, {}},
    {0x802c8c30, func_beach_802C8C30, {}, {}, {}},
    {0x802c8c58, func_beach_802C8C58, {}, {}, {}},
    {0x802c8c7c, func_beach_802C8C7C, {}, entry_802c8c7c_graphs, {}},
    {0x802c8cc0, func_beach_802C8CC0, entry_802c8cc0_callbacks, {}, {}},
    {0x802c8cfc, func_beach_802C8CFC, {}, entry_802c8cfc_graphs, {}},
    {0x802cac80, func_beach_802CAC80, {}, entry_802cac80_graphs, {}},
    {0x802cacd4, func_beach_802CACD4, entry_802cacd4_callbacks, {}, {}},
    {0x802cad58, func_beach_802CAD58, entry_802cad58_callbacks, {}, {}},
    {0x802cad7c, func_beach_802CAD7C, {}, entry_802cad7c_graphs, {}},
    {0x802cadcc, func_beach_802CADCC, entry_802cadcc_callbacks, {}, {}},
    {0x802cae14, func_beach_802CAE14, entry_802cae14_callbacks, entry_802cae14_graphs, {}},
    {0x802caeb0, func_beach_802CAEB0, {}, {}, {}},
    {0x802caf1c, func_beach_802CAF1C, entry_802caf1c_callbacks, {}, {}},
    {0x802caf90, func_beach_802CAF90, entry_802caf90_callbacks, entry_802caf90_graphs, {}},
    {0x802cafec, func_beach_802CAFEC, entry_802cafec_callbacks, {}, {}},
    {0x802cb054, func_beach_802CB054, entry_802cb054_callbacks, {}, {}},
    {0x802cb08c, func_beach_802CB08C, entry_802cb08c_callbacks, entry_802cb08c_graphs, {}},
    {0x802cb128, func_beach_802CB128, {}, {}, {}},
    {0x802cb2d8, func_beach_802CB2D8, entry_802cb2d8_callbacks, entry_802cb2d8_graphs, entry_802cb2d8_weighted},
    {0x802cb36c, func_beach_802CB36C, {}, entry_802cb36c_graphs, entry_802cb36c_weighted},
    {0x802cb3dc, func_beach_802CB3DC, {}, entry_802cb3dc_graphs, entry_802cb3dc_weighted},
    {0x802cb44c, func_beach_802CB44C, entry_802cb44c_callbacks, entry_802cb44c_graphs, {}},
    {0x802cb520, func_beach_802CB520, entry_802cb520_callbacks, entry_802cb520_graphs, {}},
    {0x802cb594, func_beach_802CB594, entry_802cb594_callbacks, entry_802cb594_graphs, {}},
    {0x802cb6b4, func_beach_802CB6B4, {}, {}, {}},
    {0x802cb710, func_beach_802CB710, {}, {}, {}},
    {0x802cb784, func_beach_802CB784, entry_802cb784_callbacks, {}, {}},
    {0x802cb814, func_beach_802CB814, {}, {}, {}},
    {0x802cb874, func_beach_802CB874, entry_802cb874_callbacks, entry_802cb874_graphs, {}},
    {0x802cb9e0, func_beach_802CB9E0, entry_802cb9e0_callbacks, {}, {}},
    {0x802cba48, func_beach_802CBA48, {}, {}, {}},
    {0x802cbb34, func_beach_802CBB34, {}, {}, {}},
    {0x802cbb90, func_beach_802CBB90, entry_802cbb90_callbacks, entry_802cbb90_graphs, {}},
    {0x802cbc4c, func_beach_802CBC4C, entry_802cbc4c_callbacks, entry_802cbc4c_graphs, {}},
    {0x802ca300, func_beach_802CA300, {}, {}, {}},
    {0x802ca450, func_beach_802CA450, ending_initial, {}, {}},
    {0x802ca474, func_beach_802CA474, {}, ending_graph, {}},
    {0x802ca4cc, func_beach_802CA4CC, ending_trigger, {}, {}},
    {0x802ca5dc, func_beach_802CA5DC, {}, {}, {}},
    {0x802ca8c4, func_beach_802CA8C4, {}, {}, {}},
    {0x802ca950, func_beach_802CA950, ending_outro, {}, {}},
};
constexpr std::uint32_t graphs[] = {0x802ccd60, 0x802ccf14, 0x802ccf44, 0x802cce1c, 0x802cd040, 0x802cd010, 0x802cd064, 0x802cd0b4, 0x802cd0e4, 0x802cdc60, 0x802cdc80, 0x802cdca0, 0x802cdcd0, 0x802cdd40, 0x802cdd80, 0x802cde48, 0x802cde78, 0x802cdef8, 0x802cdf98, 0x802cdfd8, 0x802cd9d8};
constexpr std::uint32_t roots[] = {0x802c7b10, 0x802c82b0, 0x802c7e70, 0x802c85b0, 0x802cacd4, 0x802ca300, 0x802ca450};
}
std::span<const actor::BehaviorCallbackBinding> initial_species_callbacks() { return callbacks; }
std::span<const std::uint32_t> initial_species_graphs() { return graphs; }
std::span<const std::uint32_t> initial_species_roots() { return roots; }
}
