#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "platform/file.h"

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_PROBE_RACING_COUNT = 7, DD2_PROBE_PERCENT = 100 };
static const uint64_t dd2_probe_hash_prime = UINT64_C(1099511628211);
static const uint64_t dd2_probe_hash_initial = UINT64_C(14695981039346656037);

typedef struct {
    double min_x;
    double max_x;
    double min_z;
    double max_z;
} dd2_probe_bounds;

typedef struct {
    const dd2_road *road;
    dd2_road_surface *surface;
    dd2_probe_bounds *bounds;
    dd2_road_contact *candidates;
    uint64_t queries;
    uint64_t found;
    uint64_t tested_cells;
    size_t maximum_cells;
    uint64_t trace;
} dd2_surface_probe;

static dd2_road *dd2_probe_load(const char *path, char code) {
    dd2_file file = {0};
    dd2_archive *archive = NULL;
    dd2_asset asset = {0};
    dd2_level_data level = {0};
    dd2_road *road = NULL;
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = code;
    if (dd2_file_read(path, &file) &&
        dd2_archive_open(file.data, file.size, &archive) == DD2_ARCHIVE_OK &&
        dd2_archive_find(archive, name, &asset) &&
        dd2_level_decode((dd2_byte_view){.data = asset.bytes, .size = asset.size}, &level)) {
        road = dd2_road_create(&level, code <= '0' + DD2_PROBE_RACING_COUNT ? DD2_ROAD_RACING
                                                                            : DD2_ROAD_ARENA);
    }
    /* The surface must work after the archive and platform file are released. */
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return road;
}

static void dd2_probe_bounds_init(dd2_surface_probe *probe) {
    const dd2_road_cell *cells = dd2_road_cells(probe->road);
    const dd2_track_vertex *vertices = dd2_road_vertices(probe->road);
    for (size_t index = 0; index < dd2_road_cell_count(probe->road); ++index) {
        dd2_probe_bounds *bounds = &probe->bounds[index];
        const dd2_track_vertex first = vertices[cells[index].vertices[0]];
        *bounds = (dd2_probe_bounds){.min_x = (double)first.x,
                                     .max_x = (double)first.x,
                                     .min_z = (double)first.z,
                                     .max_z = (double)first.z};
        for (size_t corner = 1; corner < DD2_ROAD_CORNERS; ++corner) {
            const dd2_track_vertex vertex = vertices[cells[index].vertices[corner]];
            bounds->min_x = fmin(bounds->min_x, (double)vertex.x);
            bounds->max_x = fmax(bounds->max_x, (double)vertex.x);
            bounds->min_z = fmin(bounds->min_z, (double)vertex.z);
            bounds->max_z = fmax(bounds->max_z, (double)vertex.z);
        }
    }
}

static bool dd2_probe_linear(dd2_surface_probe *probe, dd2_surface_query query,
                             dd2_road_contact *result) {
    size_t count = 0;
    double highest = -DBL_MAX;
    for (size_t index = 0; index < dd2_road_cell_count(probe->road); ++index) {
        const dd2_probe_bounds bounds = probe->bounds[index];
        if (query.point.x < bounds.min_x - DD2_ROAD_EDGE_TOLERANCE ||
            query.point.x > bounds.max_x + DD2_ROAD_EDGE_TOLERANCE ||
            query.point.z < bounds.min_z - DD2_ROAD_EDGE_TOLERANCE ||
            query.point.z > bounds.max_z + DD2_ROAD_EDGE_TOLERANCE) {
            continue;
        }
        dd2_road_contact contact = {0};
        if (dd2_road_contact_cell(probe->road, index, query.point, &contact) &&
            contact.height >= query.min_height && contact.height <= query.max_height) {
            probe->candidates[count++] = contact;
            highest = fmax(highest, contact.height);
        }
    }
    *result = (dd2_road_contact){0};
    bool found = false;
    for (size_t index = 0; index < count; ++index) {
        const dd2_road_contact candidate = probe->candidates[index];
        if (candidate.height < highest - DD2_ROAD_EDGE_TOLERANCE) {
            continue;
        }
        if (!found) {
            *result = candidate;
            found = true;
        }
        if (candidate.cell == query.preferred_cell) {
            *result = candidate;
            break;
        }
    }
    return found;
}

static bool dd2_probe_query(dd2_surface_probe *probe, dd2_surface_query query) {
    dd2_road_contact actual = {0};
    dd2_road_contact expected = {0};
    dd2_surface_statistics statistics = {0};
    const bool found = dd2_road_surface_sample(probe->surface, query, &actual, &statistics);
    const bool wanted = dd2_probe_linear(probe, query, &expected);
    if (found != wanted || actual.cell != expected.cell || actual.triangle != expected.triangle ||
        actual.height != expected.height || actual.normal[0] != expected.normal[0] ||
        actual.normal[1] != expected.normal[1] || actual.normal[2] != expected.normal[2]) {
        printf("Surface mismatch at %.17g %.17g height window %.17g %.17g: %d/%u vs %d/%u\n",
               query.point.x, query.point.z, query.min_height, query.max_height, (int)found,
               actual.cell, (int)wanted, expected.cell);
        return false;
    }
    ++probe->queries;
    probe->found += (uint64_t)found;
    probe->tested_cells += statistics.cell_tests;
    if (statistics.cell_tests > probe->maximum_cells) {
        probe->maximum_cells = statistics.cell_tests;
    }
    const uint32_t values[] = {(uint32_t)found, actual.cell, actual.triangle};
    for (size_t index = 0; index < sizeof(values) / sizeof(values[0]); ++index) {
        probe->trace = (probe->trace ^ values[index]) * dd2_probe_hash_prime;
    }
    return true;
}

static bool dd2_probe_centroids(dd2_surface_probe *probe, size_t cell) {
    const dd2_track_vertex *vertices = dd2_road_vertices(probe->road);
    const dd2_road_cell *geometry = &dd2_road_cells(probe->road)[cell];
    static const size_t corners[2][3] = {{0, 1, 3}, {2, 3, 1}};
    for (unsigned triangle = 0; triangle < 2; ++triangle) {
        dd2_surface_query query = {
            .min_height = -DBL_MAX, .max_height = DBL_MAX, .preferred_cell = (uint32_t)cell};
        double height = 0;
        for (size_t corner = 0; corner < 3; ++corner) {
            const dd2_track_vertex vertex = vertices[geometry->vertices[corners[triangle][corner]]];
            query.point.x += (double)vertex.x / 3;
            query.point.z += (double)vertex.z / 3;
            height += (double)vertex.y / 3;
        }
        if (!dd2_probe_query(probe, query)) {
            return false;
        }
        query.min_height = height - 1;
        query.max_height = height + 1;
        if (!dd2_probe_query(probe, query)) {
            return false;
        }
        query.min_height = -DBL_MAX;
        query.max_height = height - 1;
        if (!dd2_probe_query(probe, query)) {
            return false;
        }
    }
    return true;
}

static bool dd2_probe_edges(dd2_surface_probe *probe, size_t cell) {
    const dd2_track_vertex *vertices = dd2_road_vertices(probe->road);
    const dd2_road_cell *geometry = &dd2_road_cells(probe->road)[cell];
    for (size_t corner = 0; corner < DD2_ROAD_CORNERS; ++corner) {
        const dd2_track_vertex vertex = vertices[geometry->vertices[corner]];
        const dd2_surface_query query = {.point = {.x = (double)vertex.x, .z = (double)vertex.z},
                                         .min_height = -DBL_MAX,
                                         .max_height = DBL_MAX,
                                         .preferred_cell = DD2_ROAD_NO_STRIP};
        if (!dd2_probe_query(probe, query)) {
            return false;
        }
    }
    return true;
}

static bool dd2_probe_original(const dd2_road *road) {
    const size_t cells = dd2_road_cell_count(road);
    dd2_surface_probe probe = {.road = road,
                               .surface = dd2_road_surface_create(road),
                               .bounds = calloc(cells, sizeof(*probe.bounds)),
                               .candidates = calloc(cells, sizeof(*probe.candidates)),
                               .trace = dd2_probe_hash_initial};
    bool valid = probe.surface != NULL && probe.bounds != NULL && probe.candidates != NULL;
    if (valid) {
        dd2_probe_bounds_init(&probe);
        for (size_t cell = 0; valid && cell < cells; ++cell) {
            valid = dd2_probe_centroids(&probe, cell) && dd2_probe_edges(&probe, cell);
        }
    }
    if (valid) {
        const double reduction =
            (double)probe.tested_cells / ((double)probe.queries * (double)cells);
        printf("{\"cells\":%zu,\"queries\":%" PRIu64 ",\"found\":%" PRIu64
               ",\"cell_tests\":%" PRIu64 ",\"max_cell_tests\":%zu,\"mean_cell_tests\":%.17g"
               ",\"fraction_of_full_scan\":%.17g,\"trace\":\"%016" PRIx64 "\"}\n",
               cells, probe.queries, probe.found, probe.tested_cells, probe.maximum_cells,
               (double)probe.tested_cells / (double)probe.queries, reduction, probe.trace);
    }
    dd2_road_surface_destroy(probe.surface);
    free(probe.bounds);
    free(probe.candidates);
    return valid;
}

int main(int argc, char **argv) {
    if (argc != 3 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL) {
        return EXIT_FAILURE;
    }
    dd2_road *road = dd2_probe_load(argv[1], argv[2][0]);
    const bool valid = road != NULL && dd2_probe_original(road);
    dd2_road_destroy(road);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
