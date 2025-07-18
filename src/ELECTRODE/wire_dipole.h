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

/* ----------------------------------------------------------------------
   Contributing authors: Ludwig Ahrens-Iwers (TUHH), Shern Tee (UQ), Robert Meissner (TUHH)
------------------------------------------------------------------------- */

#ifndef LMP_WIRE_DIPOLE_H
#define LMP_WIRE_DIPOLE_H

#include "boundary_correction.h"

namespace LAMMPS_NS {

class WireDipole : public BoundaryCorrection {
 public:
  WireDipole(LAMMPS *);
  void vector_corr(double *, int, int, bool) override;
  void matrix_corr(bigint *, double **) override;
  void compute_corr(double, int, int, double &, double *) override;
  void setup(double);
};

}    // namespace LAMMPS_NS
#endif
