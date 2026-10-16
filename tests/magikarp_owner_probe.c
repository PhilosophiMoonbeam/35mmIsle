#include "magikarp_owner_probe.h"

void isle_magikarp_geo_probe(IsleGameObject* object, void* user) {
    static const IsleActorGeoDefinition common = { UINT32_C(0x8038837c) };
    IsleMagikarpGeoProbe* probe = (IsleMagikarpGeoProbe*) user;
    ++probe->calls;
    probe->child = Pokemon_AddAtGeo(object, 1003, &common);
}
