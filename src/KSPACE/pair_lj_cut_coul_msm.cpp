// clang-format off
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
   Contributing authors: Stan Moore (SNL), Paul Crozier (SNL)
------------------------------------------------------------------------- */

#include "pair_lj_cut_coul_msm.h"
#include <cmath>
#include <cstring>
#include "atom.h"
#include "force.h"
#include "kspace.h"
#include "neigh_list.h"
#include "memory.h"
#include "error.h"

using namespace LAMMPS_NS;

/* ---------------------------------------------------------------------- */

PairLJCutCoulMSM::PairLJCutCoulMSM(LAMMPS *lmp) : PairLJCutCoulLong(lmp)
{
  ewaldflag = pppmflag = 0;
  msmflag = 1;
  nmax = 0;
  ftmp = nullptr;
}

/* ---------------------------------------------------------------------- */

PairLJCutCoulMSM::~PairLJCutCoulMSM()
{
  if (ftmp) memory->destroy(ftmp);
}

/* ---------------------------------------------------------------------- */

void PairLJCutCoulMSM::compute(int eflag, int vflag)
{
  int i,ii,j,jj,inum,jnum,itype,jtype,itable;
  double qtmp,xtmp,ytmp,ztmp,delx,dely,delz,evdwl,ecoul,fpair,fcoul;
  double fraction,table;
  double r,r2inv,r6inv,forcecoul,forcelj,factor_coul,factor_lj;
  double egamma,fgamma,prefactor;
  int *ilist,*jlist,*numneigh,**firstneigh;
  double rsq;
  int eflag_old = eflag;

  if (force->kspace->scalar_pressure_flag && vflag) {
    if (vflag > 2)
      error->all(FLERR,"Must use 'kspace_modify pressure/scalar no' "
        "to obtain per-atom virial with kspace_style MSM");

    if (atom->nmax > nmax) {
      if (ftmp) memory->destroy(ftmp);
      nmax = atom->nmax;
      memory->create(ftmp,nmax,3,"pair:ftmp");
    }
    memset(&ftmp[0][0],0,nmax*3*sizeof(double));

    // must switch on global energy computation if not already on

    if (eflag == 0 || eflag == 2) {
      eflag++;
    }
  }

  evdwl = ecoul = 0.0;
  ev_init(eflag,vflag);

  double **x = atom->x;
  double **f = atom->f;
  double *q = atom->q;
  int *type = atom->type;
  int nlocal = atom->nlocal;
  double *special_coul = force->special_coul;
  double *special_lj = force->special_lj;
  int newton_pair = force->newton_pair;
  double qqrd2e = force->qqrd2e;

  inum = list->inum;
  ilist = list->ilist;
  numneigh = list->numneigh;
  firstneigh = list->firstneigh;

  // loop over neighbors of my atoms

  for (ii = 0; ii < inum; ii++) {
    i = ilist[ii];
    qtmp = q[i];
    xtmp = x[i][0];
    ytmp = x[i][1];
    ztmp = x[i][2];
    itype = type[i];
    jlist = firstneigh[i];
    jnum = numneigh[i];

    for (jj = 0; jj < jnum; jj++) {
      j = jlist[jj];
      factor_lj = special_lj[sbmask(j)];
      factor_coul = special_coul[sbmask(j)];
      j &= NEIGHMASK;

      delx = xtmp - x[j][0];
      dely = ytmp - x[j][1];
      delz = ztmp - x[j][2];
      rsq = delx*delx + dely*dely + delz*delz;
      jtype = type[j];

      if (rsq < cutsq[itype][jtype]) {
        r2inv = 1.0/rsq;

        if (rsq < cut_coulsq) {
          if (!ncoultablebits || rsq <= tabinnersq) {
            r = sqrt(rsq);
            prefactor = qqrd2e * qtmp*q[j]/r;
            egamma = 1.0 - (r/cut_coul)*force->kspace->gamma(r/cut_coul);
            fgamma = 1.0 + (rsq/cut_coulsq)*force->kspace->dgamma(r/cut_coul);
            forcecoul = prefactor * fgamma;
            if (factor_coul < 1.0) forcecoul -= (1.0-factor_coul)*prefactor;
          } else {
            union_int_float_t rsq_lookup;
            rsq_lookup.f = rsq;
            itable = rsq_lookup.i & ncoulmask;
            itable >>= ncoulshiftbits;
            fraction = ((double) rsq_lookup.f - rtable[itable]) * drtable[itable];
            table = ftable[itable] + fraction*dftable[itable];
            forcecoul = qtmp*q[j] * table;
            if (factor_coul < 1.0) {
              table = ctable[itable] + fraction*dctable[itable];
              prefactor = qtmp*q[j] * table;
              forcecoul -= (1.0-factor_coul)*prefactor;
            }
          }
        } else forcecoul = 0.0;

        if (rsq < cut_ljsq[itype][jtype]) {
          r6inv = r2inv*r2inv*r2inv;
          forcelj = r6inv * (lj1[itype][jtype]*r6inv - lj2[itype][jtype]);
        } else forcelj = 0.0;

        if (!(force->kspace->scalar_pressure_flag && vflag)) {
          fpair = (forcecoul + factor_lj*forcelj) * r2inv;

          f[i][0] += delx*fpair;
          f[i][1] += dely*fpair;
          f[i][2] += delz*fpair;
          if (newton_pair || j < nlocal) {
            f[j][0] -= delx*fpair;
            f[j][1] -= dely*fpair;
            f[j][2] -= delz*fpair;
          }
        } else {
          // separate LJ and Coulombic forces

          fpair = (factor_lj*forcelj) * r2inv;

          f[i][0] += delx*fpair;
          f[i][1] += dely*fpair;
          f[i][2] += delz*fpair;
          if (newton_pair || j < nlocal) {
            f[j][0] -= delx*fpair;
            f[j][1] -= dely*fpair;
            f[j][2] -= delz*fpair;
          }

          fcoul = (forcecoul) * r2inv;

          ftmp[i][0] += delx*fcoul;
          ftmp[i][1] += dely*fcoul;
          ftmp[i][2] += delz*fcoul;
          if (newton_pair || j < nlocal) {
            ftmp[j][0] -= delx*fcoul;
            ftmp[j][1] -= dely*fcoul;
            ftmp[j][2] -= delz*fcoul;
          }
        }

        if (eflag) {
          if (rsq < cut_coulsq) {
            if (!ncoultablebits || rsq <= tabinnersq)
              ecoul = prefactor*egamma;
            else {
              table = etable[itable] + fraction*detable[itable];
              ecoul = qtmp*q[j] * table;
            }
            if (factor_coul < 1.0) ecoul -= (1.0-factor_coul)*prefactor;
          } else ecoul = 0.0;

          if (eflag_old && rsq < cut_ljsq[itype][jtype]) {
            evdwl = r6inv*(lj3[itype][jtype]*r6inv-lj4[itype][jtype]) -
              offset[itype][jtype];
            evdwl *= factor_lj;
          } else evdwl = 0.0;
        }

        if (evflag) ev_tally(i,j,nlocal,newton_pair,
                             evdwl,ecoul,fpair,delx,dely,delz);
      }
    }
  }

  if (vflag_fdotr) virial_fdotr_compute();

  if (force->kspace->scalar_pressure_flag && vflag) {
    for (i = 0; i < 3; i++) virial[i] += force->pair->eng_coul/3.0;
    for (int i = 0; i < nmax; i++) {
      f[i][0] += ftmp[i][0];
      f[i][1] += ftmp[i][1];
      f[i][2] += ftmp[i][2];
    }
  }
}

/* ---------------------------------------------------------------------- */

void PairLJCutCoulMSM::compute_outer(int eflag, int vflag)
{
  int i,j,ii,jj,inum,jnum,itype,jtype,itable;
  double qtmp,xtmp,ytmp,ztmp,delx,dely,delz,evdwl,ecoul,fpair;
  double fraction,table;
  double r,r2inv,r6inv,forcecoul,forcelj,factor_coul,factor_lj;
  double egamma,fgamma,prefactor;
  double rsw;
  int *ilist,*jlist,*numneigh,**firstneigh;
  double rsq;

  if (force->kspace->scalar_pressure_flag)
   error->all(FLERR,"Must use 'kspace_modify pressure/scalar no' "
     "for rRESPA with kspace_style MSM");

  evdwl = ecoul = 0.0;
  ev_init(eflag,vflag);

  double **x = atom->x;
  double **f = atom->f;
  double *q = atom->q;
  int *type = atom->type;
  int nlocal = atom->nlocal;
  double *special_coul = force->special_coul;
  double *special_lj = force->special_lj;
  int newton_pair = force->newton_pair;
  double qqrd2e = force->qqrd2e;

  inum = list->inum;
  ilist = list->ilist;
  numneigh = list->numneigh;
  firstneigh = list->firstneigh;

  double cut_in_off = cut_respa[2];
  double cut_in_on = cut_respa[3];

  double cut_in_diff = cut_in_on - cut_in_off;
  double cut_in_off_sq = cut_in_off*cut_in_off;
  double cut_in_on_sq = cut_in_on*cut_in_on;

  // loop over neighbors of my atoms

  for (ii = 0; ii < inum; ii++) {
    i = ilist[ii];
    qtmp = q[i];
    xtmp = x[i][0];
    ytmp = x[i][1];
    ztmp = x[i][2];
    itype = type[i];
    jlist = firstneigh[i];
    jnum = numneigh[i];

    for (jj = 0; jj < jnum; jj++) {
      j = jlist[jj];
      factor_lj = special_lj[sbmask(j)];
      factor_coul = special_coul[sbmask(j)];
      j &= NEIGHMASK;

      delx = xtmp - x[j][0];
      dely = ytmp - x[j][1];
      delz = ztmp - x[j][2];
      rsq = delx*delx + dely*dely + delz*delz;
      jtype = type[j];

      if (rsq < cutsq[itype][jtype]) {
        r2inv = 1.0/rsq;

        if (rsq < cut_coulsq) {
          if (!ncoultablebits || rsq <= tabinnersq) {
            r = sqrt(rsq);
            prefactor = qqrd2e * qtmp*q[j]/r;
            egamma = 1.0 - (r/cut_coul)*force->kspace->gamma(r/cut_coul);
            fgamma = 1.0 + (rsq/cut_coulsq)*force->kspace->dgamma(r/cut_coul);
            forcecoul = prefactor * (fgamma - 1.0);
            if (rsq > cut_in_off_sq) {
              if (rsq < cut_in_on_sq) {
                rsw = (r - cut_in_off)/cut_in_diff;
                forcecoul += prefactor*rsw*rsw*(3.0 - 2.0*rsw);
                if (factor_coul < 1.0)
                  forcecoul -=
                    (1.0-factor_coul)*prefactor*rsw*rsw*(3.0 - 2.0*rsw);
              } else {
                forcecoul += prefactor;
                if (factor_coul < 1.0)
                  forcecoul -= (1.0-factor_coul)*prefactor;
              }
            }
          } else {
            union_int_float_t rsq_lookup;
            rsq_lookup.f = rsq;
            itable = rsq_lookup.i & ncoulmask;
            itable >>= ncoulshiftbits;
            fraction = ((double) rsq_lookup.f - rtable[itable]) * drtable[itable];
            table = ftable[itable] + fraction*dftable[itable];
            forcecoul = qtmp*q[j] * table;
            if (factor_coul < 1.0) {
              table = ctable[itable] + fraction*dctable[itable];
              prefactor = qtmp*q[j] * table;
              forcecoul -= (1.0-factor_coul)*prefactor;
            }
          }
        } else forcecoul = 0.0;

        if (rsq < cut_ljsq[itype][jtype] && rsq > cut_in_off_sq) {
          r6inv = r2inv*r2inv*r2inv;
          forcelj = r6inv * (lj1[itype][jtype]*r6inv - lj2[itype][jtype]);
          if (rsq < cut_in_on_sq) {
            rsw = (sqrt(rsq) - cut_in_off)/cut_in_diff;
            forcelj *= rsw*rsw*(3.0 - 2.0*rsw);
          }
        } else forcelj = 0.0;

        fpair = (forcecoul + forcelj) * r2inv;

        f[i][0] += delx*fpair;
        f[i][1] += dely*fpair;
        f[i][2] += delz*fpair;
        if (newton_pair || j < nlocal) {
          f[j][0] -= delx*fpair;
          f[j][1] -= dely*fpair;
          f[j][2] -= delz*fpair;
        }

        if (eflag) {
          if (rsq < cut_coulsq) {
            if (!ncoultablebits || rsq <= tabinnersq) {
              ecoul = prefactor*egamma;
              if (factor_coul < 1.0) ecoul -= (1.0-factor_coul)*prefactor;
            } else {
              table = etable[itable] + fraction*detable[itable];
              ecoul = qtmp*q[j] * table;
              if (factor_coul < 1.0) {
                table = ptable[itable] + fraction*dptable[itable];
                prefactor = qtmp*q[j] * table;
                ecoul -= (1.0-factor_coul)*prefactor;
              }
            }
          } else ecoul = 0.0;

          if (rsq < cut_ljsq[itype][jtype]) {
            r6inv = r2inv*r2inv*r2inv;
            evdwl = r6inv*(lj3[itype][jtype]*r6inv-lj4[itype][jtype]) -
              offset[itype][jtype];
            evdwl *= factor_lj;
          } else evdwl = 0.0;
        }

        if (vflag) {
          if (rsq < cut_coulsq) {
            if (!ncoultablebits || rsq <= tabinnersq) {
              forcecoul = prefactor * fgamma;
              if (factor_coul < 1.0) forcecoul -= (1.0-factor_coul)*prefactor;
            } else {
              table = vtable[itable] + fraction*dvtable[itable];
              forcecoul = qtmp*q[j] * table;
              if (factor_coul < 1.0) {
                table = ptable[itable] + fraction*dptable[itable];
                prefactor = qtmp*q[j] * table;
                forcecoul -= (1.0-factor_coul)*prefactor;
              }
            }
          } else forcecoul = 0.0;

          if (rsq <= cut_in_off_sq) {
            r6inv = r2inv*r2inv*r2inv;
            forcelj = r6inv * (lj1[itype][jtype]*r6inv - lj2[itype][jtype]);
          } else if (rsq <= cut_in_on_sq) {
            r6inv = r2inv*r2inv*r2inv;
            forcelj = r6inv * (lj1[itype][jtype]*r6inv - lj2[itype][jtype]);
          }
          fpair = (forcecoul + factor_lj*forcelj) * r2inv;
        }

        if (evflag) ev_tally(i,j,nlocal,newton_pair,
                             evdwl,ecoul,fpair,delx,dely,delz);
      }
    }
  }
}

/* ---------------------------------------------------------------------- */

double PairLJCutCoulMSM::single(int i, int j, int itype, int jtype,
                                 double rsq,
                                 double factor_coul, double factor_lj,
                                 double &fforce)
{
  double r2inv,r6inv,r,egamma,fgamma,prefactor;
  double fraction,table,forcecoul,forcelj,phicoul,philj;
  int itable;

  r2inv = 1.0/rsq;
  if (rsq < cut_coulsq) {
    if (!ncoultablebits || rsq <= tabinnersq) {
      r = sqrt(rsq);
      prefactor = force->qqrd2e * atom->q[i]*atom->q[j]/r;
      egamma = 1.0 - (r/cut_coul)*force->kspace->gamma(r/cut_coul);
      fgamma = 1.0 + (rsq/cut_coulsq)*force->kspace->dgamma(r/cut_coul);
      forcecoul = prefactor * fgamma;
      if (factor_coul < 1.0) forcecoul -= (1.0-factor_coul)*prefactor;
    } else {
      union_int_float_t rsq_lookup_single;
      rsq_lookup_single.f = rsq;
      itable = rsq_lookup_single.i & ncoulmask;
      itable >>= ncoulshiftbits;
      fraction = ((double) rsq_lookup_single.f - rtable[itable]) * drtable[itable];
      table = ftable[itable] + fraction*dftable[itable];
      forcecoul = atom->q[i]*atom->q[j] * table;
      if (factor_coul < 1.0) {
        table = ctable[itable] + fraction*dctable[itable];
        prefactor = atom->q[i]*atom->q[j] * table;
        forcecoul -= (1.0-factor_coul)*prefactor;
      }
    }
  } else forcecoul = 0.0;

  if (rsq < cut_ljsq[itype][jtype]) {
    r6inv = r2inv*r2inv*r2inv;
    forcelj = r6inv * (lj1[itype][jtype]*r6inv - lj2[itype][jtype]);
  } else forcelj = 0.0;

  fforce = (forcecoul + factor_lj*forcelj) * r2inv;

  double eng = 0.0;
  if (rsq < cut_coulsq) {
    if (!ncoultablebits || rsq <= tabinnersq)
      phicoul = prefactor*egamma;
    else {
      table = etable[itable] + fraction*detable[itable];
      phicoul = atom->q[i]*atom->q[j] * table;
    }
    if (factor_coul < 1.0) phicoul -= (1.0-factor_coul)*prefactor;
    eng += phicoul;
  }

  if (rsq < cut_ljsq[itype][jtype]) {
    philj = r6inv*(lj3[itype][jtype]*r6inv-lj4[itype][jtype]) -
      offset[itype][jtype];
    eng += factor_lj*philj;
  }

  return eng;
}

/* ---------------------------------------------------------------------- */

void *PairLJCutCoulMSM::extract(const char *str, int &dim)
{
  dim = 0;
  if (strcmp(str,"cut_coul") == 0) return (void *) &cut_coul;
  dim = 2;
  if (strcmp(str,"epsilon") == 0) return (void *) epsilon;
  if (strcmp(str,"sigma") == 0) return (void *) sigma;
  return nullptr;
}
