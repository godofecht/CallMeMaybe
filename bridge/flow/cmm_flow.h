#ifndef CMM_FLOW_H
#define CMM_FLOW_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t cmm_flow_info;

typedef enum cmm_flow_kind {
    CMM_FLOW_VOID = 0,
    CMM_FLOW_BOOL = 1,
    CMM_FLOW_I8 = 2,
    CMM_FLOW_U8 = 3,
    CMM_FLOW_I16 = 4,
    CMM_FLOW_U16 = 5,
    CMM_FLOW_I32 = 6,
    CMM_FLOW_U32 = 7,
    CMM_FLOW_I64 = 8,
    CMM_FLOW_U64 = 9,
    CMM_FLOW_F32 = 10,
    CMM_FLOW_F64 = 11,
    CMM_FLOW_POINTER = 12,
    CMM_FLOW_STRING = 13,
    CMM_FLOW_BYTES = 14,
    CMM_FLOW_UNSUPPORTED = 255
} cmm_flow_kind;

typedef struct cmm_flow_value {
    uint32_t kind;
    uint32_t reserved;
    uint64_t bits;
    uint64_t extra;
} cmm_flow_value;

typedef struct cmm_flow_span_u8 {
    const uint8_t* data;
    int64_t len;
} cmm_flow_span_u8;

cmm_flow_info cmm_flow_reflect_name(const char* name);
uint64_t cmm_flow_parameter_count(cmm_flow_info function_id);
uint32_t cmm_flow_parameter_kind(cmm_flow_info function_id, uint64_t index);
uint32_t cmm_flow_return_kind(cmm_flow_info function_id);
int32_t cmm_flow_invoke(
    cmm_flow_info function_id,
    const cmm_flow_value* arguments,
    uint64_t argument_count,
    cmm_flow_value* result);
const char* cmm_flow_error_string(int32_t error);
uint64_t cmm_flow_f32_bits(float value);
uint64_t cmm_flow_f64_bits(double value);
float cmm_flow_bits_f32(uint64_t bits);
double cmm_flow_bits_f64(uint64_t bits);
uint64_t cmm_flow_string_bits(const char* value);
const char* cmm_flow_bits_string(uint64_t bits);
cmm_flow_value cmm_flow_bytes(cmm_flow_span_u8 value);
cmm_flow_span_u8 cmm_flow_as_bytes(cmm_flow_value value);
uint64_t cmm_flow_byte_span_data(cmm_flow_span_u8 value);
const uint8_t* cmm_flow_bits_u8_ptr(uint64_t bits);

#ifdef __cplusplus
}
#endif

#endif
