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

/* ----------------------------------------------------------------------
   Contributing author: Mingjian Wen (University of Minnesota)
   e-mail: wenxx151@umn.edu

   This implements the DRIP model as described in
   M. Wen, S. Carr, S. Fang, E. Kaxiras, and E. B. Tadmor,
   Phys. Rev. B, 98, 235404 (2018).
------------------------------------------------------------------------- */

#ifdef PAIR_CLASS
// clang-format off
PairStyle(drip, PairDRIP);
// clang-format on
#else

#ifndef LMP_PAIR_DRIP_H
#define LMP_PAIR_DRIP_H

#include "pair.h"

namespace LAMMPS_NS {

class PairDRIP : public Pair {
 public:
  PairDRIP(class LAMMPS *);
  ~PairDRIP() override;

  void compute(int, int) override;
  void settings(int, char **) override;
  void coeff(int, char **) override;
  double init_one(int, int) override;
  void init_style() override;

  static constexpr int NPARAMS_PER_LINE = 15;
  using V3 = double[3];

 protected:
  struct Param {
    int ielement, jelement;
    double C0, C2, C4, C, delta, lambda, A, z0, B, eta, rhocut, rcut, ncut;
    double rhocutsq, rcutsq, ncutsq;
  };
  Param *params;          // parameter set for I-J interactions
  int **nearest3neigh;    // nearest 3 neighbors of atoms
  double cutmax;          // max cutoff for all species

  void read_file(char *);
  void allocate();

  // DRIP specific functions
  double calc_attractive(Param &, double const, double const *, double *const, double *const);

  double calc_repulsive(int const, int const, Param &, double const, double const *, double const *,
                        V3 const *, V3 const *, V3 const *, V3 const *, double *const,
                        double *const);

  void find_nearest3neigh();

  void calc_normal(int const, double *const, V3 *const, V3 *const, V3 *const, V3 *const);

  void get_drhosqij(double const *, double const *, V3 const *, V3 const *, V3 const *, V3 const *,
                    double *const, double *const, double *const, double *const, double *const);

  double td(double, double, double, double, double const *const, double, const double *const,
            double &, double &);

  double dihedral(const int, const int, Param &, double const, double &, double *const,
                  double *const, double *const, double *const, double *const, double *const,
                  double *const, double *const);

  double deriv_cos_omega(double const *, double const *, double const *, double const *,
                         double *const, double *const, double *const, double *const);

  double tap(double, double, double &);

  double tap_rho(double, double, double &);

  void deriv_cross(double const *, double const *, double const *, double *const, V3 *const,
                   V3 *const, V3 *const);
};
}    // namespace LAMMPS_NS

#endif
#endif
