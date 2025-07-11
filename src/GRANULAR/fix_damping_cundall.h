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

#ifdef FIX_CLASS
// clang-format off
FixStyle(damping/cundall,FixDampingCundall);
// clang-format on
#else

#ifndef LMP_FIX_DAMPING_CUNDALL_H
#define LMP_FIX_DAMPING_CUNDALL_H

#include "fix.h"

namespace LAMMPS_NS {

class FixDampingCundall : public Fix {
 public:
  FixDampingCundall(class LAMMPS *, int, char **);
  ~FixDampingCundall() override;
  int setmask() override;
  void init() override;
  void setup(int) override;
  void min_setup(int) override;
  void post_force(int) override;
  void post_force_respa(int, int, int) override;
  void min_post_force(int) override;
  double memory_usage() override;

 protected:
  double gamma_lin, gamma_ang, *scalegamma, *scaleval;
  const char *scalevarid;
  int scalestyle, scalevar;
  int ilevel_respa;
};

}    // namespace LAMMPS_NS

#endif
#endif
