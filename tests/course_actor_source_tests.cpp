#include <cstdio>

extern "C" int isle_course_actor_fence_smoke(void);

int main() {
    // Failure transfers stay inside the C fixture, never across a C++ owner.
    const int failed_line = isle_course_actor_fence_smoke();
    if (failed_line) {
        std::fprintf(stderr, "course actor C fence failed at C line %d\n", failed_line);
        return 1;
    }
    std::puts("course actor C fence passed");
    return 0;
}
