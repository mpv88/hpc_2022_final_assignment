#include "evolution_fog.h"
#include "evolution_common.h"

static uint32_t fog_random_state;

static uint32_t fog_random(void)
{
    fog_random_state ^= fog_random_state << 13;
    fog_random_state ^= fog_random_state >> 17;
    fog_random_state ^= fog_random_state << 5;
    return fog_random_state;
}

static int random_index(int size)
{
    return (int)(fog_random() % (uint32_t)size);  // random index in [0, size) range
}

static int wrap_index(int index, int size)
{ // wrap index to [0, size) assuming periodic grid
    if (index < 0)
        return size - 1; // to last valid index
    if (index >= size)
        return 0; // to 0
    return index; // if in range, unchanged
}

static void collect_dead_neighbors(const uint8_t *grid, int row, int column, int width, int height, int rows[8], int columns[8], int *count)
{ // collect number and coordinates of all dead 8-neighbors
    *count = 0; // init count of dead neighbors

    for (int row_offset = -1; row_offset <= 1; row_offset++) { // scan 3 rows around current
        for (int column_offset = -1; column_offset <= 1; column_offset++) { // scan 3 cols around current
            if (row_offset == 0 && column_offset == 0) // skip current cell
                continue;

            int neighbor_row = wrap_index(row + row_offset, height); // wrapped neighbour row
            int neighbor_column = wrap_index(column + column_offset, width); // wrapped neighbour col

            if (grid[neighbor_row * width + neighbor_column] == 0) { // if neighbor is dead (0)
                rows[*count] = neighbor_row; // store its row
                columns[*count] = neighbor_column; // store its col
                (*count)++;
            }
        }
    }
}

static void shuffle_neighbors(int rows[8], int columns[8], int count)
{ // Fisher–Yates shuffle
    for (int i = count - 1; i > 0; i--) { // from last element down to second
        int j = random_index(i + 1); // pick random j in [0, i]
        // swap rows[i] and rows[j]
        int temporary = rows[i];
        rows[i] = rows[j];
        rows[j] = temporary;
        // swap cols[i] and cols[j]
        temporary = columns[i];
        columns[i] = columns[j];
        columns[j] = temporary;
    }
}

static void collect_dead_neighbors_parallel(const uint8_t *grid, int row, int column, int width, int height, int start_row, int local_rows, int rows[8], int columns[8], int *count)
{ // collect number and coordinates of all dead 8-neighbors
    *count = 0; // init count of dead neighbors

    for (int row_offset = -1; row_offset <= 1; row_offset++) { // scan 3 rows around current
        for (int column_offset = -1; column_offset <= 1; column_offset++) { // scan 3 cols around current
            if (row_offset == 0 && column_offset == 0) // skip current cell
                continue;

            int neighbor_row = wrap_index(row + row_offset, height); // wrapped neighbour row
            int neighbor_column = wrap_index(column + column_offset, width); // wrapped neighbour col
            int local_neighbor_row = neighbor_row - start_row;

            if (local_neighbor_row >= -1 && local_neighbor_row <= local_rows &&
                grid[local_neighbor_row * width + neighbor_column] == 0) { // if neighbor is dead (0)
                rows[*count] = neighbor_row; // store its global row
                columns[*count] = neighbor_column; // store its global col
                (*count)++;
            }
        }
    }
}

void fog_seed(uint32_t seed)
{
    fog_random_state = seed != 0 ? seed : 1;  // ensure non-zero seed
}

void apply_fog_serial(uint8_t *grid, int width, int height, double p_l)
{   // pick random cell
    int row = random_index(height);
    int column = random_index(width);
    // draw uniform random in [0,1] and compare to p_l to decide T/F for the "live" event
    if ((double)fog_random() / (double)UINT32_MAX < p_l) {
        grid[row * width + column] = 1; // TRUE: force chosen cell alive

        int live_neighbors = count_live_neighbors(grid, row, column, width, height);
        int required_neighbors = 2 - live_neighbors; // aim for 2 live neighbours total after spreading (exactly 2)

        if (required_neighbors > 0) { // spread life only if cell has < 2 alive neighbours
            int dead_rows[8]; // array for row indices of dead neighbours
            int dead_columns[8]; // array for col indices of dead neighbours
            int dead_count; // dead neighbours found

            collect_dead_neighbors(grid, row, column, width, height, dead_rows, dead_columns, &dead_count); // fill buffers

            shuffle_neighbors(dead_rows, dead_columns, dead_count); // randomize dead cells to revive

            if (required_neighbors > dead_count)
                required_neighbors = dead_count; // cap to available dead neighbors

            for (int i = 0; i < required_neighbors; i++) { // revive exactly required_neighbors dead cells (< 2)
                grid[dead_rows[i] * width + dead_columns[i]] = 1; // revive selected dead neighbors
            }
        }
    } else {
        grid[row * width + column] = 0; // FALSE: force chosen cell dead
    }
}

void apply_fog_parallel(uint8_t *grid, int width, int height, int local_rows, int start_row, int rank, int size, double p_l, MPI_Comm comm)
{
    int event[4] = {0, 0, 0, 0}; // event info: row, col, alive flag, owner rank
    int spread[18] = {0}; // data for neighbor revival: counts + coords

    if (rank == 0) {
        // pick random global FoG cell
        event[0] = random_index(height);
        event[1] = random_index(width);

        // draw uniform random in [0,1] and compare to p_l to decide T/F for the "live" event
        event[2] = (double)fog_random() / (double)UINT32_MAX < p_l;

        int base_rows = height / size; // base number of rows per rank
        int remainder = height % size; // extra rows distributed to first remainder ranks

        for (event[3] = 0; event[3] < size; event[3]++) { //search for owner rank
            int owner_start = event[3] * base_rows + (event[3] < remainder ? event[3] : remainder); // first global row of current rank
            int owner_rows = base_rows + (event[3] < remainder); // rows owned by current rank

            if (event[0] >= owner_start && event[0] < owner_start + owner_rows) // event row falls in current rank's range
                break;
        }
    }

    MPI_Bcast(event, 4, MPI_INT, 0, comm); // broadcast (row, col, alive, owner) to other ranks

    int event_row = event[0]; // global row FoG cell
    int event_column = event[1]; // global col FoG cell
    int event_alive = event[2]; // 1 if cell alive, 0 if dead
    int owner = event[3]; // rank owning the event row

    int required_neighbors = 0; // dead neighbours to revive
    int dead_count = 0; // dead neighbours around FoG cell
    int dead_rows[8] = {0}; // buffer for global row indices of dead neighbours
    int dead_columns[8] = {0}; // buffer for column indices of dead neighbours

    if (rank == owner) {
        int local_event_row = event_row - start_row; // convert global FoG row to local rank index

        if (event_alive) {
            grid[local_event_row * width + event_column] = 1; // TRUE: force chosen cell alive

            int live_neighbors = count_live_neighbors_parallel(grid, local_event_row, event_column, width); // count live neighbours
            required_neighbors = 2 - live_neighbors; // aim for 2 live neighbours total after spreading (exactly 2)

            if (required_neighbors > 0) { // spread life only if cell has < 2 alive neighbours
                collect_dead_neighbors_parallel(grid, event_row, event_column, width, height, start_row, local_rows, dead_rows, dead_columns, &dead_count); // fill buffers
            }
        } else {
            grid[local_event_row * width + event_column] = 0; // FALSE: force chosen cell dead
        }

        spread[0] = required_neighbors; // pack required_neighbors into spread buffer
        spread[1] = dead_count; // pack dead_count into spread buffer

        for (int i = 0; i < 8; i++) {
            spread[2 + i] = dead_rows[i]; // pack dead neighbour rows
            spread[10 + i] = dead_columns[i]; // pack dead neighbour cols
        }

        if (rank == 0 && event_alive && required_neighbors > 0) {
            shuffle_neighbors(dead_rows, dead_columns, dead_count); // randomize dead cells to revive

            for (int i = 0; i < 8; i++) {
                spread[2 + i] = dead_rows[i]; // update packed rows after shuffle
                spread[10 + i] = dead_columns[i]; // update packed cols after shuffle
            }
        }
    }

    if (owner != 0 && rank == owner)
        MPI_Send(spread, 18, MPI_INT, 0, 0, comm); // owner not rank 0 sends spread buffer to rank 0

    if (rank == 0 && owner != 0) {
        MPI_Recv(spread, 18, MPI_INT, owner, 0, comm, MPI_STATUS_IGNORE); // rank 0 receives spread buffer

        if (event_alive && required_neighbors > 0) {
            shuffle_neighbors(&spread[2], &spread[10], spread[1]); // randomize dead cells to revive
        }
    }

    MPI_Bcast(spread, 18, MPI_INT, 0, comm); // broadcast final spread buffer (counts + shuffled coords) to all ranks

    required_neighbors = spread[0]; // unpack how many neighbours to revive

    if (required_neighbors > 0) {
        for (int i = 0; i < required_neighbors; i++) {
            int global_row = spread[2 + i]; // global row of dead neighbour to revive
            int global_column = spread[10 + i]; // col of dead neighbour to revive

            if (global_row >= start_row && global_row < start_row + local_rows) { // if this neighbour belongs to current local portion
                int local_row = global_row - start_row; // convert global row to local index
                grid[local_row * width + global_column] = 1; // revive selected dead neighbour
            }
        }
    }
}