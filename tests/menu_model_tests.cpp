#include "menu_model_producer.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace isle::menu;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Input {
    std::array<std::byte,0x78> wire{};
    Material material;
    animation::MaterialState state;
    Input() {
        material.wire = {0x80300000,0,wire};
        material.images = {0x80310000,0x80311000,0x80312000};
        material.palettes = {0x80320000,0x80320200};
        material.format = 0; material.size_code = 2;
        material.auxiliary_format = 0; material.auxiliary_size_code = 2;
        material.width = material.block_width = 32;
        material.height = material.block_height = 16;
        material.scale = 32;
        state.parameters[2] = state.parameters[3] = 1;
    }
};
void defaults_and_scroll() {
    Input input;
    model::MaterialCommands commands;
    require(bool(model::encode_material(input.material,input.state,commands)),"default material rejected");
    require(commands.count == 4 && commands.words[0].w0 == 0xfd100000 &&
        commands.words[0].w1 == input.material.images[0] && commands.words[1].w0 == 0xf2000000 &&
        commands.words[1].w1 == ((124u << 12) | 60) && commands.words[2].w1 == 0xffffffff,
        "flags0 did not apply genuine A1 image/tile/texture defaults");
    input.material.width = 16; input.material.height = 8;
    input.state.parameters[0] = 1; input.state.parameters[1] = 0.25f;
    input.state.parameters[2] = 2; input.state.parameters[3] = 0.5f;
    require(bool(model::encode_material(input.material,input.state,commands)),"scroll rejected");
    require(commands.words[1].w0 == (0xf2000000u | (32u << 12) | 16u) &&
        commands.words[1].w1 == ((92u << 12) | 44u) && commands.words[2].w1 == 0x8000ffff,
        "animated tile scroll/scale lost source grouping or upper saturation");
}
void palette_and_light_colors() {
    Input input;
    input.material.flags = 0x3007;
    input.state.palette_index = 1.9f; input.state.next_image_index = 2;
    input.state.color_and_trailing_words[4] = 0x12345678;
    input.state.color_and_trailing_words[5] = 0x90abcdef;
    model::MaterialCommands commands;
    require(bool(model::encode_material(input.material,input.state,commands)),"palette material rejected");
    require(commands.words[0].w1 == input.material.palettes[1] &&
        commands.words[4].w0 == 0xf0000000 && commands.words[4].w1 == 0x0503c000,
        "palette float truncation or16-entryTLUT wire changed");
    require(commands.words[6].w0 == 0xdb0a0000 && commands.words[7].w0 == 0xdb0a0004 &&
        commands.words[6].w1 == 0x12345678 && commands.words[7].w1 == 0x12345678 &&
        commands.words[8].w0 == 0xdb0a0018 && commands.words[9].w0 == 0xdb0a001c &&
        commands.words[9].w1 == 0x90abcdef,"light colors did not update BOTH public light color words");
    require(commands.words[10].w1 == input.material.images[2],
        "auxiliary load selected the wrong animated image");
    // Public RDP wire consumer: the caller's auxiliary block descriptor owns
    // the upper TMEM half; the primary block descriptor owns the lower half.
    // A LOADBLOCK must resolve through that descriptor, not a presumed tile7.
    std::array<unsigned,8> descriptor_tmem{};
    std::array<unsigned,512> loaded_texels{};
    const std::array<model::Command,2> descriptors{{
        {0xf5100100,0x06000000}, {0xf5100000,0x07000000}}};
    auto consume = [&](model::Command command) {
        const auto opcode = command.w0 >> 24;
        const auto tile = (command.w1 >> 24) & 7u;
        if (opcode == 0xf5) descriptor_tmem[tile] = command.w0 & 511u;
        if (opcode == 0xf3) loaded_texels[descriptor_tmem[tile]] +=
            ((command.w1 >> 12) & 4095u) + 1;
    };
    for (const auto command : descriptors) consume(command);
    for (std::size_t i = 0; i < commands.count; ++i) consume(commands.words[i]);
    require(loaded_texels[0x100] == 32 * 16 && loaded_texels[0] == 0,
        "auxiliary LOADBLOCK did not target its caller-selected upper-TMEM descriptor");
}
void lod_and_atomic_rejection() {
    Input input;
    input.material.flags = 0xe10;
    input.state.lod_level = 1.25f;
    input.state.color_and_trailing_words[0] = 0xabcdef01;
    input.state.color_and_trailing_words[1] = 0x00330000;
    input.state.color_and_trailing_words[2] = 0x23456789;
    input.state.color_and_trailing_words[3] = 0x11223344;
    model::MaterialCommands commands;
    require(bool(model::encode_material(input.material,input.state,commands)),"lod material rejected");
    require(commands.words[0].w0 == 0xfa003340 && commands.words[0].w1 == 0xabcdef01 &&
        commands.words[1].w0 == 0xfb000000 && commands.words[2].w0 == 0xf9000000 &&
        commands.words[3].w1 == input.material.images[2] &&
        commands.words[7].w1 == input.material.images[1],"lod blend must select floor and floor+1, not last/static frame");
    const auto saved = commands;
    input.state.lod_level = 2.0f;
    require(!model::encode_material(input.material,input.state,commands),"out-of-range nextLOD frame admitted");
    require(commands.count == saved.count,"rejection changed command extent");
    for (std::size_t i = 0; i < commands.words.size(); ++i)
        require(commands.words[i].w0 == saved.words[i].w0 && commands.words[i].w1 == saved.words[i].w1,
            "rejection partially published command body");
    input.material.flags = 4; input.state.palette_index = -1;
    require(!model::encode_material(input.material,input.state,commands),"negative palette index admitted");
    input.material.flags = 0; input.state.parameters[2] = std::numeric_limits<float>::infinity();
    require(!model::encode_material(input.material,input.state,commands),"nonfinite scale admitted");
}
void fractional_lod_above_terminal_key() {
    Input input;
    input.material.images = {0x80310000,0x80311000,0x80312000,0x80313000,0x80314000};
    input.material.flags = 0x10;
    input.state.lod_level = 3.25f;
    model::MaterialCommands commands;
    // A Hermite track may exceed its terminal key3 while staying below4.
    // The five-entry table admits this fractional blend of images3 and4.
    require(bool(model::encode_material(input.material,input.state,commands)),
        "legitimate fractional LOD above terminal key was rejected");
    require(commands.words[0].w0 == 0xfa000040 &&
        commands.words[1].w1 == input.material.images[4] &&
        commands.words[5].w1 == input.material.images[3],
        "fractional LOD was clamped to the terminal key or selected the wrong frame pair");
}
}
int main() {
    try {
        defaults_and_scroll(); palette_and_light_colors();
        lod_and_atomic_rejection(); fractional_lod_above_terminal_key();
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    return 0;
}
