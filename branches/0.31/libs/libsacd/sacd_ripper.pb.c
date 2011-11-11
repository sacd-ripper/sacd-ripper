/* Automatically generated nanopb constant definitions */
#include "sacd_ripper.pb.h"

const uint32_t ServerRequest_sector_offset_default = 0;
const uint32_t ServerRequest_sector_count_default = 0;


const pb_field_t ServerRequest_fields[4] = {
    {1, PB_HTYPE_REQUIRED | PB_LTYPE_VARINT,
    offsetof(ServerRequest, type), 0,
    pb_membersize(ServerRequest, type), 0, 0},

    {2, PB_HTYPE_REQUIRED | PB_LTYPE_VARINT,
    pb_delta_end(ServerRequest, sector_offset, type), 0,
    pb_membersize(ServerRequest, sector_offset), 0,
    &ServerRequest_sector_offset_default},

    {3, PB_HTYPE_REQUIRED | PB_LTYPE_VARINT,
    pb_delta_end(ServerRequest, sector_count, sector_offset), 0,
    pb_membersize(ServerRequest, sector_count), 0,
    &ServerRequest_sector_count_default},

    PB_LAST_FIELD
};

const pb_field_t ServerResponse_fields[4] = {
    {1, PB_HTYPE_REQUIRED | PB_LTYPE_VARINT,
    offsetof(ServerResponse, type), 0,
    pb_membersize(ServerResponse, type), 0, 0},

    {2, PB_HTYPE_REQUIRED | PB_LTYPE_VARINT,
    pb_delta_end(ServerResponse, result, type), 0,
    pb_membersize(ServerResponse, result), 0, 0},

    {3, PB_HTYPE_OPTIONAL | PB_LTYPE_BYTES,
    pb_delta_end(ServerResponse, data, result),
    pb_delta(ServerResponse, has_data, data),
    512 * 2048, 0, 0},

    PB_LAST_FIELD
};

