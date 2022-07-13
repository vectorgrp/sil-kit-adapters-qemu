// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <algorithm>
#include <cstring>
#include <fstream>
#include <thread>
#include <atomic>
#include <mutex>


using namespace std::chrono_literals;

extern "C"
{
#include "qemu-plugin.h"
#include "ExecutionManager.hpp"
#include "Options.hpp"

    QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;
    QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc, char **argv);
}

/* Information about a translated block */
typedef struct
{
    uint64_t pc;
    uint64_t insns;
} BlockInfo;

std::atomic<uint64_t> instructionCount{};
std::atomic<uint64_t> innerEventCount{0};

struct timespec start[2], stop[2];

std::unique_ptr<ExecutionManager> executionManager;
std::unique_ptr<Options> options;

std::mutex myMutex;

static void vcpu_tb_exec_icount(unsigned int cpu_index, void *udata)
{
    BlockInfo *bi = (BlockInfo *)udata;
    instructionCount += bi->insns;

    if (instructionCount > options->AdvanceTimeInMicroSeconds * 1000)
    {
        executionManager->SendFinishedSignal();

        // Blocks execution
        executionManager->WaitForContinueSignal();

        instructionCount = 0;
    }
}

static void vcpu_tb_exec_timeMeasurement(unsigned int cpu_index, void *udata)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &stop[cpu_index]);
    double result = (stop[cpu_index].tv_sec - start[cpu_index].tv_sec) * 1e6
                    + (stop[cpu_index].tv_nsec - start[cpu_index].tv_nsec) / 1e3; // in microseconds
    if (result > 100)
    {
        std::lock_guard guard(myMutex);
        executionManager->SendFinishedSignal();

        executionManager->WaitForContinueSignal();

        clock_gettime(CLOCK_MONOTONIC_RAW, &start[0]);
        clock_gettime(CLOCK_MONOTONIC_RAW, &start[1]);

        instructionCount = 0;
    }
}

static void vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    BlockInfo *bi = new BlockInfo;
    bi->pc = qemu_plugin_tb_vaddr(tb);
    bi->insns = qemu_plugin_tb_n_insns(tb);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start[0]);
    clock_gettime(CLOCK_MONOTONIC_RAW, &start[1]);

    qemu_plugin_register_vcpu_tb_exec_cb(tb, vcpu_tb_exec_icount, QEMU_PLUGIN_CB_NO_REGS, bi);
}

qemu_plugin_id_t pluginId;

static void plugin_exit(qemu_plugin_id_t id, void *p)
{
    qemu_plugin_outs("Plugin exited successfully.\n");
    qemu_plugin_uninstall(pluginId, nullptr);
}

extern "C"
{
    QEMU_PLUGIN_EXPORT int qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc, char **argv)
    {
        pluginId = id;
        options = std::make_unique<Options>(argc, argv);
        executionManager = std::make_unique<ExecutionManager>(options->HostToGuestPipe, options->GuestToHostPipe);

        qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
        qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);

        qemu_plugin_outs("Successfully installed plugin.\n");

        return 0;
    }
}
