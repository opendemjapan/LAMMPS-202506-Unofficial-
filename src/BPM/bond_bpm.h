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

#ifndef LMP_BOND_BPM_H
#define LMP_BOND_BPM_H

#include "bond.h"

#include <vector>

namespace LAMMPS_NS {

class Fix;

class BondBPM : public Bond {
 public:
  BondBPM(class LAMMPS *);
  ~BondBPM() override;
  void compute(int, int) override = 0;
  void coeff(int, char **) override = 0;
  void init_style() override;
  void settings(int, char **) override;
  double equilibrium_distance(int) override;
  void write_restart(FILE *) override;
  void read_restart(FILE *) override;
  double single(int, double, int, int, double &) override = 0;

 protected:
  double r0_max_estimate;
  double max_stretch;
  int store_local_freq, nhistory, update_flag, hybrid_flag;

  std::vector<int> leftover_iarg;

  char *id_fix_dummy_special, *id_fix_dummy_history;
  char *id_fix_update_special_bonds, *id_fix_bond_history;
  char *id_fix_store_local, *id_fix_property_atom;
  class FixStoreLocal *fix_store_local;
  class FixBondHistory *fix_bond_history;
  class FixUpdateSpecialBonds *fix_update_special_bonds;

  void process_broken(int, int);
  using FnPtrPack = void (BondBPM::*)(int, int, int);
  FnPtrPack *pack_choice;    // ptrs to pack functions
  double *output_data;

  int property_atom_flag, nvalues, overlay_flag, break_flag, ignore_special_flag;
  int index_x_ref, index_y_ref, index_z_ref;

  int n_histories;
  std::vector<Fix *> histories;

  void pack_id1(int, int, int);
  void pack_id2(int, int, int);
  void pack_time(int, int, int);
  void pack_x(int, int, int);
  void pack_y(int, int, int);
  void pack_z(int, int, int);
  void pack_x_ref(int, int, int);
  void pack_y_ref(int, int, int);
  void pack_z_ref(int, int, int);
};

}    // namespace LAMMPS_NS

#endif
