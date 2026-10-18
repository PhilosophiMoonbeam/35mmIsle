#include "actor_resource_catalog.hpp"

#include <stdexcept>

namespace isle::actor::detail {
const CourseCatalog& course_catalog(std::int32_t course_id) {
    switch (course_id) {
    case 0: return beach_catalog();
    case 1: return tunnel_catalog();
    case 2: return volcano_catalog();
    case 3: return river_catalog();
    case 4: return cave_catalog();
    case 5: return valley_catalog();
    case 6: return rainbow_catalog();
    default: throw std::invalid_argument("Unknown actor resource course ID");
    }
}
} // namespace isle::actor::detail
