#include "menu_animation.hpp"
#include <cmath>
#include <stdexcept>

// GAME df8815e75f3d79618466e01e0827c6165b93d11c src/1F960.c.
// Native numeric data, not an SDK layout. Preserve source binary32 grouping.
namespace isle::menu::animation {
namespace {
float speed(float x, const float* c) {
    float v = c[0]*(x*x*x*x) + c[1]*(x*x*x) + c[2]*(x*x) + c[3]*x + c[4];
    if (v < 0 && v > -0.001f) v = 0;
    return std::sqrt(v);
}
float integrate(float start, float end, const float* c) {
    const float step = (end-start)/8;
    float sum = 0, x = start+step;
    for (int i = 2; i < 9; ++i, x += step) sum += (i%2 == 0 ? 4.0f : 2.0f)*speed(x,c);
    return (speed(start,c)+sum+speed(end,c))*step/3;
}
float uniform(const AnimationPath& p, float t) {
    if (!std::isfinite(t) || t < 0 || t > 1 || p.segment_point_count < 2 || p.kind < 0 || p.kind > 3 ||
        p.timestamps.size() < p.segment_point_count)
        throw std::invalid_argument("Invalid native GAME interpolation path/parameter");
    if (t == 1) return 1;
    std::size_t segment = 0;
    while (segment+1 < p.timestamps.size() && p.timestamps[segment+1] < t) ++segment;
    if (segment+1 >= p.segment_point_count || p.timestamps[segment+1] <= p.timestamps[segment])
        throw std::invalid_argument("Invalid GAME interpolation timestamp interval");
    float mid = 0, low = 0, high = 1;
    if (p.kind == 0) mid = (t-p.timestamps[segment])/(p.timestamps[segment+1]-p.timestamps[segment]);
    else {
        if (p.speed_polynomials.size() < (segment+1)*5)
            throw std::invalid_argument("Missing GAME spline speed polynomial");
        float target = (t-p.timestamps[segment])*p.duration;
        const auto* c = p.speed_polynomials.data()+segment*5;
        for (;;) {
            mid = (low+high)*0.5f;
            const float current = integrate(low,mid,c);
            if (!std::isfinite(current)) throw std::invalid_argument("Invalid GAME spline speed polynomial");
            if (target < current+0.00001f) high = mid;
            else { low = mid; target = target-current; }
            if (std::fabs(low-high) < 0.00001f || !(target > current+0.00001f || target < current-0.00001f)) break;
        }
    }
    return (static_cast<float>(segment)+mid)/(static_cast<float>(p.segment_point_count)-1.0f);
}
Vec3 evaluate(const AnimationPath& p, float parameter, bool derivative) {
    float t = uniform(p,parameter);
    const auto count = p.segment_point_count;
    std::size_t segment = 0;
    const bool endpoint = t == 1;
    t *= static_cast<float>(count-1);
    segment = static_cast<std::size_t>(t);
    t -= static_cast<float>(segment);
    if (p.kind == 0) {
        if (endpoint) {
            if (!derivative) return p.points.at(count-1);
            --segment;
        }
        const auto a = p.points.at(segment), b = p.points.at(segment+1);
        if (derivative) return {b.x-a.x,b.y-a.y,b.z-a.z};
        return {(b.x-a.x)*t+a.x,(b.y-a.y)*t+a.y,(b.z-a.z)*t+a.z};
    }
    if (endpoint && !derivative) {
        if (p.kind == 1) return p.points.at((count-1)*3);
        if (p.kind == 3) return p.points.at(count);
        segment = count-2; t = 1;
    }
    // Source velocity uses the terminal segment at t==1; admission retains its
    // required trailing spline points rather than silently clamping its read.
    const auto first = p.kind == 1 ? segment*3 : segment;
    if (first+4 > p.points.size()) throw std::invalid_argument("GAME spline control extent");
    float w0, w1, w2, w3;
    const float t2 = t*t, t3 = t2*t, u = 1-t;
    if (p.kind == 1) {
        if (derivative) {
            const float a = t-1;
            w0 = -3.0f*a*a; w1 = 3.0f*(1.0f-4.0f*t+3.0f*t2);
            w2 = 3.0f*(2.0f*t-3.0f*t2); w3 = 3.0f*t2;
        } else { const float u2 = u*u; w0 = u2*u; w1 = 3*t*u2; w2 = 3*t2*u; w3 = t2*t; }
    } else if (p.kind == 2) {
        if (derivative) {
            w0 = -0.5f*u*u; w1 = 0.5f*(3*t2-4*t);
            w2 = 0.5f*(-3*t2+2*t+1); w3 = 0.5f*t2;
        } else {
            w0 = (1.0f/6.0f)*u*u*u; w1 = (1.0f/6.0f)*(3*t3-6*t2+4);
            w2 = (1.0f/6.0f)*(3*(t2-t3+t)+1); w3 = (1.0f/6.0f)*t3;
        }
    } else {
        const float a = p.tension;
        if (derivative) {
            w0 = (-3*t2+4*t-1)*a; w1 = (2-a)*3*t2+2*(a-3)*t;
            w2 = (a-2)*3*t2+2*(3-2*a)*t+a; w3 = (3*t2-2*t)*a;
        } else {
            w0 = (2*t2-t3-t)*a; w1 = (2-a)*t3+(a-3)*t2+1;
            w2 = (a-2)*t3+(3-2*a)*t2+a*t; w3 = (t3-t2)*a;
        }
    }
    const auto* q = p.points.data()+first;
    return {q[0].x*w0+q[1].x*w1+q[2].x*w2+q[3].x*w3,
            q[0].y*w0+q[1].y*w1+q[2].y*w2+q[3].y*w3,
            q[0].z*w0+q[1].z*w1+q[2].z*w2+q[3].z*w3};
}
}
Vec3 path_position(const AnimationPath& p, float t) { return evaluate(p,t,false); }
Vec3 path_velocity(const AnimationPath& p, float t) { return evaluate(p,t,true); }
} // namespace isle::menu::animation
