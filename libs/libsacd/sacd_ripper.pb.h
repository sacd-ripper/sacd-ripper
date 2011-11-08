#ifndef _PB_SACD_RIPPER_PB_H_
#define _PB_SACD_RIPPER_PB_H_
#include <pb.h>

/* Enum definitions */
typedef enum {
    ServerRequest_Type_DISC_OPEN = 1,
    ServerRequest_Type_DISC_CLOSE = 2,
    ServerRequest_Type_DISC_READ = 3,
    ServerRequest_Type_DISC_SIZE = 4
} ServerRequest_Type;

typedef enum {
    ServerResponse_Type_DISC_OPENED = 1,
    ServerResponse_Type_DISC_CLOSED = 2,
    ServerResponse_Type_DISC_READ = 3,
    ServerResponse_Type_DISC_SIZE = 4
} ServerResponse_Type;

/* Struct definitions */
typedef struct {
    ServerRequest_Type type;
    uint32_t sector_offset;
    uint32_t sector_count;
} ServerRequest;

typedef struct {
    size_t size;
    uint8_t *bytes;
} ServerResponse_data_t;

typedef struct {
    ServerResponse_Type type;
    int64_t result;
    bool has_data;
    ServerResponse_data_t data;
} ServerResponse;

/* Default values for struct fields */
extern const uint32_t ServerRequest_sector_offset_default;
extern const uint32_t ServerRequest_sector_count_default;

/* Struct field encoding specification for nanopb */
extern const pb_field_t ServerRequest_fields[4];
extern const pb_field_t ServerResponse_fields[4];

#endif
