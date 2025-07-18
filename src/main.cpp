/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "lammps.h"

#include "exceptions.h"
#include "input.h"
#include "library.h"

#include "json.h"

#include <cstdlib>
#include <mpi.h>
#include <new>

// import MolSSI Driver Interface library
#if defined(LMP_MDI)
#include <mdi.h>
#endif

using namespace LAMMPS_NS;

// for convenience
static void finalize()
{
  lammps_kokkos_finalize();
  lammps_python_finalize();
  lammps_plugin_finalize();
}

/* ----------------------------------------------------------------------
   main program to drive LAMMPS
------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  MPI_Comm lammps_comm = MPI_COMM_WORLD;

#if defined(LMP_MDI)
  // initialize MDI interface, if compiled in

  int mdi_flag;
  if (MDI_Init(&argc, &argv)) MPI_Abort(MPI_COMM_WORLD, 1);
  if (MDI_Initialized(&mdi_flag)) MPI_Abort(MPI_COMM_WORLD, 1);

  // get the MPI communicator that spans all ranks running LAMMPS
  // when using MDI, this may be a subset of MPI_COMM_WORLD

  if (mdi_flag)
    if (MDI_MPI_get_world_comm(&lammps_comm)) MPI_Abort(MPI_COMM_WORLD, 1);
#endif

  try {
    auto lammps = new LAMMPS(argc, argv, lammps_comm);
    lammps->input->file();
    delete lammps;
  } catch (LAMMPSAbortException &ae) {
    finalize();
    MPI_Abort(ae.get_universe(), 1);
  } catch (LAMMPSException &) {
    finalize();
    MPI_Barrier(lammps_comm);
    MPI_Finalize();
    exit(1);
  } catch (fmt::format_error &fe) {
    fprintf(stderr, "\nfmt::format_error: %s%s\n", fe.what(), utils::errorurl(12).c_str());
    finalize();
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  } catch (json::exception &je) {
    fprintf(stderr, "\nJSON library error %d: %s\n", je.id, je.what());
    finalize();
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  } catch (std::bad_alloc &ae) {
    fprintf(stderr, "C++ memory allocation failed: %s\n", ae.what());
    finalize();
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  } catch (std::exception &e) {
    fprintf(stderr, "Exception: %s\n", e.what());
    finalize();
    MPI_Abort(MPI_COMM_WORLD, 1);
    exit(1);
  }
  finalize();
  MPI_Barrier(lammps_comm);
  MPI_Finalize();
}
