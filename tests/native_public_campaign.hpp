#ifndef ISLE_NATIVE_PUBLIC_CAMPAIGN_HPP
#define ISLE_NATIVE_PUBLIC_CAMPAIGN_HPP
#include <filesystem>

namespace isle::native_public_campaign {
// Fresh, actual HostControlDeck campaign. Gameplay is supplied exclusively by
// real platform input; boundary/save output is passive qualification evidence.
int run(const std::filesystem::path& rom, const std::filesystem::path& root);
// Separate negative qualification: ordinary Host stop while a real Camera/Photo
// preview submission remains unretired. Never accepts an incomplete campaign.
int run_preview_stop(const std::filesystem::path& rom, const std::filesystem::path& root);
}
#endif
