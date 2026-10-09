#include "assets/road.h"

#include "assets/bytes.h"
#include "assets/level.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_ROAD_HEADER_BYTES = 36,
    DD2_ROAD_LANE_BYTES = 14,
    DD2_ROAD_WORD_BYTES = 4,
    DD2_ROAD_VERTEX_BYTES = 12,
    DD2_ROAD_MAX_BYTES = 16 * 1024 * 1024,
    DD2_ROAD_MAX_RECORDS = 65536,
    DD2_ROAD_MAX_LANES = 127,
    DD2_ROAD_KIND_COUNT = 12,
    DD2_ROAD_GRID_SIDE = 32,
    DD2_ROAD_GRID_VERTICES = DD2_ROAD_GRID_SIDE * DD2_ROAD_GRID_SIDE,
    DD2_ROAD_GRID_CELLS = (DD2_ROAD_GRID_SIDE - 1) * (DD2_ROAD_GRID_SIDE - 1),
    DD2_ROAD_SOURCE_HEADING = 5,
    DD2_ROAD_SOURCE_NUMBER = 14,
    DD2_ROAD_SOURCE_VERTEX = 16,
    DD2_ROAD_SOURCE_FLAGS = 18,
    DD2_ROAD_SOURCE_NEXT = 20,
    DD2_ROAD_SOURCE_PREVIOUS = 24,
    DD2_ROAD_SOURCE_BRANCH = 28,
    DD2_ROAD_SPLIT = 8,
    DD2_ROAD_MERGE = 9
};

enum {
    DD2_ROAD_CONTENT_MAGIC = 8,
    DD2_ROAD_CONTENT_HEADER = 32,
    DD2_ROAD_CONTENT_STRIP = 28,
    DD2_ROAD_CONTENT_CELL = 28,
    DD2_ROAD_CONTENT_UNITS = DD2_ROAD_UNITS_PER_METER,
    DD2_ROAD_CONTENT_POSITION_LIMIT = 160000000,
    DD2_ROAD_CONTENT_MAX_CELLS = 1000000,
    DD2_ROAD_CONTENT_FIRST_CELL = 12,
    DD2_ROAD_CONTENT_MAIN_ORDER = 16,
    DD2_ROAD_CONTENT_FLAGS = 20,
    DD2_ROAD_CONTENT_KIND = 22,
    DD2_ROAD_CONTENT_LANES = 23,
    DD2_ROAD_CONTENT_HEADING = 24,
    DD2_ROAD_CONTENT_PADDING = 25,
    DD2_ROAD_CONTENT_CELL_STRIP = 16,
    DD2_ROAD_CONTENT_CELL_LANE = 20,
    DD2_ROAD_CONTENT_CELL_SURFACE = 24,
    DD2_ROAD_CONTENT_CELL_HEADING = 25,
    DD2_ROAD_CONTENT_CELL_MASK = 26
};

/* Original strip vertex lookup (two signed offsets per kind). Stored as format
 * constants, independent of the original executable's absolute table address. */
static const int dd2_road_rows[DD2_ROAD_KIND_COUNT][2] = {{0, 0},   {0, 0},  {-1, -1}, {0, -1},
                                                          {-1, -2}, {0, -1}, {0, 0},   {0, -1},
                                                          {0, 0},   {0, 0},  {-1, -1}, {0, 0}};

static const uint8_t dd2_road_first_triangles[DD2_ROAD_KIND_COUNT] = {0, 3, 2, 3, 2, 1,
                                                                      3, 1, 3, 3, 3, 3};
static const uint8_t dd2_road_last_triangles[DD2_ROAD_KIND_COUNT] = {0, 3, 3, 2, 2, 3,
                                                                     1, 1, 3, 3, 3, 3};

struct dd2_road {
    dd2_track_vertex *vertices;
    dd2_road_strip *strips;
    dd2_road_cell *cells;
    size_t vertex_count;
    size_t strip_count;
    size_t main_count;
    size_t cell_count;
};

typedef struct {
    dd2_byte_view source;
    uint32_t *offsets;
    size_t capacity;
    dd2_road *road;
} dd2_road_reader;

void dd2_road_destroy(dd2_road *road) {
    if (road != NULL) {
        free(road->cells);
        free(road->strips);
        free(road->vertices);
        free(road);
    }
}

static bool dd2_road_link(dd2_road_reader *reader, uint32_t offset, uint32_t *result) {
    if (offset % DD2_ROAD_WORD_BYTES != 0 ||
        offset > reader->source.size - DD2_ROAD_WORD_BYTES - DD2_ROAD_HEADER_BYTES) {
        return false;
    }
    const size_t word = offset / DD2_ROAD_WORD_BYTES;
    if (reader->offsets[word] == UINT32_MAX) {
        return false;
    }
    if (reader->offsets[word] == 0) {
        if (reader->road->strip_count == reader->capacity) {
            return false;
        }
        const size_t index = reader->road->strip_count++;
        reader->offsets[word] = (uint32_t)index + 1;
        reader->road->strips[index].source_offset = offset;
    }
    *result = reader->offsets[word] - 1;
    return true;
}

static bool dd2_road_reserve(dd2_road_reader *reader, const dd2_road_strip *strip) {
    const size_t size = DD2_ROAD_HEADER_BYTES + ((size_t)strip->lanes * DD2_ROAD_LANE_BYTES);
    if (strip->kind == 0 || strip->kind >= DD2_ROAD_KIND_COUNT || strip->lanes == 0 ||
        strip->lanes > DD2_ROAD_MAX_LANES ||
        size > reader->source.size - DD2_ROAD_WORD_BYTES - strip->source_offset) {
        return false;
    }
    const size_t begin = strip->source_offset / DD2_ROAD_WORD_BYTES;
    const size_t end =
        (strip->source_offset + size + DD2_ROAD_WORD_BYTES - 1) / DD2_ROAD_WORD_BYTES;
    for (size_t word = begin + 1; word < end; ++word) {
        if (reader->offsets[word] != 0) {
            return false;
        }
        reader->offsets[word] = UINT32_MAX;
    }
    return true;
}

static bool dd2_road_strip_decode(dd2_road_reader *reader, dd2_road_strip *strip) {
    const uint8_t *bytes = reader->source.data + DD2_ROAD_WORD_BYTES + strip->source_offset;
    strip->kind = bytes[0];
    strip->lanes = bytes[1];
    strip->source_lane_start = bytes[3];
    strip->heading = bytes[DD2_ROAD_SOURCE_HEADING];
    strip->source_number = dd2_read_le16(bytes + DD2_ROAD_SOURCE_NUMBER);
    strip->first_vertex = dd2_read_le16(bytes + DD2_ROAD_SOURCE_VERTEX);
    strip->flags = dd2_read_le16(bytes + DD2_ROAD_SOURCE_FLAGS);
    strip->main_order = DD2_ROAD_NO_STRIP;
    strip->branch = DD2_ROAD_NO_STRIP;
    if (!dd2_road_reserve(reader, strip) ||
        !dd2_road_link(reader, dd2_read_le32(bytes + DD2_ROAD_SOURCE_NEXT), &strip->next) ||
        !dd2_road_link(reader, dd2_read_le32(bytes + DD2_ROAD_SOURCE_PREVIOUS), &strip->previous)) {
        return false;
    }
    return (strip->kind != DD2_ROAD_SPLIT && strip->kind != DD2_ROAD_MERGE) ||
           dd2_road_link(reader, dd2_read_le32(bytes + DD2_ROAD_SOURCE_BRANCH), &strip->branch);
}

static bool dd2_road_main_loop(dd2_road *road) {
    uint32_t index = 0;
    do {
        dd2_road_strip *strip = &road->strips[index];
        if (strip->main_order != DD2_ROAD_NO_STRIP) {
            return false;
        }
        strip->main_order = (uint32_t)road->main_count++;
        index = strip->next;
    } while (index != 0);
    return true;
}

static bool dd2_road_paths_valid(const dd2_road *road, bool backward) {
    uint8_t *marks = calloc(road->strip_count, sizeof(*marks));
    if (marks == NULL) {
        return false;
    }
    marks[0] = 2;
    bool valid = true;
    for (size_t start = 1; valid && start < road->strip_count; ++start) {
        size_t index = start;
        while (marks[index] == 0) {
            marks[index] = 1;
            index = backward ? road->strips[index].previous : road->strips[index].next;
        }
        valid = marks[index] == 2;
        index = start;
        while (valid && marks[index] == 1) {
            marks[index] = 2;
            index = backward ? road->strips[index].previous : road->strips[index].next;
        }
    }
    free(marks);
    return valid;
}

static uint8_t dd2_road_triangle_mask(const dd2_road_strip *strip, unsigned lane) {
    uint8_t mask = 3;
    if (lane == 0) {
        mask &= dd2_road_first_triangles[strip->kind];
    }
    if (lane + 1 == strip->lanes) {
        mask &= dd2_road_last_triangles[strip->kind];
    }
    return mask;
}

static bool dd2_road_cell_decode(dd2_road *road, const dd2_road_strip *strip, dd2_road_cell *cell) {
    const int first = (int)strip->first_vertex + dd2_road_rows[strip->kind][0] + (int)cell->lane;
    const int second = (int)strip->first_vertex + dd2_road_rows[strip->kind][1] +
                       (int)strip->lanes + 1 + (int)cell->lane;
    if (first < 0 || second < 0 || (size_t)first + 1 >= road->vertex_count ||
        (size_t)second + 1 >= road->vertex_count) {
        return false;
    }
    cell->vertices[0] = (uint32_t)first;
    cell->vertices[1] = (uint32_t)first + 1;
    cell->vertices[2] = (uint32_t)second + 1;
    cell->vertices[3] = (uint32_t)second;
    cell->triangle_mask = dd2_road_triangle_mask(strip, cell->lane);
    return true;
}

static bool dd2_road_cells_decode(dd2_road_reader *reader) {
    dd2_road *road = reader->road;
    for (size_t index = 0; index < road->strip_count; ++index) {
        road->cell_count += road->strips[index].lanes;
    }
    road->cells = calloc(road->cell_count, sizeof(*road->cells));
    if (road->cells == NULL) {
        return false;
    }
    size_t count = 0;
    for (size_t index = 0; index < road->strip_count; ++index) {
        dd2_road_strip *strip = &road->strips[index];
        strip->first_cell = (uint32_t)count;
        const uint8_t *bytes = reader->source.data + DD2_ROAD_WORD_BYTES + strip->source_offset +
                               DD2_ROAD_HEADER_BYTES;
        for (unsigned lane = 0; lane < strip->lanes; ++lane) {
            dd2_road_cell *cell = &road->cells[count++];
            cell->strip = (uint32_t)index;
            cell->lane = lane;
            const uint8_t *surface = bytes + ((size_t)lane * DD2_ROAD_LANE_BYTES);
            cell->surface_flags = surface[0];
            cell->heading = surface[1];
            if (!dd2_road_cell_decode(road, strip, cell)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_road_racing_decode(dd2_road *road, dd2_byte_view bytes) {
    if (bytes.data == NULL || bytes.size < DD2_ROAD_WORD_BYTES + DD2_ROAD_HEADER_BYTES ||
        bytes.size > DD2_ROAD_MAX_BYTES) {
        return false;
    }
    const size_t capacity = dd2_read_le32(bytes.data);
    if (capacity == 0 || capacity > DD2_ROAD_MAX_RECORDS ||
        capacity > (bytes.size - DD2_ROAD_WORD_BYTES) / DD2_ROAD_HEADER_BYTES) {
        return false;
    }
    const size_t words = (bytes.size + DD2_ROAD_WORD_BYTES - 1) / DD2_ROAD_WORD_BYTES;
    dd2_road_reader reader = {.source = bytes,
                              .offsets = calloc(words, sizeof(*reader.offsets)),
                              .capacity = capacity,
                              .road = road};
    road->strips = calloc(capacity, sizeof(*road->strips));
    uint32_t first = 0;
    bool decoded =
        reader.offsets != NULL && road->strips != NULL && dd2_road_link(&reader, 0, &first);
    for (size_t index = 0; decoded && index < road->strip_count; ++index) {
        decoded = dd2_road_strip_decode(&reader, &road->strips[index]);
    }
    decoded = decoded && road->strip_count == capacity && dd2_road_main_loop(road) &&
              dd2_road_paths_valid(road, false) && dd2_road_paths_valid(road, true) &&
              dd2_road_cells_decode(&reader);
    free(reader.offsets);
    return decoded;
}

static bool dd2_road_arena_decode(dd2_road *road, dd2_byte_view bytes) {
    if (bytes.data == NULL || bytes.size != (size_t)DD2_ROAD_GRID_VERTICES * DD2_ROAD_LANE_BYTES ||
        road->vertex_count != DD2_ROAD_GRID_VERTICES) {
        return false;
    }
    road->cell_count = DD2_ROAD_GRID_CELLS;
    road->cells = calloc(road->cell_count, sizeof(*road->cells));
    if (road->cells == NULL) {
        return false;
    }
    size_t count = 0;
    for (uint32_t row = 0; row + 1 < DD2_ROAD_GRID_SIDE; ++row) {
        for (uint32_t column = 0; column + 1 < DD2_ROAD_GRID_SIDE; ++column) {
            const uint32_t first = (row * DD2_ROAD_GRID_SIDE) + column;
            const uint32_t second = first + DD2_ROAD_GRID_SIDE;
            road->cells[count++] =
                (dd2_road_cell){.vertices = {first, first + 1, second + 1, second},
                                .strip = DD2_ROAD_NO_STRIP,
                                .lane = first,
                                .surface_flags = bytes.data[(size_t)first * DD2_ROAD_LANE_BYTES],
                                .heading = bytes.data[((size_t)first * DD2_ROAD_LANE_BYTES) + 1],
                                .triangle_mask = 3};
        }
    }
    return true;
}

dd2_road *dd2_road_create(const dd2_level_data *level, dd2_road_layout layout) {
    if (level == NULL || level->vertex_count < 3 || level->vertex_count > DD2_ROAD_MAX_RECORDS ||
        (layout != DD2_ROAD_RACING && layout != DD2_ROAD_ARENA)) {
        return NULL;
    }
    const dd2_byte_view vertices = level->sections[DD2_LEVEL_ROAD_VERTICES];
    if (vertices.data == NULL || vertices.size != level->vertex_count * DD2_ROAD_VERTEX_BYTES) {
        return NULL;
    }
    dd2_road *road = calloc(1, sizeof(*road));
    if (road == NULL) {
        return NULL;
    }
    road->vertex_count = level->vertex_count;
    road->vertices = calloc(road->vertex_count, sizeof(*road->vertices));
    bool decoded = road->vertices != NULL;
    for (size_t index = 0; decoded && index < road->vertex_count; ++index) {
        decoded = dd2_level_vertex(level, index, &road->vertices[index]);
    }
    const dd2_byte_view bytes = level->sections[DD2_LEVEL_ROAD_STRIPS];
    decoded = decoded && (layout == DD2_ROAD_RACING ? dd2_road_racing_decode(road, bytes)
                                                    : dd2_road_arena_decode(road, bytes));
    if (!decoded) {
        dd2_road_destroy(road);
        return NULL;
    }
    return road;
}

static bool dd2_road_content_vertices(dd2_road *road, const uint8_t *bytes) {
    for (size_t index = 0; index < road->vertex_count; ++index) {
        dd2_track_vertex *vertex = &road->vertices[index];
        const uint8_t *record = bytes + (index * DD2_ROAD_VERTEX_BYTES);
        vertex->x = dd2_read_le_i32(record);
        vertex->y = dd2_read_le_i32(record + DD2_ROAD_WORD_BYTES);
        vertex->z = dd2_read_le_i32(record + ((size_t)2 * DD2_ROAD_WORD_BYTES));
        if (vertex->x < -DD2_ROAD_CONTENT_POSITION_LIMIT ||
            vertex->x > DD2_ROAD_CONTENT_POSITION_LIMIT ||
            vertex->y < -DD2_ROAD_CONTENT_POSITION_LIMIT ||
            vertex->y > DD2_ROAD_CONTENT_POSITION_LIMIT ||
            vertex->z < -DD2_ROAD_CONTENT_POSITION_LIMIT ||
            vertex->z > DD2_ROAD_CONTENT_POSITION_LIMIT) {
            return false;
        }
    }
    return true;
}

static bool dd2_road_content_strip(dd2_road *road, size_t index, const uint8_t *record) {
    dd2_road_strip *strip = &road->strips[index];
    strip->next = dd2_read_le32(record);
    strip->previous = dd2_read_le32(record + DD2_ROAD_WORD_BYTES);
    strip->branch = dd2_read_le32(record + ((size_t)2 * DD2_ROAD_WORD_BYTES));
    strip->first_cell = dd2_read_le32(record + DD2_ROAD_CONTENT_FIRST_CELL);
    strip->main_order = dd2_read_le32(record + DD2_ROAD_CONTENT_MAIN_ORDER);
    strip->flags = dd2_read_le16(record + DD2_ROAD_CONTENT_FLAGS);
    strip->kind = record[DD2_ROAD_CONTENT_KIND];
    strip->lanes = record[DD2_ROAD_CONTENT_LANES];
    strip->heading = record[DD2_ROAD_CONTENT_HEADING];
    for (size_t byte = DD2_ROAD_CONTENT_PADDING; byte < DD2_ROAD_CONTENT_STRIP; ++byte) {
        if (record[byte] != 0) {
            return false;
        }
    }
    const bool junction = strip->kind == DD2_ROAD_SPLIT || strip->kind == DD2_ROAD_MERGE;
    return strip->next < road->strip_count && strip->previous < road->strip_count &&
           (junction ? strip->branch < road->strip_count : strip->branch == DD2_ROAD_NO_STRIP) &&
           (strip->main_order == DD2_ROAD_NO_STRIP || strip->main_order < road->main_count) &&
           strip->kind > 0 && strip->kind < DD2_ROAD_KIND_COUNT && strip->lanes > 0 &&
           strip->lanes <= DD2_ROAD_MAX_LANES && strip->first_cell <= road->cell_count &&
           strip->lanes <= road->cell_count - strip->first_cell;
}

static bool dd2_road_content_main(const dd2_road *road) {
    if (road->strip_count == 0) {
        return road->main_count == 0;
    }
    uint8_t *seen = calloc(road->strip_count, sizeof(*seen));
    if (seen == NULL) {
        return false;
    }
    uint32_t current = 0;
    bool valid = true;
    for (size_t order = 0; valid && order < road->main_count; ++order) {
        valid = seen[current] == 0 && road->strips[current].main_order == order;
        seen[current] = 1;
        current = road->strips[current].next;
    }
    valid = valid && current == 0;
    for (size_t index = 0; valid && index < road->strip_count; ++index) {
        valid = (seen[index] != 0) == (road->strips[index].main_order != DD2_ROAD_NO_STRIP);
    }
    free(seen);
    return valid && dd2_road_paths_valid(road, false) && dd2_road_paths_valid(road, true);
}

static bool dd2_road_content_cells(dd2_road *road, const uint8_t *bytes) {
    for (size_t index = 0; index < road->cell_count; ++index) {
        const uint8_t *record = bytes + (index * DD2_ROAD_CONTENT_CELL);
        dd2_road_cell *cell = &road->cells[index];
        for (size_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
            cell->vertices[corner] = dd2_read_le32(record + (corner * DD2_ROAD_WORD_BYTES));
            if (cell->vertices[corner] >= road->vertex_count) {
                return false;
            }
        }
        cell->strip = dd2_read_le32(record + DD2_ROAD_CONTENT_CELL_STRIP);
        cell->lane = dd2_read_le32(record + DD2_ROAD_CONTENT_CELL_LANE);
        cell->surface_flags = record[DD2_ROAD_CONTENT_CELL_SURFACE];
        cell->heading = record[DD2_ROAD_CONTENT_CELL_HEADING];
        cell->triangle_mask = record[DD2_ROAD_CONTENT_CELL_MASK];
        if (cell->triangle_mask == 0 || cell->triangle_mask > 3 ||
            record[DD2_ROAD_CONTENT_CELL - 1] != 0) {
            return false;
        }
        if (road->strip_count == 0) {
            if (cell->strip != DD2_ROAD_NO_STRIP) {
                return false;
            }
        } else if (cell->strip >= road->strip_count ||
                   cell->lane >= road->strips[cell->strip].lanes ||
                   road->strips[cell->strip].first_cell + cell->lane != index) {
            return false;
        }
    }
    return true;
}

static bool dd2_road_content_decode(dd2_road *road, const uint8_t *bytes) {
    if (!dd2_road_content_vertices(road, bytes)) {
        return false;
    }
    bytes += road->vertex_count * DD2_ROAD_VERTEX_BYTES;
    size_t first = 0;
    for (size_t index = 0; index < road->strip_count; ++index) {
        if (!dd2_road_content_strip(road, index, bytes + (index * DD2_ROAD_CONTENT_STRIP)) ||
            road->strips[index].first_cell != first) {
            return false;
        }
        first += road->strips[index].lanes;
    }
    bytes += road->strip_count * DD2_ROAD_CONTENT_STRIP;
    return (road->strip_count == 0 || first == road->cell_count) && dd2_road_content_main(road) &&
           dd2_road_content_cells(road, bytes);
}

dd2_road *dd2_road_create_prepared(dd2_byte_view bytes) {
    static const uint8_t magic[DD2_ROAD_CONTENT_MAGIC] = {'D', 'D', '2', 'R', 'O', 'A', 'D', '1'};
    if (bytes.data == NULL || bytes.size < DD2_ROAD_CONTENT_HEADER) {
        return NULL;
    }
    for (size_t byte = 0; byte < sizeof(magic); ++byte) {
        if (bytes.data[byte] != magic[byte]) {
            return NULL;
        }
    }
    const uint8_t *header = bytes.data + DD2_ROAD_CONTENT_MAGIC;
    const uint32_t layout = dd2_read_le32(header);
    const uint32_t units = dd2_read_le32(header + DD2_ROAD_WORD_BYTES);
    const size_t vertices = dd2_read_le32(header + ((size_t)2 * DD2_ROAD_WORD_BYTES));
    const size_t strips = dd2_read_le32(header + ((size_t)3 * DD2_ROAD_WORD_BYTES));
    const size_t main = dd2_read_le32(header + ((size_t)4 * DD2_ROAD_WORD_BYTES));
    const size_t cells = dd2_read_le32(header + ((size_t)5 * DD2_ROAD_WORD_BYTES));
    if (layout > DD2_ROAD_ARENA || units != DD2_ROAD_CONTENT_UNITS || vertices == 0 ||
        vertices > DD2_ROAD_MAX_RECORDS || strips > DD2_ROAD_MAX_RECORDS || cells == 0 ||
        cells > DD2_ROAD_CONTENT_MAX_CELLS || main > strips ||
        (layout == DD2_ROAD_ARENA ? strips != 0 || main != 0 : strips == 0 || main == 0) ||
        bytes.size != DD2_ROAD_CONTENT_HEADER + (vertices * DD2_ROAD_VERTEX_BYTES) +
                          (strips * DD2_ROAD_CONTENT_STRIP) + (cells * DD2_ROAD_CONTENT_CELL)) {
        return NULL;
    }
    dd2_road *road = calloc(1, sizeof(*road));
    if (road == NULL) {
        return NULL;
    }
    road->vertex_count = vertices;
    road->strip_count = strips;
    road->main_count = main;
    road->cell_count = cells;
    road->vertices = calloc(vertices, sizeof(*road->vertices));
    road->strips = strips != 0 ? calloc(strips, sizeof(*road->strips)) : NULL;
    road->cells = calloc(cells, sizeof(*road->cells));
    if (road->vertices == NULL || (strips != 0 && road->strips == NULL) || road->cells == NULL ||
        !dd2_road_content_decode(road, bytes.data + DD2_ROAD_CONTENT_HEADER)) {
        dd2_road_destroy(road);
        return NULL;
    }
    return road;
}

size_t dd2_road_vertex_count(const dd2_road *road) {
    return road != NULL ? road->vertex_count : 0;
}
const dd2_track_vertex *dd2_road_vertices(const dd2_road *road) {
    return road != NULL ? road->vertices : NULL;
}
size_t dd2_road_strip_count(const dd2_road *road) {
    return road != NULL ? road->strip_count : 0;
}
const dd2_road_strip *dd2_road_strips(const dd2_road *road) {
    return road != NULL ? road->strips : NULL;
}
size_t dd2_road_main_count(const dd2_road *road) {
    return road != NULL ? road->main_count : 0;
}
size_t dd2_road_cell_count(const dd2_road *road) {
    return road != NULL ? road->cell_count : 0;
}
const dd2_road_cell *dd2_road_cells(const dd2_road *road) {
    return road != NULL ? road->cells : NULL;
}
