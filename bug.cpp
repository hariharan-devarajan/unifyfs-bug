#include <unifyfs.h>
#include <unifyfs/unifyfs_api.h>
#include <string>
#include <cstring>
#include <experimental/filesystem>
#include <cassert>
#include <mpi.h>

namespace fs = std::experimental::filesystem;

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int comm_rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    auto request_size = 4096, iteration = 1024;
    std::string unifyfs_path = "/unifyfs", filename = "test.dat";
    unifyfs_handle fshdl;
    int options_ct = 0;
    char unifyfs_consistency[32] = "LAMINATED";
    options_ct++;
    char client_fsync_persist[32] = "off";
    options_ct++;
    char logio_chunk_size[32];
    strcpy(logio_chunk_size, std::to_string(1024L * 1024L).c_str());

    options_ct++;
    char logio_shmem_size[32];
    strcpy(logio_shmem_size, std::to_string(1024L * 1024L * 1024L ).c_str());
    options_ct++;
    char logio_spill_dir[256];
    fs::path splill_dir;
    splill_dir = std::string("/dev/shm");
    strcpy(logio_spill_dir, splill_dir.c_str());
    options_ct++;
    char logio_spill_size[32];

        strcpy(logio_spill_size,
               std::to_string(request_size * iteration + 1024L * 1024L)
                       .c_str());
    options_ct++;
    char client_local_extents[32];
    char client_node_local_extents[32];
    strcpy(client_local_extents, "off");
    options_ct++;
    strcpy(client_node_local_extents, "off");
    options_ct++;

    unifyfs_cfg_option *options = static_cast<unifyfs_cfg_option *>(
            calloc(options_ct, sizeof(unifyfs_cfg_option)));
    options[0] = {.opt_name = "unifyfs.consistency",
            .opt_value = unifyfs_consistency};
    options[1] = {.opt_name = "client.fsync_persist",
            .opt_value = client_fsync_persist};
    options[2] = {.opt_name = "logio.chunk_size",
            .opt_value = logio_chunk_size};
    options[3] = {.opt_name = "logio.shmem_size",
            .opt_value = logio_shmem_size};
    options[4] = {.opt_name = "logio.spill_dir", .opt_value = logio_spill_dir};
    options[5] = {.opt_name = "logio.spill_size",
            .opt_value = logio_spill_size};
    options[6] = {.opt_name = "client.local_extents",
            .opt_value = client_local_extents};
    options[7] = {.opt_name = "client.node_local_extents",
            .opt_value = client_node_local_extents};
    const char *val = unifyfs_path.c_str();

    int rc = unifyfs_initialize(val, options, options_ct, &fshdl);
    assert(rc == UNIFYFS_SUCCESS);
    fs::path unifyfs_filename = fs::path(unifyfs_path) / filename;
    unifyfs_gfid gfid = 0;
    filename = filename + "_" + std::to_string(comm_rank) + "_of_" +
               std::to_string(comm_size);
    rc = UNIFYFS_SUCCESS;
    int create_flags = 0;
    rc = unifyfs_create(fshdl, create_flags, unifyfs_filename.c_str(), &gfid);

    assert(rc == UNIFYFS_SUCCESS);
    assert(gfid != UNIFYFS_INVALID_GFID);
    /* Write data to file */
    auto write_data =
            std::vector<char>(request_size * iteration, 'w');
    size_t write_req_ct = iteration + 1;
    unifyfs_io_request write_req[write_req_ct];
    for (size_t i = 0; i < iteration; ++i) {
        write_req[i].op = UNIFYFS_IOREQ_OP_WRITE;
        write_req[i].gfid = gfid;
        write_req[i].nbytes = request_size;
        off_t base_offset = 0;
        off_t relative_offset = i * request_size;
        write_req[i].offset = base_offset + relative_offset;
        write_req[i].user_buf = write_data.data() + (i * request_size);
    }
    write_req[iteration].op = UNIFYFS_IOREQ_OP_SYNC_META;
    write_req[iteration].gfid = gfid;

    rc = unifyfs_dispatch_io(fshdl, write_req_ct, write_req);

    if (rc == UNIFYFS_SUCCESS) {
        int waitall = 1;

        rc = unifyfs_wait_io(fshdl, write_req_ct, write_req, waitall);

        if (rc == UNIFYFS_SUCCESS) {
            for (size_t i = 0; i < iteration; i++) {
                assert(write_req[i].result.error == 0);
                assert(write_req[i].result.count == request_size);
            }
            assert(write_req[iteration].result.error == 0);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);

    fs::path full_filename_path;
    full_filename_path = fs::path("/dev/shm") / filename;

    unifyfs_transfer_request mv_req;
    mv_req.src_path = unifyfs_filename.c_str();
    mv_req.dst_path = full_filename_path.c_str();
    mv_req.mode = UNIFYFS_TRANSFER_MODE_COPY;
    mv_req.use_parallel = 1;
    rc = unifyfs_dispatch_transfer(fshdl, 1, &mv_req);
    assert(rc == UNIFYFS_SUCCESS);
    if (rc == UNIFYFS_SUCCESS) {
        int waitall = 1;
        rc = unifyfs_wait_transfer(fshdl, 1, &mv_req, waitall);
        if (rc == UNIFYFS_SUCCESS) {
            for (int i = 0; i < (int)1; i++) {
                assert(mv_req.result.error == 0);
            }
        }
    }


    MPI_Barrier(MPI_COMM_WORLD);

    rc = unifyfs_finalize(fshdl);
    assert(rc == UNIFYFS_SUCCESS);
    MPI_Barrier(MPI_COMM_WORLD);
    if (comm_rank == 0) {
        assert(fs::file_size(full_filename_path) ==
               request_size * iteration);
    }
    printf("Done Test\n");
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
