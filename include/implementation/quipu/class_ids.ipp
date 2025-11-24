namespace vira::quipu {
    template <>
    ViraClassID getClassID<ViraClassID>()
    {
        return VIRA_CLASS_ID;
    };
    template <>
    ViraClassID getClassID<float>()
    {
        return VIRA_FLOAT;
    };
    template <>
    ViraClassID getClassID<double>()
    {
        return VIRA_DOUBLE;
    };
    template <>
    ViraClassID getClassID<uint8_t>()
    {
        return VIRA_UINT8;
    };
    template <>
    ViraClassID getClassID<uint16_t>()
    {
        return VIRA_UINT16;
    };
    template <>
    ViraClassID getClassID<uint32_t>()
    {
        return VIRA_UINT32;
    };
    template <>
    ViraClassID getClassID<uint64_t>()
    {
        return VIRA_UINT64;
    };
    template <>
    ViraClassID getClassID<int8_t>()
    {
        return VIRA_INT8;
    };
    template <>
    ViraClassID getClassID<int16_t>()
    {
        return VIRA_INT16;
    };
    template <>
    ViraClassID getClassID<int32_t>()
    {
        return VIRA_INT32;
    };
    template <>
    ViraClassID getClassID<int64_t>()
    {
        return VIRA_INT64;
    };
};
