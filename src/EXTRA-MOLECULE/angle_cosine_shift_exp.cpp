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
   Contributing author: Carsten Svaneborg, science@zqex.dk
------------------------------------------------------------------------- */

#include "angle_cosine_shift_exp.h"

#include <cmath>
#include "atom.h"
#include "neighbor.h"
#include "domain.h"
#include "comm.h"
#include "force.h"
#include "math_const.h"
#include "memory.h"
#include "error.h"


using namespace LAMMPS_NS;
using namespace MathConst;

static constexpr double SMALL = 0.001;

/* ---------------------------------------------------------------------- */

AngleCosineShiftExp::AngleCosineShiftExp(LAMMPS *lmp) : Angle(lmp)
{
  doExpansion = nullptr;
  umin = nullptr;
  a = nullptr;
  opt1 = nullptr;
  theta0 = nullptr;
  sint = nullptr;
  cost = nullptr;
}

/* ---------------------------------------------------------------------- */

AngleCosineShiftExp::~AngleCosineShiftExp()
{
  if (allocated) {
    memory->destroy(setflag);
    memory->destroy(umin);
    memory->destroy(a);
    memory->destroy(opt1);
    memory->destroy(cost);
    memory->destroy(sint);
    memory->destroy(theta0);
    memory->destroy(doExpansion);
  }
}

/* ---------------------------------------------------------------------- */

void AngleCosineShiftExp::compute(int eflag, int vflag)
{
  int i1,i2,i3,n,type;
  double delx1,dely1,delz1,delx2,dely2,delz2;
  double eangle,f1[3],f3[3],ff;
  double rsq1,rsq2,r1,r2,c,s,a11,a12,a22;
  double exp2,aa,uumin,cccpsss,cssmscc;

  eangle = 0.0;
  ev_init(eflag,vflag);

  double **x = atom->x;
  double **f = atom->f;
  int **anglelist = neighbor->anglelist;
  int nanglelist = neighbor->nanglelist;
  int nlocal = atom->nlocal;
  int newton_bond = force->newton_bond;

  for (n = 0; n < nanglelist; n++) {
    i1 = anglelist[n][0];
    i2 = anglelist[n][1];
    i3 = anglelist[n][2];
    type = anglelist[n][3];

    // 1st bond

    delx1 = x[i1][0] - x[i2][0];
    dely1 = x[i1][1] - x[i2][1];
    delz1 = x[i1][2] - x[i2][2];

    rsq1 = delx1*delx1 + dely1*dely1 + delz1*delz1;
    r1 = sqrt(rsq1);

    // 2nd bond

    delx2 = x[i3][0] - x[i2][0];
    dely2 = x[i3][1] - x[i2][1];
    delz2 = x[i3][2] - x[i2][2];

    rsq2 = delx2*delx2 + dely2*dely2 + delz2*delz2;
    r2 = sqrt(rsq2);

    // c = cosine of angle
    c = delx1*delx2 + dely1*dely2 + delz1*delz2;
    c /= r1*r2;
    if (c > 1.0) c = 1.0;
    if (c < -1.0) c = -1.0;

    // C= sine of angle
    s = sqrt(1.0 - c*c);
    if (s < SMALL) s = SMALL;

    // force & energy

    aa=a[type];
    uumin=umin[type];

    cccpsss = c*cost[type]+s*sint[type];
    cssmscc = c*sint[type]-s*cost[type];

    if (doExpansion[type])
       {  //  |a|<0.01 so use expansions relative precision <1e-5
//         std::cout << "Using expansion\n";
            if (eflag) eangle = -0.125*(1+cccpsss)*(4+aa*(cccpsss-1))*uumin;
            ff=0.25*uumin*cssmscc*(2+aa*cccpsss)/s;
       }
     else
       {
//   std::cout << "Not using expansion\n";
            exp2=exp(0.5*aa*(1+cccpsss));
            if (eflag) eangle = opt1[type]*(1-exp2);
            ff=0.5*a[type]*opt1[type]*exp2*cssmscc/s;
       }

    a11 =   ff*c/ rsq1;
    a12 =  -ff  / (r1*r2);
    a22 =   ff*c/ rsq2;

    f1[0] = a11*delx1 + a12*delx2;
    f1[1] = a11*dely1 + a12*dely2;
    f1[2] = a11*delz1 + a12*delz2;
    f3[0] = a22*delx2 + a12*delx1;
    f3[1] = a22*dely2 + a12*dely1;
    f3[2] = a22*delz2 + a12*delz1;

    // apply force to each of 3 atoms

    if (newton_bond || i1 < nlocal) {
      f[i1][0] += f1[0];
      f[i1][1] += f1[1];
      f[i1][2] += f1[2];
    }

    if (newton_bond || i2 < nlocal) {
      f[i2][0] -= f1[0] + f3[0];
      f[i2][1] -= f1[1] + f3[1];
      f[i2][2] -= f1[2] + f3[2];
    }

    if (newton_bond || i3 < nlocal) {
      f[i3][0] += f3[0];
      f[i3][1] += f3[1];
      f[i3][2] += f3[2];
    }

    if (evflag) ev_tally(i1,i2,i3,nlocal,newton_bond,eangle,f1,f3,
                         delx1,dely1,delz1,delx2,dely2,delz2);
  }
}

/* ---------------------------------------------------------------------- */

void AngleCosineShiftExp::allocate()
{
  allocated = 1;
  int n = atom->nangletypes;

  memory->create(doExpansion,   n+1, "angle:doExpansion");
  memory->create(umin       ,   n+1, "angle:umin");
  memory->create(a          ,   n+1, "angle:a");
  memory->create(sint       ,   n+1, "angle:sint");
  memory->create(cost       ,   n+1, "angle:cost");
  memory->create(opt1       ,   n+1, "angle:opt1");
  memory->create(theta0     ,   n+1, "angle:theta0");
  memory->create(setflag    ,   n+1, "angle:setflag");

  for (int i = 1; i <= n; i++) setflag[i] = 0;
}

/* ----------------------------------------------------------------------
   set coeffs for one type
------------------------------------------------------------------------- */

void AngleCosineShiftExp::coeff(int narg, char **arg)
{
  if (narg != 4) error->all(FLERR,"Incorrect args for angle coefficients" + utils::errorurl(21));
  if (!allocated) allocate();

  int ilo,ihi;
  utils::bounds(FLERR,arg[0],1,atom->nangletypes,ilo,ihi,error);

  double umin_   = utils::numeric(FLERR,arg[1],false,lmp);
  double theta0_ = utils::numeric(FLERR,arg[2],false,lmp);
  double a_      = utils::numeric(FLERR,arg[3],false,lmp);

  int count = 0;
  for (int i = ilo; i <= ihi; i++) {
    doExpansion[i]=(fabs(a_)<0.001);
    umin[i]  = umin_;
    a[i]     = a_;
    cost[i]  = cos(theta0_*MY_PI / 180.0);
    sint[i]  = sin(theta0_*MY_PI / 180.0);
    theta0[i]=     theta0_*MY_PI / 180.0;

    if (!doExpansion[i]) opt1[i]=umin_/(exp(a_)-1);

    setflag[i] = 1;
    count++;
  }

  if (count == 0) error->all(FLERR,"Incorrect args for angle coefficients" + utils::errorurl(21));
}

/* ---------------------------------------------------------------------- */

double AngleCosineShiftExp::equilibrium_angle(int i)
{
  return theta0[i];
}

/* ----------------------------------------------------------------------
   proc 0 writes out coeffs to restart file
------------------------------------------------------------------------- */

void AngleCosineShiftExp::write_restart(FILE *fp)
{
  fwrite(&umin[1],sizeof(double),atom->nangletypes,fp);
  fwrite(&a[1],sizeof(double),atom->nangletypes,fp);
  fwrite(&cost[1],sizeof(double),atom->nangletypes,fp);
  fwrite(&sint[1],sizeof(double),atom->nangletypes,fp);
  fwrite(&theta0[1],sizeof(double),atom->nangletypes,fp);
}

/* ----------------------------------------------------------------------
   proc 0 reads coeffs from restart file, bcasts them
------------------------------------------------------------------------- */

void AngleCosineShiftExp::read_restart(FILE *fp)
{
  allocate();

  if (comm->me == 0)
      {
        utils::sfread(FLERR,&umin[1],sizeof(double),atom->nangletypes,fp,nullptr,error);
        utils::sfread(FLERR,&a[1],sizeof(double),atom->nangletypes,fp,nullptr,error);
        utils::sfread(FLERR,&cost[1],sizeof(double),atom->nangletypes,fp,nullptr,error);
        utils::sfread(FLERR,&sint[1],sizeof(double),atom->nangletypes,fp,nullptr,error);
        utils::sfread(FLERR,&theta0[1],sizeof(double),atom->nangletypes,fp,nullptr,error);
      }
  MPI_Bcast(&umin[1],atom->nangletypes,MPI_DOUBLE,0,world);
  MPI_Bcast(&a[1],atom->nangletypes,MPI_DOUBLE,0,world);
  MPI_Bcast(&cost[1],atom->nangletypes,MPI_DOUBLE,0,world);
  MPI_Bcast(&sint[1],atom->nangletypes,MPI_DOUBLE,0,world);
  MPI_Bcast(&theta0[1],atom->nangletypes,MPI_DOUBLE,0,world);

  for (int i = 1; i <= atom->nangletypes; i++)
             {
                setflag[i] = 1;
                doExpansion[i]=(fabs(a[i])<0.01);
                if (!doExpansion[i]) opt1[i]=umin[i]/(exp(a[i])-1);
             }
}

/* ----------------------------------------------------------------------
   proc 0 writes to data file
------------------------------------------------------------------------- */

void AngleCosineShiftExp::write_data(FILE *fp)
{
  for (int i = 1; i <= atom->nangletypes; i++)
    fprintf(fp,"%d %g %g %g\n",i,umin[i],theta0[i]/MY_PI*180.0,a[i]);
}

/* ---------------------------------------------------------------------- */

double AngleCosineShiftExp::single(int type, int i1, int i2, int i3)
{
  double **x = atom->x;

  double delx1 = x[i1][0] - x[i2][0];
  double dely1 = x[i1][1] - x[i2][1];
  double delz1 = x[i1][2] - x[i2][2];
  domain->minimum_image(FLERR, delx1,dely1,delz1);
  double r1 = sqrt(delx1*delx1 + dely1*dely1 + delz1*delz1);

  double delx2 = x[i3][0] - x[i2][0];
  double dely2 = x[i3][1] - x[i2][1];
  double delz2 = x[i3][2] - x[i2][2];
  domain->minimum_image(FLERR, delx2,dely2,delz2);
  double r2 = sqrt(delx2*delx2 + dely2*dely2 + delz2*delz2);

  double c = delx1*delx2 + dely1*dely2 + delz1*delz2;
  c /= r1*r2;
  if (c > 1.0) c = 1.0;
  if (c < -1.0) c = -1.0;
  double s=sqrt(1.0-c*c);

  double cccpsss=c*cost[type]+s*sint[type];

  if (doExpansion[type])
       {
         return -0.125*(1+cccpsss)*(4+a[type]*(cccpsss-1))*umin[type];
       }
     else
       {
         return opt1[type]*(1-exp(0.5*a[type]*(1+cccpsss)));
      }
}
