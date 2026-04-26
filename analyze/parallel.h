#ifndef _PARALLEL_H
#define _PARALLEL_H

#define OPENMP 1

// intel cilk+
#if CILK == 1
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#define p_for cilk_for
#define parallel_main main
#ifdef __clang__
#define p_for_1 _Pragma("cilk grainsize 1") cilk_for
#define p_for_8 _Pragma("cilk grainsize 8") cilk_for
#define p_for_32 _Pragma("cilk grainsize 32") cilk_for
#define p_for_256 _Pragma("cilk grainsize 256") cilk_for
#define p_for_2048 _Pragma("cilk grainsize 2048") cilk_for
#else
#define p_for_1 _Pragma("cilk grainsize = 1") cilk_for
#define p_for_8 _Pragma("cilk grainsize = 8") cilk_for
#define p_for_32 _Pragma("cilk grainsize = 8") cilk_for
#define p_for_256 _Pragma("cilk grainsize = 256") cilk_for
#define p_for_2048 _Pragma("cilk grainsize = 2048") cilk_for
#endif
[[maybe_unused]] static int getWorkers() { return __cilkrts_get_nworkers(); }

[[maybe_unused]] static int getWorkerNum() {
  return __cilkrts_get_worker_number();
}

// openmp
#elif OPENMP == 1
#include <omp.h>
#define cilk_spawn
#define cilk_sync
#define parallel_main main
#define p_for _Pragma("omp parallel for") for
#define p_for_1 _Pragma("omp parallel for schedule (static,1)") for
#define p_for_8 _Pragma("omp parallel for schedule (static,8)") for
#define p_for_32 _Pragma("omp parallel for schedule (static,8)") for
#define p_for_256 _Pragma("omp parallel for schedule (static,256)") for
#define p_for_2048 _Pragma("omp parallel for schedule (static,2048)") for

[[maybe_unused]] static int getWorkers() { return omp_get_max_threads(); }
[[maybe_unused]] static int getWorkerNum() { return omp_get_thread_num(); }

// c++
#else
#define cilk_spawn
#define cilk_sync
#define parallel_main main
#define p_for for
#define p_for_1 for
#define p_for_8 for
#define p_for_32 for
#define p_for_256 for
#define p_for_2048 for
#define cilk_for for

[[maybe_unused]] static int getWorkers() { return 1; }
[[maybe_unused]] static int getWorkerNum() { return 0; }

#endif

#endif // _PARALLEL_H
