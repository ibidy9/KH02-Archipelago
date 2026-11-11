
/* Read-only UE4SS adapter. Preserve the existing authenticated ownership path. */
__declspec(dllexport) uint32_t Kh02SdkMovementOwnershipV1(uint32_t version, uint32_t size,
                                                          uint32_t *mask) {
    if (!mask || version != 1 || size != sizeof(*mask))
        return 0;
    *mask = 0;
    return Kh02MovementOwnership209(mask);
}
