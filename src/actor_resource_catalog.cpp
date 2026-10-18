#include "actor_resource_catalog.hpp"

#include <stdexcept>

namespace isle::actor::detail {
const CourseCatalog& course_catalog(std::int32_t course_id) {
    switch (course_id) {
    case 0: return beach_catalog();
    case 6: return rainbow_catalog();
    default: throw std::invalid_argument("Unsupported live actor resource course; only Beach 0 and Rainbow 6 have closed catalogs");
    }
}
} // namespace isle::actor::detail
