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

#include "angle_hybrid.h"

#include "atom.h"
#include "comm.h"
#include "error.h"
#include "force.h"
#include "memory.h"
#include "neighbor.h"

#include <cstring>

using namespace LAMMPS_NS;

static constexpr int EXTRA = 1000;

/* ---------------------------------------------------------------------- */

AngleHybrid::AngleHybrid(LAMMPS *lmp) : Angle(lmp)
{
  writedata = 0;
  nstyles = 0;
  nanglelist = nullptr;
  maxangle = nullptr;
  anglelist = nullptr;
}

/* ---------------------------------------------------------------------- */

AngleHybrid::~AngleHybrid()
{
  if (nstyles) {
    for (int i = 0; i < nstyles; i++) delete styles[i];
    delete[] styles;
    for (int i = 0; i < nstyles; i++) delete[] keywords[i];
    delete[] keywords;
  }

  AngleHybrid::deallocate();
}

/* ---------------------------------------------------------------------- */

void AngleHybrid::compute(int eflag, int vflag)
{
  int i, j, m, n;

  // save ptrs to original anglelist

  int nanglelist_orig = neighbor->nanglelist;
  int **anglelist_orig = neighbor->anglelist;

  // if this is re-neighbor step, create sub-style anglelists
  // nanglelist[] = length of each sub-style list
  // realloc sub-style anglelist if necessary
  // load sub-style anglelist with 4 values from original anglelist

  if (neighbor->ago == 0) {
    for (m = 0; m < nstyles; m++) nanglelist[m] = 0;
    for (i = 0; i < nanglelist_orig; i++) {
      m = map[anglelist_orig[i][3]];
      if (m >= 0) nanglelist[m]++;
    }
    for (m = 0; m < nstyles; m++) {
      if (nanglelist[m] > maxangle[m]) {
        memory->destroy(anglelist[m]);
        maxangle[m] = nanglelist[m] + EXTRA;
        memory->create(anglelist[m], maxangle[m], 4, "angle_hybrid:anglelist");
      }
      nanglelist[m] = 0;
    }
    for (i = 0; i < nanglelist_orig; i++) {
      m = map[anglelist_orig[i][3]];
      if (m < 0) continue;
      n = nanglelist[m];
      anglelist[m][n][0] = anglelist_orig[i][0];
      anglelist[m][n][1] = anglelist_orig[i][1];
      anglelist[m][n][2] = anglelist_orig[i][2];
      anglelist[m][n][3] = anglelist_orig[i][3];
      nanglelist[m]++;
    }
  }

  // call each sub-style's compute function
  // set neighbor->anglelist to sub-style anglelist before call
  // accumulate sub-style global/peratom energy/virial in hybrid

  ev_init(eflag, vflag);

  // need to clear per-thread storage here, when using multiple threads
  // with thread-enabled substyles to avoid uninitlialized data access.

  const int nthreads = comm->nthreads;
  if (comm->nthreads > 1) {
    const bigint nall = atom->nlocal + atom->nghost;
    if (eflag_atom) memset(&eatom[0], 0, nall * nthreads * sizeof(double));
    if (vflag_atom) memset(&vatom[0][0], 0, 6 * nall * nthreads * sizeof(double));
  }

  for (m = 0; m < nstyles; m++) {
    neighbor->nanglelist = nanglelist[m];
    neighbor->anglelist = anglelist[m];

    styles[m]->compute(eflag, vflag);

    if (eflag_global) energy += styles[m]->energy;
    if (vflag_global)
      for (n = 0; n < 6; n++) virial[n] += styles[m]->virial[n];
    if (eflag_atom) {
      n = atom->nlocal;
      if (force->newton_bond) n += atom->nghost;
      double *eatom_substyle = styles[m]->eatom;
      for (i = 0; i < n; i++) eatom[i] += eatom_substyle[i];
    }
    if (vflag_atom) {
      n = atom->nlocal;
      if (force->newton_bond) n += atom->nghost;
      double **vatom_substyle = styles[m]->vatom;
      for (i = 0; i < n; i++)
        for (j = 0; j < 6; j++) vatom[i][j] += vatom_substyle[i][j];
    }
    if (cvflag_atom) {
      n = atom->nlocal;
      if (force->newton_bond) n += atom->nghost;
      double **cvatom_substyle = styles[m]->cvatom;
      for (i = 0; i < n; i++)
        for (j = 0; j < 9; j++) cvatom[i][j] += cvatom_substyle[i][j];
    }
  }

  // restore ptrs to original anglelist

  neighbor->nanglelist = nanglelist_orig;
  neighbor->anglelist = anglelist_orig;
}

/* ---------------------------------------------------------------------- */

void AngleHybrid::allocate()
{
  allocated = 1;
  int np1 = atom->nangletypes + 1;

  memory->create(map, np1, "angle:map");
  memory->create(setflag, np1, "angle:setflag");
  for (int i = 1; i < np1; i++) setflag[i] = 0;

  nanglelist = new int[nstyles];
  maxangle = new int[nstyles];
  anglelist = new int **[nstyles];
  for (int m = 0; m < nstyles; m++) maxangle[m] = 0;
  for (int m = 0; m < nstyles; m++) anglelist[m] = nullptr;
}

/* ---------------------------------------------------------------------- */

void AngleHybrid::deallocate()
{
  if (!allocated) return;

  allocated = 0;

  memory->destroy(setflag);
  memory->destroy(map);
  delete[] nanglelist;
  delete[] maxangle;
  for (int i = 0; i < nstyles; i++) memory->destroy(anglelist[i]);
  delete[] anglelist;
}

/* ----------------------------------------------------------------------
   create one angle style for each arg in list
------------------------------------------------------------------------- */

void AngleHybrid::settings(int narg, char **arg)
{
  int i, m;

  if (narg < 1) utils::missing_cmd_args(FLERR, "angle_style hybrid", error);

  // delete old lists, since cannot just change settings

  if (nstyles) {
    for (i = 0; i < nstyles; i++) delete styles[i];
    delete[] styles;
    for (i = 0; i < nstyles; i++) delete[] keywords[i];
    delete[] keywords;
  }

  deallocate();

  // allocate list of sub-styles

  styles = new Angle *[narg];
  keywords = new char *[narg];

  // allocate each sub-style and call its settings() with subset of args
  // allocate uses suffix, but don't store suffix version in keywords,
  //   else syntax in coeff() will not match

  int dummy;
  nstyles = 0;
  i = 0;
  while (i < narg) {
    if (strcmp(arg[i], "hybrid") == 0)
      error->all(FLERR, "Angle style hybrid cannot have hybrid as an argument");

    if (strcmp(arg[i], "none") == 0)
      error->all(FLERR, "Angle style hybrid cannot have none as an argument");

    for (m = 0; m < nstyles; m++)
      if (strcmp(arg[i], keywords[m]) == 0)
        error->all(FLERR, "Angle style hybrid cannot use same angle style twice");

    styles[nstyles] = force->new_angle(arg[i], 1, dummy);
    keywords[nstyles] = force->store_style(arg[i], 0);

    // determine list of arguments for angle style settings
    // by looking for the next known angle style name.

    int jarg = i + 1;
    while ((jarg < narg) && !force->angle_map->count(arg[jarg]) &&
           !lmp->match_style("angle", arg[jarg]))
      jarg++;

    styles[nstyles]->settings(jarg - i - 1, &arg[i + 1]);
    i = jarg;
    nstyles++;
  }
}

/* ----------------------------------------------------------------------
   set coeffs for one type
---------------------------------------------------------------------- */

void AngleHybrid::coeff(int narg, char **arg)
{
  if (!allocated) allocate();

  int ilo, ihi;
  utils::bounds(FLERR, arg[0], 1, atom->nangletypes, ilo, ihi, error);

  // 2nd arg = angle sub-style name
  // allow for "none" or "skip" as valid sub-style name

  int m;
  for (m = 0; m < nstyles; m++)
    if (strcmp(arg[1], keywords[m]) == 0) break;

  int none = 0;
  int skip = 0;
  if (m == nstyles) {
    if (strcmp(arg[1], "none") == 0)
      none = 1;
    else if (strcmp(arg[1], "skip") == 0)
      none = skip = 1;
    else if (strcmp(arg[1], "ba") == 0)
      error->all(FLERR, "BondAngle coeff for hybrid angle has invalid format");
    else if (strcmp(arg[1], "bb") == 0)
      error->all(FLERR, "BondBond coeff for hybrid angle has invalid format");
    else
      error->all(FLERR, "Expected hybrid sub-style instead of {} in angle_coeff command", arg[1]);
  }

  // move 1st arg to 2nd arg
  // just copy ptrs, since arg[] points into original input line

  arg[1] = arg[0];

  // invoke sub-style coeff() starting with 1st arg

  if (!none) styles[m]->coeff(narg - 1, &arg[1]);

  // set setflag and which type maps to which sub-style
  // if sub-style is skip: auxiliary class2 setting in data file so ignore
  // if sub-style is none: set hybrid setflag, wipe out map

  for (int i = ilo; i <= ihi; i++) {
    if (skip)
      continue;
    else if (none) {
      setflag[i] = 1;
      map[i] = -1;
    } else {
      setflag[i] = styles[m]->setflag[i];
      map[i] = m;
    }
  }
}

/* ----------------------------------------------------------------------
   run angle style specific initialization
------------------------------------------------------------------------- */

void AngleHybrid::init_style()
{
  // error if sub-style is not used

  int used;
  for (int istyle = 0; istyle < nstyles; ++istyle) {
    used = 0;
    for (int itype = 1; itype <= atom->nangletypes; ++itype)
      if (map[itype] == istyle) used = 1;
    if (used == 0) error->all(FLERR, "Angle hybrid sub-style {} is not used", keywords[istyle]);
  }

  for (int m = 0; m < nstyles; m++)
    if (styles[m]) styles[m]->init_style();
}

/* ---------------------------------------------------------------------- */

int AngleHybrid::check_itype(int itype, char *substyle)
{
  if (strcmp(keywords[map[itype]], substyle) == 0) return 1;
  return 0;
}

/* ----------------------------------------------------------------------
   return an equilbrium angle length
------------------------------------------------------------------------- */

double AngleHybrid::equilibrium_angle(int i)
{
  if (map[i] < 0) error->one(FLERR, "Invoked angle equil angle on angle style none");
  return styles[map[i]]->equilibrium_angle(i);
}

/* ----------------------------------------------------------------------
   proc 0 writes to restart file
------------------------------------------------------------------------- */

void AngleHybrid::write_restart(FILE *fp)
{
  fwrite(&nstyles, sizeof(int), 1, fp);

  int n;
  for (int m = 0; m < nstyles; m++) {
    n = strlen(keywords[m]) + 1;
    fwrite(&n, sizeof(int), 1, fp);
    fwrite(keywords[m], sizeof(char), n, fp);
    styles[m]->write_restart_settings(fp);
  }
}

/* ----------------------------------------------------------------------
   proc 0 reads from restart file, bcasts
------------------------------------------------------------------------- */

void AngleHybrid::read_restart(FILE *fp)
{
  int me = comm->me;
  if (me == 0) utils::sfread(FLERR, &nstyles, sizeof(int), 1, fp, nullptr, error);
  MPI_Bcast(&nstyles, 1, MPI_INT, 0, world);
  styles = new Angle *[nstyles];
  keywords = new char *[nstyles];

  allocate();

  int n, dummy;
  for (int m = 0; m < nstyles; m++) {
    if (me == 0) utils::sfread(FLERR, &n, sizeof(int), 1, fp, nullptr, error);
    MPI_Bcast(&n, 1, MPI_INT, 0, world);
    keywords[m] = new char[n];
    if (me == 0) utils::sfread(FLERR, keywords[m], sizeof(char), n, fp, nullptr, error);
    MPI_Bcast(keywords[m], n, MPI_CHAR, 0, world);
    styles[m] = force->new_angle(keywords[m], 1, dummy);
    styles[m]->read_restart_settings(fp);
  }
}

/* ---------------------------------------------------------------------- */

double AngleHybrid::single(int type, int i1, int i2, int i3)
{
  if (map[type] < 0) error->one(FLERR, "Invoked angle single on angle style none");
  return styles[map[type]]->single(type, i1, i2, i3);
}

/* ----------------------------------------------------------------------
   memory usage
------------------------------------------------------------------------- */

double AngleHybrid::memory_usage()
{
  double bytes = (double) maxeatom * sizeof(double);
  bytes += (double) maxvatom * 6 * sizeof(double);
  bytes += (double) maxcvatom * 9 * sizeof(double);
  for (int m = 0; m < nstyles; m++) bytes += (double) maxangle[m] * 4 * sizeof(int);
  for (int m = 0; m < nstyles; m++)
    if (styles[m]) bytes += styles[m]->memory_usage();
  return bytes;
}
