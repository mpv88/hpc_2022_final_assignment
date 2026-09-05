#include "evolution_common.h"
#include <stdio.h>
#include <stdint.h>

static int test_neighbor_count(void)
{
    const int width = 3;
    const int height = 3;

    uint8_t grid[9] = {
        1, 0, 0,
        0, 0, 0,
        0, 0, 0
    };

    int neighbors = count_live_neighbors(grid, 2, 2, width, height);

    if (neighbors != 1) {
        printf("neighbor count test failed: expected 1, got %d\n", neighbors);
        return 1;
    }

    return 0;
}

static int test_parallel_neighbor_count(void)
{
    const int width = 3;
    const int local_rows = 3;

    uint8_t grid[5 * 3] = {0};
    uint8_t *data = grid + width;

    // top ghost row
    grid[0] = 1;
    // bottom ghost row
    grid[12] = 1;
    // top real cell
    if (count_live_neighbors_parallel(data, 0, 1, width, local_rows) != 1) {
        printf("top ghost row test failed\n");
        return 1;
    }
    // bottom real cell
    if (count_live_neighbors_parallel(data, 2, 1, width, local_rows) != 1) {
        printf("bottom ghost row test failed\n");
        return 1;
    }
    // horizontal periodicity
    data[1] = 1;

    if (count_live_neighbors_parallel(data, 1, 0, width, local_rows) != 1) {
        printf("horizontal periodicity test failed\n");
        return 1;
    }

    return 0;
}

static int test_next_cell_state(void)
{
    if (next_cell_state(0, 3) != 1) return 1;
    if (next_cell_state(0, 2) != 0) return 1;
    if (next_cell_state(1, 2) != 1) return 1;
    if (next_cell_state(1, 3) != 1) return 1;
    if (next_cell_state(1, 1) != 0) return 1;
    if (next_cell_state(1, 4) != 0) return 1;

    return 0;
}

int main(void)
{
    printf("starting tests\n");

    if (test_neighbor_count() != 0) {
        printf("test_neighbor_count failed\n");
        return 1;
    }
    printf("test_neighbor_count passed\n");

    if (test_parallel_neighbor_count() != 0) {
        printf("test_parallel_neighbor_count failed\n");
        return 1;
    }
    printf("test_parallel_neighbor_count passed\n");

    if (test_next_cell_state() != 0) {
        printf("test_next_cell_state failed\n");
        return 1;
    }
    printf("test_next_cell_state passed\n");

    printf("all evolution_common tests passed\n");
    return 0;
}