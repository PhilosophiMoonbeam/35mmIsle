#ifndef ISLE_PRIVATE_POST_COURSE_PROBE_INPUTS_HPP
#define ISLE_PRIVATE_POST_COURSE_PROBE_INPUTS_HPP

namespace isle::application { class Photos; }
namespace isle::game { class State; }

namespace isle::private_post_course {
// Authored semantic save inputs only; neither captured gameplay nor GPU scores.
void seed(application::Photos&, game::State&);
}

#endif
