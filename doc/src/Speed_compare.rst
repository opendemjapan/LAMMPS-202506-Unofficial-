Comparison of various accelerator packages
==========================================

The next section compares and contrasts the various accelerator
options, since there are multiple ways to perform OpenMP threading,
run on GPUs, optimize for vector units on CPUs and run on Intel
Xeon Phi (co-)processors.

All of these packages can accelerate a LAMMPS calculation taking
advantage of hardware features, but they do it in different ways
and acceleration is not always guaranteed.

As a consequence, for a particular simulation on specific hardware,
one package may be faster than the other.  We give some guidelines
below, but the best way to determine which package is faster for your
input script is to try multiple of them on your machine and experiment
with available performance tuning settings.  See the benchmarking
section below for examples where this has been done.

**Guidelines for using each package optimally:**

* Both, the GPU and the KOKKOS package allows you to assign multiple
  MPI ranks (= CPU cores) to the same GPU. For the GPU package, this
  can lead to a speedup through better utilization of the GPU (by
  overlapping computation and data transfer) and more efficient
  computation of the non-GPU accelerated parts of LAMMPS through MPI
  parallelization, as all system data is maintained and updated on
  the host. For KOKKOS, there is less to no benefit from this, due
  to its different memory management model, which tries to retain
  data on the GPU.
* The GPU package moves per-atom data (coordinates, forces, and
  (optionally) neighbor list data, if not computed on the GPU) between
  the CPU and GPU at every timestep.  The KOKKOS/CUDA package only does
  this on timesteps when a CPU calculation is required (e.g. to invoke
  a fix or compute that is non-GPU-ized). Hence, if you can formulate
  your input script to only use GPU-ized fixes and computes, and avoid
  doing I/O too often (thermo output, dump file snapshots, restart files),
  then the data transfer cost of the KOKKOS/CUDA package can be very low,
  causing it to run faster than the GPU package.
* The GPU package is often faster than the KOKKOS/CUDA package, when the
  number of atoms per GPU is on the smaller side.  The crossover point,
  in terms of atoms/GPU at which the KOKKOS/CUDA package becomes faster
  depends strongly on the pair style.  For example, for a simple Lennard Jones
  system the crossover (in single precision) is often about 50K-100K
  atoms per GPU.  When performing double precision calculations the
  crossover point can be significantly smaller.
* When using LAMMPS with multiple MPI ranks assigned to the same GPU, its
  performance depends to some extent on the available bandwidth between
  the CPUs and the GPU. This can differ significantly based on the
  available bus technology, capability of the host CPU and mainboard,
  the wiring of the buses and whether switches are used to increase the
  number of available bus slots, or if GPUs are housed in an external
  enclosure.  This can become quite complex.
* To achieve significant acceleration through GPUs, both KOKKOS and GPU
  package require capable GPUs with fast on-device memory and efficient
  data transfer rates. This requests capable upper mid-level to high-end
  (desktop) GPUs. Using lower performance GPUs (e.g. on laptops) may
  result in a slowdown instead.
* For the GPU package, specifically when running in parallel with MPI,
  if it often more efficient to exclude the PPPM kspace style from GPU
  acceleration and instead run it - concurrently with a GPU accelerated
  pair style - on the CPU. This can often be easily achieved with placing
  a *suffix off* command before and a *suffix on* command after the
  *kspace_style pppm* command.
* The KOKKOS/OpenMP and OPENMP package have different thread management
  strategies, which should result in OPENMP being more efficient for a
  small number of threads with increasing overhead as the number of threads
  per MPI rank grows. The KOKKOS/OpenMP kernels have less overhead in that
  case, but have lower performance with few threads.
* The INTEL package contains many options and settings for achieving
  additional performance on Intel hardware (CPU and accelerator cards), but
  to unlock this potential, an Intel compiler is required. The package code
  will compile with GNU gcc, but it will not be as efficient.

**Differences between the GPU and KOKKOS packages:**

* The GPU package accelerates only pair force, neighbor list, and (parts
  of) PPPM calculations (and runs the remaining force computations on
  the CPU concurrently).  The KOKKOS package attempts to run most of the
  calculation on the GPU, but can transparently support non-accelerated
  code (with a performance penalty due to having data transfers between
  host and GPU).
* The list of which styles are accelerated by the GPU or KOKKOS package
  differs with some overlap.
* The GPU package requires neighbor lists to be built on the CPU when using
  hybrid pair styles, exclusion lists, or a triclinic simulation box.
* The GPU package benefits from running multiple MPI processes (2-8) per
  GPU to parallelize the non-GPU accelerated styles.  The KOKKOS package
  usually not, especially when all parts of the calculation have KOKKOS
  support.
* The GPU package can be compiled for CUDA, HIP, or OpenCL and thus
  supports NVIDIA, AMD, and Intel GPUs well. On NVIDIA or AMD hardware,
  using native CUDA or HIP compilation, respectively, with either GPU or
  KOKKOS results in equal or better performance over OpenCL.
* OpenCL in the GPU package supports NVIDIA, AMD, and Intel GPUs at the
  *same time* and with the *same executable*.  KOKKOS currently does not
  support OpenCL.
* The GPU package supports single precision floating point, mixed
  precision floating point, and double precision floating point math on
  the GPU.  This must be chosen at compile time.  KOKKOS currently only
  supports double precision floating point math.  Using single or mixed
  precision (recommended) results in significantly improved performance
  on consumer GPUs for some loss in accuracy (which is rather small with
  mixed precision).  Single and mixed precision support for KOKKOS is in
  development (no ETA yet).
* Some pair styles (for example :doc:`snap <pair_snap>`, :doc:`mliap
  <pair_mliap>` or :doc:`reaxff <pair_reaxff>` in the KOKKOS package have
  seen extensive optimizations and specializations for GPUs and CPUs.
