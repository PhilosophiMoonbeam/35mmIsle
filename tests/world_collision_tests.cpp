#include "world_collision.hpp"
#include <cmath>
#include <iostream>

namespace {
using namespace isle::world;
namespace col = isle::world::collision;
int failures = 0;
void check(bool ok, const char* description) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}
bool near(float a, float b) { return std::fabs(a-b)<0.001f; }
Collider primitive(unsigned type, Vec3 parameters) {
    Collider c; c.has_hitbox=true; c.hitbox.type=static_cast<std::uint8_t>(type);
    c.hitbox.parameters=parameters; return c;
}
}
int main() {
    Vec3 p{},v{};
    auto box=primitive(4,{1,1,1});
    check(col::collision_primitive(box,{-2,0,0},{3,0,0},p,v)&&near(p.x,-1)&&near(v.x,-2.1f),"box entering face and 0.7 reflection");
    check(!col::collision_primitive(box,{-2,0,0},{1,0,0},p,v),"box endpoint equality is not collision");
    check(!col::collision_primitive(box,{-1,0,0},{3,0,0},p,v),"box time-zero boundary is rejected");
    check(!col::collision_primitive(box,{-2,2,0},{3,0,0},p,v),"box stationary outside slab misses");
    box.scale={2,3,4};box.position={5,0,0};
    check(col::collision_primitive(box,{1,0,0},{6,0,0},p,v)&&near(p.x,3)&&near(v.x,-4.2f),"nonuniform collider transform preserves reflected speed coefficient");
    box=primitive(4,{1,1,1});box.rotation.z=1.5707963267948966f;box.position={0,5,0};
    check(col::collision_primitive(box,{0,2,0},{0,4,0},p,v)&&near(p.y,4)&&near(v.y,-2.8f),"rotation and inverse translation use row-vector source order");
    auto sphere=primitive(1,{1,0,0});
    check(!col::collision_primitive(sphere,{-2,0,0},{3,0,0},p,v),"pinned sphere rejects outside starts (not generic sphere sweep)");
    check(col::collision_primitive(sphere,{-.5f,0,0},{1,0,0},p,v)&&near(p.x,-1.5f)&&near(v.x,-.7f),"pinned sphere inside-start and twelve-unit retreat");
    auto cylinder=primitive(3,{1,2,0});
    check(col::collision_primitive(cylinder,{-2,0,0},{3,0,0},p,v)&&near(p.x,-1)&&near(v.x,-2.1f),"cylinder side reflection");
    check(!col::collision_primitive(cylinder,{-2,2,0},{3,0,0},p,v),"cylinder height bounds reject side intersection");
    check(!col::collision_primitive(cylinder,{0,2,0},{0,-3,0},p,v),"source cylinder has no end-cap collision");
    auto cone=primitive(2,{99,4,0});
    check(col::collision_primitive(cone,{-2,1,0},{3,0,0},p,v)&&near(p.x,-1)&&near(v.x,-2.1f),"source cone ignores radius and reflects horizontally");
    check(!col::collision_primitive(cone,{-2,-1,0},{3,0,0},p,v),"cone negative height rejects");
    CollisionModel model;model.id=7;model.scale=1;
    auto root=primitive(1,{10,0,0});root.depth=0;
    box=primitive(4,{1,1,1});box.depth=1;model.colliders={root,box};
    StaticObject object;object.id=7;object.position={5,0,0};
    check(col::collision_one(model,object,{300,0,0},{300,0,0},p,v)&&near(p.x,400)&&near(v.x,-210),"instance position map units and input/output global100 conversion");
    model.scale=0;object.scale={2,1,1};
    check(col::collision_one(model,object,{100,0,0},{600,0,0},p,v)&&near(p.x,300)&&near(v.x,-420),"nonpositive model scale selects object scale");
    model.scale=1;model.colliders={root};
    check(!col::collision_one(model,object,{450,0,0},{100,0,0},p,v),"root sphere is broad phase only");
    model.colliders={root,box};
    check(!col::collision_one(model,object,{1500,0,0},{-2000,0,0},p,v),"root broad phase uses strict start distance, not swept segment");
    // One preparation is reused for differently translated object instances.
    // Include nonuniform scale, rotation, and a second resource-model index.
    box.position={.25f,0,0};box.rotation.z=.3f;box.scale={2,3,4};
    model.colliders={root,box};
    std::vector<CollisionModel> models{model,model};
    models[1].scale=0;
    const col::PreparedModels prepared(models);
    for(std::size_t index=0;index<models.size();++index) {
        for(float shift:{0.0f,5.0f,-7.0f}) {
            object.position={shift,0,0};object.rotation={0,0,0};object.scale={2,1,1};
            const Vec3 input{(shift-5)*100,0,0},velocity{1000,0,0};
            Vec3 expected_p{},expected_v{},actual_p{},actual_v{};
            bool expected=col::collision_one(models[index],object,input,velocity,expected_p,expected_v);
            bool actual=prepared.collision_one(index,object,input,velocity,actual_p,actual_v);
            check(expected&&actual,"reused prepared model retains transformed collider hit");
            check(near(actual_p.x,expected_p.x)&&near(actual_p.y,expected_p.y)&&near(actual_p.z,expected_p.z)&&
                  near(actual_v.x,expected_v.x)&&near(actual_v.y,expected_v.y)&&near(actual_v.z,expected_v.z),
                  "prepared collider matrices match pure helper across repeated instances");
        }
    }
    check(!prepared.collision_one(models.size(),object,{},{},p,v),"prepared model rejects absent index");
    HeightMap map;
    map.patches={{1,2,2,-8,0x12345678},{0,0,1,-3,0xabcdef00}};
    HeightNode node{};node.a=1;node.b=0;node.c=0;node.left_patch=0;node.right_patch=1;
    map.nodes={node};col::GroundResult ground;
    check(col::ground_at_global(map,0,0,ground)&&near(ground.height,3)&&ground.surface==0xabcdef,"height split equality follows right branch and surface shifts eight");
    check(col::ground_at_global(map,2,1,ground)&&near(ground.height,2)&&near(ground.normal.x,1.0f/3)&&near(ground.normal.y,2.0f/3)&&near(ground.normal.z,2.0f/3),"height plane uses C as up and normal swaps axes");
    check(col::ground_at(map,{0,1,0},0,0,ground)&&near(ground.height,200),"local height subtracts block elevation before times100");
    map.patches[1].c=0;
    check(col::ground_at_global(map,0,0,ground)&&near(ground.height,0),"zero planeC preserves source zero height");
    HeightNode loop{};loop.a=1;loop.left_patch=0;loop.right_patch=no_index;loop.right_child=0;
    map.nodes={loop};
    check(!col::ground_at_global(map,0,0,ground)&&ground.height==-9000&&ground.surface==-1,
          "consumed height-node self-cycle rejects and restores source fallback");
    check(col::ground_at_global(map,2,1,ground)&&near(ground.height,2),
          "unvisited cyclic sibling does not reject an actual terminal patch");
    map.nodes.push_back(loop);map.nodes[0].right_child=1;map.nodes[1].right_child=0;
    check(!col::ground_at_global(map,0,0,ground)&&ground.height==-9000&&ground.surface==-1,
          "consumed multi-node cycle rejects within the finite native node extent");
    map.nodes.clear();
    check(!col::ground_at_global(map,0,0,ground)&&near(ground.height,-9000)&&ground.surface==-1,"absent height map resets default result");
    return failures?1:0;
}
