#include "photo_presentation.hpp"
namespace {
isle::photo::Presentation* owner(IslePhotoPresentation* value) noexcept {
    return reinterpret_cast<isle::photo::Presentation*>(value);
}
}
extern "C" int isle_photo_presentation_begin(IslePhotoPresentation* p, IsleGameCamera* camera, const IslePhotoRecord* record) {
    return p && record && owner(p)->begin(camera,*record);
}
extern "C" int isle_photo_presentation_begin_working(IslePhotoPresentation* p, IsleGameCamera* camera, IslePhotoWorking* working) {
    return p && working && owner(p)->begin_working(camera,working);
}
extern "C" IslePhotoPresentationStatus isle_photo_presentation_poll(IslePhotoPresentation* p) {
    return p?owner(p)->poll():ISLE_PHOTO_PRESENTATION_FAILED;
}
extern "C" IslePhotoPresentationStatus isle_photo_presentation_clear(IslePhotoPresentation* p) {
    return p?owner(p)->clear():ISLE_PHOTO_PRESENTATION_FAILED;
}
extern "C" IslePhotoPresentationStatus isle_photo_presentation_release_to_replay(IslePhotoPresentation* p) {
    return p?owner(p)->release_to_replay():ISLE_PHOTO_PRESENTATION_FAILED;
}
extern "C" IslePhotoPresentationStatus isle_photo_presentation_retire(IslePhotoPresentation* p) {
    return p?owner(p)->retire():ISLE_PHOTO_PRESENTATION_FAILED;
}
extern "C" int isle_photo_presentation_pause_links(IslePhotoPresentation* p, uint64_t mask, int paused) {
    return p && owner(p)->pause_links(mask,paused!=0);
}
extern "C" IslePhotoPresentationStatus isle_photo_presentation_prepare_course(IslePhotoPresentation* p, const IslePhotoRecord* record) {
    return p && record?owner(p)->prepare_course(*record):ISLE_PHOTO_PRESENTATION_FAILED;
}
