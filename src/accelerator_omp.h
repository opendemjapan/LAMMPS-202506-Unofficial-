/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

// NOTE: this file is *supposed* to be included multiple times

#ifdef LMP_OPENMP

// true interface to OPENMP

// provide a DomainOMP class with some overrides for Domain
#include "domain.h"

#ifndef LMP_DOMAIN_OMP_H
#define LMP_DOMAIN_OMP_H

namespace LAMMPS_NS {

class DomainOMP : public Domain {
 public:
  DomainOMP(class LAMMPS *lmp) : Domain(lmp) {}
  ~DomainOMP() override = default;

  // multi-threaded versions
  void pbc() override;
  void lamda2x(int) override;
  void lamda2x(double *lamda, double *x) override { Domain::lamda2x(lamda, x); }
  void x2lamda(int) override;
  void x2lamda(double *x, double *lamda) override { Domain::x2lamda(x, lamda); }
};
}    // namespace LAMMPS_NS

#endif /* LMP_DOMAIN_OMP_H */

#endif /* !LMP_OPENMP */
