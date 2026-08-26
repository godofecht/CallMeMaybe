#include "cmm_flow.h"

#include <cstdint>

#include "cmm/meta.hpp"

extern "C" int32_t cmm_flow_release_object(uint64_t object_handle, cmm_flow_info type_id)
{
    if (object_handle == 0 || type_id == cmm::invalid_info)
    {
        return static_cast<int32_t>(cmm::Error::NullValue);
    }

    const cmm::info destructor_id = cmm::lookup::get_destructor(type_id);
    if (destructor_id == cmm::invalid_info)
    {
        return static_cast<int32_t>(cmm::Error::EntityNotFound);
    }

    cmm_flow_value result{CMM_FLOW_VOID, 0, 0, 0};
    return cmm_flow_invoke_method(destructor_id, object_handle, nullptr, 0, &result);
}
