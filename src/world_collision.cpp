#include "world_collision.hpp"
#include <cmath>
#include <cstring>
#include <limits>

namespace isle::world::collision {
namespace {
using Vec3f = Vec3;
using f32 = float;
using s32 = int;
using Mtx4f = float[4][4];
constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
constexpr float SQ(float x) { return x*x; }
float ABS(float x) { return std::fabs(x); }
float dot_product(const Vec3* a,const Vec3* b) { return a->x*b->x+a->y*b->y+a->z*b->z; }
float Vec3fNormalize(Vec3* v) { float n=std::sqrt(dot_product(v,v)); if(n!=0) { v->x/=n;v->y/=n;v->z/=n; } return n; }
void Vec3fScale(Vec3* v,float s) { v->x*=s;v->y*=s;v->z*=s; }
void Vec3fDiff(Vec3* o,const Vec3* a,const Vec3* b) { *o={a->x-b->x,a->y-b->y,a->z-b->z}; }
void Vec3fAdd(Vec3* a,const Vec3* b) { a->x+=b->x;a->y+=b->y;a->z+=b->z; }
void Vec3fDirection(Vec3* o,const Vec3* a,const Vec3* b) { Vec3fDiff(o,b,a);Vec3fNormalize(o); }
void Vec3fReflect(Vec3* v,const Vec3* n) { float d=2*dot_product(v,n); v->x-=d*n->x;v->y-=d*n->y;v->z-=d*n->z; }
// Arithmetic and branch order below follow GAME df8815e75f3d79618466e01e0827c6165b93d11c.
void transform_point(const Mtx4f arg0, Vec3f* arg1) {
    s32 i;
    f32 temp[3];

    for (i = 0; i < 3; i++) {
        temp[i] = arg1->x * arg0[0][i] + arg1->y * arg0[1][i] + arg1->z * arg0[2][i] + arg0[3][i];
    }

    arg1->x = temp[0];
    arg1->y = temp[1];
    arg1->z = temp[2];
}

void transform_point_zero_w(const Mtx4f arg0, Vec3f* arg1) {
    s32 i;
    f32 temp[3];

    for (i = 0; i < 3; i++) {
        temp[i] = arg1->x * arg0[0][i] + arg1->y * arg0[1][i] + arg1->z * arg0[2][i];
    }

    arg1->x = temp[0];
    arg1->y = temp[1];
    arg1->z = temp[2];
}


// identical to guMtxCatF
void multiply_matrix(Mtx4f arg0, Mtx4f arg1, Mtx4f arg2) {
    s32 i, j, k;
    Mtx4f temp;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            temp[i][j] = 0.0f;
            for (k = 0; k < 4; k++) {
                temp[i][j] += arg1[i][k] * arg2[k][j];
            }
        }
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            arg0[i][j] = temp[i][j];
        }
    }
}

void transpose_matrix(Mtx4f dst, Mtx4f src) {
    s32 i, j;
    Mtx4f temp;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            temp[i][j] = src[j][i];
        }
    }

    memcpy(dst, temp, sizeof(Mtx4f));
}

void create_identity_matrix(Mtx4f dst) { for(int i=0;i<4;++i) for(int j=0;j<4;++j) dst[i][j]=i==j?1.0f:0.0f; }
void create_matrix(Mtx4f mtx, Mtx4f invMtx, Vec3f scale, Vec3f rotation, Vec3f translation) {
    f32 sinAngleX = sinf(rotation.x);
    f32 cosAngleX = cosf(rotation.x);
    f32 sinAngleY = sinf(rotation.y);
    f32 cosAngleY = cosf(rotation.y);
    f32 sinAngleZ = sinf(rotation.z);
    f32 cosAngleZ = cosf(rotation.z);
    Mtx4f temp;

    create_identity_matrix(mtx);
    create_identity_matrix(invMtx);

    mtx[0][0] = scale.x;
    mtx[1][1] = scale.y;
    mtx[2][2] = scale.z;
    mtx[3][3] = 1.0f;

    invMtx[0][0] = 1.0f / scale.x;
    invMtx[1][1] = 1.0f / scale.y;
    invMtx[2][2] = 1.0f / scale.z;
    invMtx[3][3] = 1.0f;

    create_identity_matrix(temp);
    temp[2][1] = -sinAngleX;
    temp[1][2] = sinAngleX;
    temp[1][1] = cosAngleX;
    temp[2][2] = cosAngleX;
    multiply_matrix(mtx, mtx, temp);
    transpose_matrix(temp, temp);
    multiply_matrix(invMtx, temp, invMtx);

    create_identity_matrix(temp);
    temp[0][2] = -sinAngleY;
    temp[2][0] = sinAngleY;
    temp[0][0] = cosAngleY;
    temp[2][2] = cosAngleY;
    multiply_matrix(mtx, mtx, temp);
    transpose_matrix(temp, temp);
    multiply_matrix(invMtx, temp, invMtx);

    create_identity_matrix(temp);
    temp[1][0] = -sinAngleZ;
    temp[0][1] = sinAngleZ;
    temp[0][0] = cosAngleZ;
    temp[1][1] = cosAngleZ;
    multiply_matrix(mtx, mtx, temp);
    transpose_matrix(temp, temp);
    multiply_matrix(invMtx, temp, invMtx);

    create_identity_matrix(temp);
    temp[3][0] = translation.x;
    temp[3][1] = translation.y;
    temp[3][2] = translation.z;
    multiply_matrix(mtx, mtx, temp);

    create_identity_matrix(temp);
    temp[3][0] = -translation.x;
    temp[3][1] = -translation.y;
    temp[3][2] = -translation.z;
    multiply_matrix(invMtx, temp, invMtx);
}

void reflect_from_plane(Vec3f* pos, Vec3f* vel, f32 time, s32 dir, Vec3f* outPos, Vec3f* outVel) {
    Vec3f dirVec;

    outPos->x = vel->x * time + pos->x;
    outPos->y = vel->y * time + pos->y;
    outPos->z = vel->z * time + pos->z;

    switch (dir) {
        case 1:
            dirVec.x = -1;
            dirVec.y = 0;
            dirVec.z = 0;
            break;
        case 2:
            dirVec.x = 1;
            dirVec.y = 0;
            dirVec.z = 0;
            break;
        case 3:
            dirVec.x = 0;
            dirVec.y = -1;
            dirVec.z = 0;
            break;
        case 4:
            dirVec.x = 0;
            dirVec.y = 1;
            dirVec.z = 0;
            break;
        case 5:
            dirVec.x = 0;
            dirVec.y = 0;
            dirVec.z = -1;
            break;
        case 6:
            dirVec.x = 0;
            dirVec.y = 0;
            dirVec.z = 1;
            break;
    }

    outVel->x = vel->x;
    outVel->y = vel->y;
    outVel->z = vel->z;
    Vec3fReflect(outVel, &dirVec);
}

s32 collide_with_box(Vec3f* inPos, Vec3f* inVel, f32 x1, f32 x2, f32 y1, f32 y2, f32 z1, f32 z2, Vec3f* outPos, Vec3f* outVel) {
    Vec3f velDir;
    f32 speed;
    f32 time = 0.0f;
    f32 minTime = FLOAT_MAX;
    f32 timeToEdge;
    s32 dir = 0;

    outVel->x = inVel->x;
    outVel->y = inVel->y;
    outVel->z = inVel->z;

    velDir.x = outVel->x;
    velDir.y = outVel->y;
    velDir.z = outVel->z;

    speed = Vec3fNormalize(&velDir);

    if (velDir.x < -0.0001f) {
        timeToEdge = (x1 - inPos->x) / velDir.x;
        if (timeToEdge < time) {
            return false;
        }

        if (timeToEdge <= minTime) {
            minTime = timeToEdge;
        }

        timeToEdge = (x2 - inPos->x) / velDir.x;
        if (timeToEdge >= time) {
            dir = 2;
            if (timeToEdge > minTime) {
                return false;
            }
            time = timeToEdge;
        }
    } else if (velDir.x > 0.0001f) {
        timeToEdge = (x2 - inPos->x) / velDir.x;
        if (timeToEdge < time) {
            return false;
        }

        if (timeToEdge <= minTime) {
            minTime = timeToEdge;
        }

        timeToEdge = (x1 - inPos->x) / velDir.x;
        if (timeToEdge >= time) {
            dir = 1;
            if (timeToEdge > minTime) {
                return false;
            }
            time = timeToEdge;
        }
    } else if (inPos->x < x1 || inPos->x > x2) {
        return false;
    }

    if (velDir.y < -0.0001f) {
        timeToEdge = (y1 - inPos->y) / velDir.y;
        if (timeToEdge < time) {
            return false;
        }

        if (timeToEdge <= minTime) {
            minTime = timeToEdge;
        }

        timeToEdge = (y2 - inPos->y) / velDir.y;
        if (timeToEdge >= time) {
            dir = 4;
            if (timeToEdge > minTime) {
                return false;
            }
            time = timeToEdge;
        }
    } else if (velDir.y > 0.0001f) {
        timeToEdge = (y2 - inPos->y) / velDir.y;
        if (timeToEdge < time) {
            return false;
        }

        if (timeToEdge <= minTime) {
            minTime = timeToEdge;
        }

        timeToEdge = (y1 - inPos->y) / velDir.y;
        if (timeToEdge >= time) {
            dir = 3;
            if (timeToEdge > minTime) {
                return false;
            }
            time = timeToEdge;
        }
    } else if (inPos->y < y1 || inPos->y > y2) {
        return false;
    }

    if (velDir.z < -0.0001f) {
        timeToEdge = (z1 - inPos->z) / velDir.z;
        if (timeToEdge < time) {
            return false;
        }

        if (timeToEdge <= minTime) {
            minTime = timeToEdge;
        }

        timeToEdge = (z2 - inPos->z) / velDir.z;
        if (timeToEdge >= time) {
            dir = 6;
            if (timeToEdge > minTime) {
                return false;
            }
            time = timeToEdge;
        }
    } else if (velDir.z > 0.0001f) {
        timeToEdge = (z2 - inPos->z) / velDir.z;
        if (timeToEdge < time) {
            return false;
        }

        if (timeToEdge <= minTime) {
            minTime = timeToEdge;
        }

        timeToEdge = (z1 - inPos->z) / velDir.z;
        if (timeToEdge >= time) {
            dir = 5;
            if (timeToEdge > minTime) {
                return false;
            }
            time = timeToEdge;
        }
    } else if (inPos->z < z1 || inPos->z > z2) {
        return false;
    }

    if (time < 0.0001f) {
        return false;
    }

    if (dir > 0 && time < speed) {
        reflect_from_plane(inPos, &velDir, time, dir, outPos, outVel);
        speed *= 0.7f;
        Vec3fScale(outVel, speed);
        return true;
    } else {
        return false;
    }
}

s32 collide_with_sphere(Vec3f* inPos, Vec3f* inVel, Vec3f* center, f32 radius, Vec3f* outPos, Vec3f* outVel) {
    f32 distToCenterSquared;
    f32 speed;
    f32 distToClosestPoint;
    f32 outPathLenSquared;
    f32 dist;
    Vec3f vecToCenter;
    Vec3f normal;

    outVel->x = inVel->x;
    outVel->y = inVel->y;
    outVel->z = inVel->z;
    speed = Vec3fNormalize(outVel);
    Vec3fDiff(&vecToCenter, center, inPos);
    distToCenterSquared = dot_product(&vecToCenter, &vecToCenter);
    distToClosestPoint = dot_product(&vecToCenter, outVel);

    if (distToCenterSquared >= SQ(radius) || distToClosestPoint < 0.0001f) {
        return false;
    }

    outPathLenSquared = sqrtf(distToClosestPoint) + (SQ(radius) - distToCenterSquared); // BUG: SQ(distToClosestPoint) makes more sense
    if (outPathLenSquared > 0.0001f) {
        dist = distToClosestPoint - sqrtf(outPathLenSquared);
        if (dist < speed) {
            dist -= 12.0f;
            if (dist < 0.0f) {
                dist = 0.0f;
            }
            // move in the original direction
            outPos->x = inPos->x + dist * outVel->x;
            outPos->y = inPos->y + dist * outVel->y;
            outPos->z = inPos->z + dist * outVel->z;
            dist = speed - dist;
            // bounce
            Vec3fDirection(&normal, outPos, center);
            speed *= 0.7f;
            Vec3fReflect(outVel, &normal);
            // move in the new direction
            outPos->x += dist * outVel->x;
            outPos->y += dist * outVel->y;
            outPos->z += dist * outVel->z;
            Vec3fScale(outVel, speed);
            return true;
        }
    }
    return false;
}

void reflect_from_cylinder(Vec3f* pos, Vec3f* vel, f32 time, Vec3f* outPos, Vec3f* outVel) {
    Vec3f normal;

    outPos->x = pos->x + time * vel->x;
    outPos->y = pos->y + time * vel->y;
    outPos->z = pos->z + time * vel->z;

    normal.x = outPos->x;
    normal.y = 0.0f;
    normal.z = outPos->z;
    Vec3fNormalize(&normal);
    Vec3fReflect(outVel, &normal);
}

s32 collide_with_cylinder(Vec3f* inPos, Vec3f* inVel, Vec3f* center, f32 radius, f32 halfHeight, Vec3f* outPos, Vec3f* outVel) {
    Vec3f velDir;
    Vec3f vecFromCenter;
    f32 speed;
    f32 distToClosestPointSq;
    f32 unused1;
    f32 hSpeedSquared;
    f32 distToClosestPoint;
    f32 f12;
    f32 sp4C;
    f32 timeToClosestPoint;

    Vec3fDiff(&vecFromCenter, inPos, center);
    outVel->x = inVel->x;
    outVel->y = inVel->y;
    outVel->z = inVel->z;

    speed = Vec3fNormalize(outVel);
    velDir.x = outVel->x;
    velDir.y = outVel->y;
    velDir.z = outVel->z;

    hSpeedSquared = SQ(velDir.x) + SQ(velDir.z);
    if (hSpeedSquared > 0.0001f) {
        distToClosestPoint = vecFromCenter.x * velDir.x + vecFromCenter.z * velDir.z;
        distToClosestPointSq = SQ(distToClosestPoint);
        unused1 = SQ(vecFromCenter.x) + SQ(vecFromCenter.z) - SQ(radius);
        f12 = hSpeedSquared * unused1;
        sp4C = distToClosestPointSq - f12;
        if (f12 <= distToClosestPointSq) {
            timeToClosestPoint = (-distToClosestPoint - sqrtf(sp4C)) / hSpeedSquared;
            if (timeToClosestPoint > 0.0001f) { // Pinned source reads an uninitialized stack value here; no defined native equivalent.
                if (ABS(timeToClosestPoint * velDir.y + vecFromCenter.y) <= halfHeight && timeToClosestPoint < speed) {
                    reflect_from_cylinder(&vecFromCenter, &velDir, timeToClosestPoint, outPos, outVel);
                    speed *= 0.7f;
                    Vec3fScale(outVel, speed);
                    Vec3fAdd(outPos, center);
                    return true;
                }
            }
        }
    }
    return false;
}

void reflect_from_cylinder_copy(Vec3f* pos, Vec3f* vel, f32 time, Vec3f* outPos, Vec3f* outVel) {
    Vec3f normal;

    outPos->x = pos->x + time * vel->x;
    outPos->y = pos->y + time * vel->y;
    outPos->z = pos->z + time * vel->z;

    normal.x = outPos->x;
    normal.y = 0.0f;
    normal.z = outPos->z;
    Vec3fNormalize(&normal);
    Vec3fReflect(outVel, &normal);
}

s32 func_800E5AD4_63284(Vec3f* inPos, Vec3f* inVel, Vec3f* center, f32 arg3, f32 height, Vec3f* outPos, Vec3f* outVel) {
    Vec3f velDir;
    Vec3f vecFromCenter;
    f32 velSquaredMod;
    f32 dotProductMod;
    f32 distSquaredMod;
    f32 colPointY;
    f32 speed;
    f32 time;

    Vec3fDiff(&vecFromCenter, inPos, center);
    outVel->x = inVel->x;
    outVel->y = inVel->y;
    outVel->z = inVel->z;
    speed = Vec3fNormalize(outVel);
    velDir.x = outVel->x;
    velDir.y = outVel->y;
    velDir.z = outVel->z;

    velSquaredMod = SQ(velDir.x) + SQ(velDir.z) - SQ(velDir.y);
    dotProductMod = velDir.x * vecFromCenter.x + velDir.z * vecFromCenter.z - velDir.y * vecFromCenter.y;
    distSquaredMod = SQ(vecFromCenter.x) + SQ(vecFromCenter.z) - SQ(vecFromCenter.y);

    if (!(ABS(velSquaredMod) < 0.0001f) && velSquaredMod * distSquaredMod <= SQ(dotProductMod)) {
        time = (-dotProductMod - sqrtf(SQ(dotProductMod) - velSquaredMod * distSquaredMod)) / velSquaredMod;
        if (time > 0.0001f) { // Pinned source reads an uninitialized stack value here; no defined native equivalent.
            colPointY = time * velDir.y + vecFromCenter.y;
            if (colPointY >= 0.0f && colPointY <= height && time < speed) {
                reflect_from_cylinder_copy(&vecFromCenter, &velDir, time, outPos, outVel);
                speed *= 0.7f;
                Vec3fScale(outVel, speed);
                Vec3fAdd(outPos, center);
                return true;
            }
        }
    }
    return false;
}


bool hit(const Collider& c, const Mtx4f m, const Mtx4f inv, Vec3 p, Vec3 v, Vec3& op, Vec3& ov) {
    float global_speed=std::sqrt(dot_product(&v,&v));
    transform_point(inv,&p);transform_point_zero_w(inv,&v);
    float coefficient=global_speed/std::sqrt(dot_product(&v,&v));
    Vec3 center{};
    bool result=false;
    switch(c.hitbox.type) {
    case 1: result=collide_with_sphere(&p,&v,&center,c.hitbox.parameters.x,&op,&ov);break;
    case 2: result=func_800E5AD4_63284(&p,&v,&center,c.hitbox.parameters.x,c.hitbox.parameters.y*.5f,&op,&ov);break;
    case 3: result=collide_with_cylinder(&p,&v,&center,c.hitbox.parameters.x,c.hitbox.parameters.y*.5f,&op,&ov);break;
    case 4: result=collide_with_box(&p,&v,-c.hitbox.parameters.x,c.hitbox.parameters.x,-c.hitbox.parameters.y,c.hitbox.parameters.y,-c.hitbox.parameters.z,c.hitbox.parameters.z,&op,&ov);break;
    default: break;
    }
    if(result) {
        float speed=std::sqrt(dot_product(&ov,&ov));
        transform_point(m,&op);transform_point_zero_w(m,&ov);
        Vec3fNormalize(&ov);Vec3fScale(&ov,speed*coefficient);
    }
    return result;
}
bool hit(const Collider& c, Vec3 p, Vec3 v, Vec3& op, Vec3& ov) {
    Mtx4f m, inv;
    create_matrix(m,inv,c.scale,c.rotation,c.position);
    return hit(c,m,inv,p,v,op,ov);
}
std::size_t root_index(const CollisionModel& model) {
    std::size_t root=no_index;int depth=100;
    for(std::size_t i=0;i<model.colliders.size();++i) {
        const auto& c=model.colliders[i];
        if(c.depth==18) break;
        if(c.has_hitbox&&c.hitbox.type==1&&c.depth<depth) { root=i;depth=c.depth; }
    }
    return root;
}
template<class Hit>
bool collide(const CollisionModel& model,std::size_t root_index,Vec3 obj,Vec3 rotation,Vec3 scale,Vec3 p,Vec3 v,Vec3& op,Vec3& ov,Hit&& hit_collider) {
    if(root_index==no_index) return false;
    const auto* root=&model.colliders[root_index];
    Vec3fScale(&p,.01f);Vec3fScale(&v,.01f);
    float speed=Vec3fNormalize(&v);if(ABS(speed)<.1f) speed=.1f;Vec3fScale(&v,speed);
    float radius=root->hitbox.parameters.x*root->scale.x*scale.x;
    if(! (SQ(p.x-obj.x)+SQ(p.z-obj.z)<SQ(radius))) return false;
    Mtx4f m,inv;create_matrix(m,inv,scale,rotation,obj);
    transform_point(inv,&p);transform_point_zero_w(inv,&v);
    for(std::size_t i=0;i<model.colliders.size();++i) {
        const auto& c=model.colliders[i];
        if(c.depth==18) break;
        if(&c!=root&&c.has_hitbox&&hit_collider(i,c,p,v,op,ov)) {
            transform_point(m,&op);transform_point_zero_w(m,&ov);
            Vec3fScale(&op,100);Vec3fScale(&ov,100);return true;
        }
    }
    return false;
}
bool collide(const CollisionModel& model,Vec3 obj,Vec3 rotation,Vec3 scale,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) {
    return collide(model,root_index(model),obj,rotation,scale,p,v,op,ov,
        [](std::size_t,const Collider& c,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) { return hit(c,p,v,op,ov); });
}
} // namespace

PreparedModels::PreparedModels(std::span<const CollisionModel> models) {
    models_.reserve(models.size());
    std::size_t count=0;
    for(const auto& model:models) count+=model.colliders.size();
    transforms_.resize(count);
    std::size_t first=0;
    for(const auto& model:models) {
        models_.push_back({&model,first,root_index(model)});
        for(std::size_t i=0;i<model.colliders.size();++i) {
            const auto& c=model.colliders[i];
            if(c.depth==18) break;
            if(c.has_hitbox) {
                auto& t=transforms_[first+i];
                create_matrix(t.local_to_global,t.global_to_local,c.scale,c.rotation,c.position);
            }
        }
        first+=model.colliders.size();
    }
}
bool PreparedModels::collision_one(std::size_t index,const StaticObject& o,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) const noexcept {
    if(index>=models_.size()) return false;
    const auto& prepared=models_[index];
    const auto& model=*prepared.source;
    const Vec3 scale=model.scale>0?Vec3{model.scale,model.scale,model.scale}:o.scale;
    return collide(model,prepared.root,o.position,o.rotation,scale,p,v,op,ov,
        [&](std::size_t i,const Collider& c,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) {
            const auto& t=transforms_[prepared.first_transform+i];
            return hit(c,t.local_to_global,t.global_to_local,p,v,op,ov);
        });
}
bool PreparedModels::collision_model_one(std::size_t index,Vec3 obj,Vec3 rotation,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) const noexcept {
    if(index>=models_.size()) return false;
    const auto& prepared=models_[index];
    const auto& model=*prepared.source;
    // Authored aggregate fixtures have no Snapshot. Real standalone resources
    // retain their actual bounded record and reject a moved-from source.
    if(model.snapshot && (model.snapshot->canonical().data()!=model.source_data ||
                         model.snapshot->canonical().size()!=rom::us_size)) return false;
    Vec3fScale(&obj,.01f);
    return collide(model,prepared.root,obj,rotation,{model.scale,model.scale,model.scale},p,v,op,ov,
        [&](std::size_t i,const Collider& c,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) {
            const auto& t=transforms_[prepared.first_transform+i];
            return hit(c,t.local_to_global,t.global_to_local,p,v,op,ov);
        });
}

bool collision_primitive(const Collider& c,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) noexcept {
    return c.has_hitbox&&hit(c,p,v,op,ov);
}
bool collision_one(const CollisionModel& m,const StaticObject& o,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) noexcept {
    Vec3 scale=m.scale>0?Vec3{m.scale,m.scale,m.scale}:o.scale;
    return collide(m,o.position,o.rotation,scale,p,v,op,ov);
}
bool collision_model_one(const CollisionModel& m,Vec3 obj,Vec3 rotation,Vec3 p,Vec3 v,Vec3& op,Vec3& ov) noexcept {
    Vec3fScale(&obj,.01f);
    return collide(m,obj,rotation,{m.scale,m.scale,m.scale},p,v,op,ov);
}
const HeightPatch* height_patch_at(const HeightMap& map,float x,float z) noexcept {
    std::size_t index=0;
    for(std::size_t visited=0;index<map.nodes.size()&&visited<map.nodes.size();++visited) {
        const auto& n=map.nodes[index];bool right=n.a*x+n.b*z+n.c<=0;
        auto patch=right?n.right_patch:n.left_patch;
        if(patch!=no_index) return patch<map.patches.size()?&map.patches[patch]:nullptr;
        index=right?n.right_child:n.left_child;
    }
    return nullptr;
}
float patch_height_at(const HeightPatch* p,float x,float z) noexcept {
    return !p||p->c==0?0:-(p->a*x+p->b*z+p->d)/p->c;
}
bool ground_at_global(const HeightMap& map,float x,float z,GroundResult& out) noexcept {
    if(map.nodes.empty()||map.patches.empty()) { out={};return false; }
    const auto* p=height_patch_at(map,x,z);
    // GAME dereferences a null patch normal. Reject malformed/uncovered maps
    // rather than manufacturing a surface where source behavior is undefined.
    if(!p) { out={};return false; }
    out.height=patch_height_at(p,x,z);
    // ground_int.c118-129 and own vector.c5-16 normalize (A,B,C) before
    // ground.c swaps B/C, using a float reciprocal followed by multiplication.
    Vec3 normal{p->a,p->b,p->c};
    const float magnitude=std::sqrt(dot_product(&normal,&normal));
    if(magnitude!=0) {
        const float inverse=1.0f/magnitude;
        normal.x*=inverse;normal.y*=inverse;normal.z*=inverse;
    }
    out.normal={normal.x,normal.z,normal.y};
    out.surface=static_cast<std::int32_t>((p->surface>>8)&0xffffff);
    return true;
}
bool ground_at(const HeightMap& map,Vec3 block,float x,float z,GroundResult& out) noexcept {
    const float global_x=x/100+block.x,global_z=z/100+block.z;
    if(!std::isfinite(global_x)||!std::isfinite(global_z)) { out={};return false; }
    if(!ground_at_global(map,global_x,global_z,out)) return false;
    out.height=(out.height-block.y)*100;return true;
}
} // namespace isle::world::collision
