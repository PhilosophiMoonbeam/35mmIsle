#ifndef ISLE_CART_ANIMATION_REFERENCE_NUMERIC_H
#define ISLE_CART_ANIMATION_REFERENCE_NUMERIC_H
#ifdef __cplusplus
extern "C" {
#endif
/* Test-only semantic values, not guest/SDK structures or ABI layouts. */
typedef struct { float x, y, z; } IsleReferencePoint;
typedef struct { float c[5]; } IsleReferenceSpeed;
typedef struct {
    int type, numPoints;
    float unk_04, duration;
    IsleReferencePoint* points;
    float* paramPoints;
    IsleReferenceSpeed* speedFunc;
} IsleReferencePath;
float isle_reference_camera(float, float, float, float, float, float);
float isle_reference_model(float, float, float, float, float, float);
float isle_reference_map(IsleReferencePath*, float);
void isle_reference_position(IsleReferencePoint*, IsleReferencePath*, float);
#ifdef __cplusplus
}
#endif
#endif
