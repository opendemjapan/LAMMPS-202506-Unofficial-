Notes for updating code written for older LAMMPS versions
---------------------------------------------------------

This section documents how C++ source files that are available *outside
of the LAMMPS source distribution* (e.g. in external USER packages or as
source files provided as a supplement to a publication) that are written
for an older version of LAMMPS and thus need to be updated to be
compatible with the current version of LAMMPS.  Due to the active
development of LAMMPS it is likely to always be incomplete.  Please
contact developers@lammps.org in case you run across an issue that is not
(yet) listed here.  Please also review the latest information about the
LAMMPS :doc:`programming style conventions <Modify_style>`, especially
if you are considering to submit the updated version for inclusion into
the LAMMPS distribution.

Available topics in mostly chronological order are:

- `Setting flags in the constructor`_
- `Rename of pack/unpack_comm() to pack/unpack_forward_comm()`_
- `Use ev_init() to initialize variables derived from eflag and vflag`_
- `Use utils::count_words() functions instead of atom->count_words()`_
- `Use utils::numeric() functions instead of force->numeric()`_
- `Use utils::open_potential() function to open potential files`_
- `Use symbolic Atom and AtomVec constants instead of numerical values`_
- `Simplify customized error messages`_
- `Use of "override" instead of "virtual"`_
- `Simplified and more compact neighbor list requests`_
- `Split of fix STORE into fix STORE/GLOBAL and fix STORE/PERATOM`_
- `Rename of fix STORE/PERATOM to fix STORE/ATOM and change of arguments`_
- `Use Output::get_dump_by_id() instead of Output::find_dump()`_
- `Refactored grid communication using Grid3d/Grid2d classes instead of GridComm`_
- `FLERR as first argument to minimum image functions in Domain class`_
- `Use utils::logmesg() instead of error->warning()`_

----

Setting flags in the constructor
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As LAMMPS gains additional functionality, new flags may need to be set
in the constructor or a class to signal compatibility with such features.
Most of the time the defaults are chosen conservatively, but sometimes
the conservative choice is the uncommon choice, and then those settings
need to be made when updating code.

Pair styles:

  - ``manybody_flag``: set to 1 if your pair style is not pair-wise additive
  - ``restartinfo``: set to 0 if your pair style does not store data in restart files


Rename of pack/unpack_comm() to pack/unpack_forward_comm()
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 8Aug2014

In this change set, the functions to pack/unpack data into communication buffers
for :doc:`forward communications <Developer_comm_ops>` were renamed from
``pack_comm()`` and ``unpack_comm()`` to ``pack_forward_comm()`` and
``unpack_forward_comm()``, respectively.  Also the meaning of the return
value of these functions was changed: rather than returning the number
of items per atom stored in the buffer, now the total number of items
added (or unpacked) needs to be returned.  Here is an example from the
`PairEAM` class.  Of course the member function declaration in corresponding
header file needs to be updated accordingly.

Old:

.. code-block:: c++

   int PairEAM::pack_comm(int n, int *list, double *buf, int pbc_flag, int *pbc)
   {
     int m = 0;
     for (int i = 0; i < n; i++) {
       int j = list[i];
       buf[m++] = fp[j];
     }
     return 1;
   }

New:

.. code-block:: c++

   int PairEAM::pack_forward_comm(int n, int *list, double *buf, int pbc_flag, int *pbc)
   {
     int m = 0;
     for (int i = 0; i < n; i++) {
       int j = list[i];
       buf[m++] = fp[j];
     }
     return m;
   }

.. note::

   Because the various "pack" and "unpack" functions are defined in the
   respective base classes as dummy functions doing nothing, and because
   of the the name mismatch the custom versions in the derived class
   will no longer be called, there will be no compilation error when
   this change is not applied.  Only calculations will suddenly produce
   incorrect results because the required forward communication calls
   will cease to function correctly.

Use ev_init() to initialize variables derived from eflag and vflag
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 29Mar2019

There are several variables that need to be initialized based on
the values of the "eflag" and "vflag" variables and since sometimes
there are new bits added and new variables need to be set to 1 or 0.
To make this consistent across all styles, there is now an inline
function ``ev_init(eflag, vflag)`` that makes those settings
consistently and calls either ``ev_setup()`` or ``ev_unset()``.
Example from a pair style:

Old:

.. code-block:: c++

   if (eflag || vflag) ev_setup(eflag, vflag);
   else evflag = vflag_fdotr = eflag_global = eflag_atom = 0;

New:

.. code-block:: c++

   ev_init(eflag, vflag);

Not applying this change will not cause a compilation error, but
can lead to inconsistent behavior and incorrect tallying of
energy or virial.

Use utils::count_words() functions instead of atom->count_words()
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 2Jun2020

The "count_words()" functions for parsing text have been moved from the
Atom class to the :doc:`utils namespace <Developer_utils>`.  The
"count_words()" function in "utils" uses the Tokenizer class internally
to split a line into words and count them, thus it will not modify the
argument string as the function in the Atoms class did and thus had a
variant using a copy buffer.  Unlike the old version, the new version
does not remove comments. For that you can use the
:cpp:func:`utils::trim_comment() function
<LAMMPS_NS::utils::trim_comment>` as shown in the example below.

Old:

.. code-block:: c++

   nwords = atom->count_words(line);
   int nwords = atom->count_words(buf);

New:

.. code-block:: c++

   nwords = utils::count_words(line);
   int nwords = utils::count_words(utils::trim_comment(buf));

.. seealso::

   :cpp:func:`utils::count_words() <LAMMPS_NS::utils::count_words>`,
   :cpp:func:`utils::trim_comments() <LAMMPS_NS::utils::trim_comments>`


Use utils::numeric() functions instead of force->numeric()
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 18Sep2020

The "numeric()" conversion functions (including "inumeric()",
"bnumeric()", and "tnumeric()") have been moved from the Force class to
the :doc:`utils namespace <Developer_utils>`.  Also they take an
additional argument that selects whether the ``Error::all()`` or
``Error::one()`` function should be called in case of an error.  The
former should be used when *all* MPI processes call the conversion
function and the latter *must* be used when they are called from only
one or a subset of the MPI processes.

Old:

.. code-block:: c++

    val = force->numeric(FLERR, arg[1]);
    num = force->inumeric(FLERR, arg[2]);

New:

.. code-block:: c++

    val = utils::numeric(FLERR, true, arg[1], lmp);
    num = utils::inumeric(FLERR, false, arg[2], lmp);

.. seealso::

   :cpp:func:`utils::numeric() <LAMMPS_NS::utils::numeric>`,
   :cpp:func:`utils::inumeric() <LAMMPS_NS::utils::inumeric>`,
   :cpp:func:`utils::bnumeric() <LAMMPS_NS::utils::bnumeric>`,
   :cpp:func:`utils::tnumeric() <LAMMPS_NS::utils::tnumeric>`

Use utils::open_potential() function to open potential files
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 18Sep2020

The :cpp:func:`utils::open_potential()
<LAMMPS_NS::utils::open_potential>` function must be used to replace
calls to ``force->open_potential()`` and should be used to replace
``fopen()`` for opening potential files for reading.  The custom
function does three additional steps compared to ``fopen()``: 1) it will
try to parse the ``UNITS:`` and ``DATE:`` metadata and will stop with an
error on a units mismatch and will print the date info, if present, in
the log file; 2) for pair styles that support it, it will set up
possible automatic unit conversions based on the embedded unit
information and LAMMPS' current units setting; 3) it will not only try
to open a potential file at the given path, but will also search in the
folders listed in the ``LAMMPS_POTENTIALS`` environment variable.  This
allows potential files to reside in a common location instead of having to
copy them around for simulations.

Old:

.. code-block:: c++

   fp = force->open_potential(filename);
   fp = fopen(filename, "r");

New:

.. code-block:: c++

   fp = utils::open_potential(filename, lmp);

Use symbolic Atom and AtomVec constants instead of numerical values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 18Sep2020

Properties in LAMMPS that were represented by integer values (0, 1,
2, 3) to indicate settings in the ``Atom`` and ``AtomVec`` classes (or
classes derived from it) (and its derived classes) have been converted
to use scoped enumerators instead.

.. list-table::
   :header-rows: 1
   :widths: 23 10 23 10 23 10

   * - Symbolic Constant
     - Value
     - Symbolic Constant
     - Value
     - Symbolic Constant
     - Value
   * - Atom::GROW
     - 0
     - Atom::ATOMIC
     - 0
     - Atom::MAP_NONE
     - 0
   * - Atom::RESTART
     - 1
     - Atom::MOLECULAR
     - 1
     - Atom::MAP_ARRAY
     - 1
   * - Atom::BORDER
     - 2
     - Atom::TEMPLATE
     - 2
     - Atom::MAP_HASH
     - 2
   * - AtomVec::PER_ATOM
     - 0
     - AtomVec::PER_TYPE
     - 1
     - Atom::MAP_YES
     - 3

Old:

.. code-block:: c++

   molecular = 0;
   mass_type = 1;
   if (atom->molecular == 2)
   if (atom->map_style == 2)
   atom->add_callback(0);
   atom->delete_callback(id,1);

New:

.. code-block:: c++

   molecular = Atom::ATOMIC;
   mass_type = AtomVec::PER_TYPE;
   if (atom->molecular == Atom::TEMPLATE)
   if (atom->map_style == Atom::MAP_HASH)
   atom->add_callback(Atom::GROW);
   atom->delete_callback(id,Atom::RESTART);

Simplify customized error messages
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 14May2021

Aided by features of the bundled {fmt} library, error messages now
can have a variable number of arguments and the string will be interpreted
as a {fmt} style format string so that error messages can be
easily customized without having to use temporary buffers and ``sprintf()``.
Example:

Old:

.. code-block:: c++

   if (fptr == NULL) {
     char str[128];
     sprintf(str,"Cannot open AEAM potential file %s",filename);
     error->one(FLERR,str);
   }

New:

.. code-block:: c++

   if (fptr == nullptr)
     error->one(FLERR, "Cannot open AEAM potential file {}: {}", filename, utils::getsyserror());

Use of "override" instead of "virtual"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 17Feb2022

Since LAMMPS requires C++11, we switched to use the "override" keyword
instead of "virtual" to indicate polymorphism in derived classes.  This
allows the C++ compiler to better detect inconsistencies when an
override is intended or not.  Please note that "override" has to be
added to **all** polymorph functions in derived classes and "virtual"
*only* to the function in the base class (or the destructor).  Here is
an example from the ``FixWallReflect`` class:

Old:

.. code-block:: c++

   FixWallReflect(class LAMMPS *, int, char **);
   virtual ~FixWallReflect();
   int setmask();
   void init();
   void post_integrate();

New:

.. code-block:: c++

   FixWallReflect(class LAMMPS *, int, char **);
   ~FixWallReflect() override;
   int setmask() override;
   void init() override;
   void post_integrate() override;

This change set will neither cause a compilation failure, nor will it
change functionality, but if you plan to submit the updated code for
inclusion into the LAMMPS distribution, it will be requested for achieve
a consistent :doc:`programming style <Modify_style>`.

Simplified function names for forward and reverse communication
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 24Mar2022

Rather than using the function name to distinguish between the different
forward and reverse communication functions for styles, LAMMPS now uses
the type of the "this" pointer argument.

Old:

.. code-block:: c++

   comm->forward_comm_pair(this);
   comm->forward_comm_fix(this);
   comm->forward_comm_compute(this);
   comm->forward_comm_dump(this);
   comm->reverse_comm_pair(this);
   comm->reverse_comm_fix(this);
   comm->reverse_comm_compute(this);
   comm->reverse_comm_dump(this);

New:

.. code-block:: c++

   comm->forward_comm(this);
   comm->reverse_comm(this);

This change is **required** or else the code will not compile.

Simplified and more compact neighbor list requests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 24Mar2022

This change set reduces the amount of code required to request a
neighbor list.  It enforces consistency and no longer requires to change
internal data of the request.  More information on neighbor list
requests can be :doc:`found here <Developer_notes>`. Example from the
``ComputeRDF`` class:

Old:

.. code-block:: c++

   int irequest = neighbor->request(this,instance_me);
   neighbor->requests[irequest]->pair = 0;
   neighbor->requests[irequest]->compute = 1;
   neighbor->requests[irequest]->occasional = 1;
   if (cutflag) {
     neighbor->requests[irequest]->cut = 1;
     neighbor->requests[irequest]->cutoff = mycutneigh;
   }

New:

.. code-block:: c++

   auto req = neighbor->add_request(this, NeighConst::REQ_OCCASIONAL);
   if (cutflag) req->set_cutoff(mycutneigh);

Public access to the ``NeighRequest`` class data members has been
removed so this update is **required** to avoid compilation failure.

Split of fix STORE into fix STORE/GLOBAL and fix STORE/PERATOM
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 15Sep2022

This change splits the GLOBAL and PERATOM modes of fix STORE into two
separate fixes STORE/GLOBAL and STORE/PERATOM.  There was very little
shared code between the two fix STORE modes and the two different code
paths had to be prefixed with if statements.  Furthermore, some flags
were used differently in the two modes leading to confusion.  Splitting
the code into two fix styles, makes it more easily maintainable.  Since
these are internal fixes, there is no user visible change.

Old:

.. code-block:: c++

   #include "fix_store.h"

   FixStore *fix = dynamic_cast<FixStore *>(
      modify->add_fix(fmt::format("{} {} STORE peratom 1 13",id_pole,group->names[0]));

   FixStore *fix = dynamic_cast<FixStore *>(modify->get_fix_by_id(id_pole));

New:

.. code-block:: c++

   #include "fix_store_peratom.h"

   FixStorePeratom *fix = dynamic_cast<FixStorePeratom *>(
      modify->add_fix(fmt::format("{} {} STORE/PERATOM 1 13",id_pole,group->names[0]));

   FixStorePeratom *fix = dynamic_cast<FixStorePeratom *>(modify->get_fix_by_id(id_pole));

Old:

.. code-block:: c++

   #include "fix_store.h"

   FixStore *fix = dynamic_cast<FixStore *>(
      modify->add_fix(fmt::format("{} {} STORE global 1 1",id_fix,group->names[igroup]));

   FixStore *fix = dynamic_cast<FixStore *>(modify->get_fix_by_id(id_fix));

New:

.. code-block:: c++

   #include "fix_store_global.h"

   FixStoreGlobal *fix = dynamic_cast<FixStoreGlobal *>(
      modify->add_fix(fmt::format("{} {} STORE/GLOBAL 1 1",id_fix,group->names[igroup]));

   FixStoreGlobal *fix = dynamic_cast<FixStoreGlobal *>(modify->get_fix_by_id(id_fix));

This change is **required** or else the code will not compile.

Rename of fix STORE/PERATOM to fix STORE/ATOM and change of arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 28Mar2023

The available functionality of the internal fix to store per-atom
properties was expanded to enable storing data with ghost atoms and to
support binary restart files.  With those changes, the fix was renamed
to fix STORE/ATOM and the number and order of (required) arguments has
changed.

Old syntax: ``ID group-ID STORE/PERATOM rflag n1 n2 [n3]``

- *rflag* = 0/1, *no*/*yes* store per-atom values in restart file
- :math:`n1 = 1, n2 = 1, \mathrm{no}\;n3 \to` per-atom vector, single value per atom
- :math:`n1 = 1, n2 > 1, \mathrm{no}\;n3 \to` per-atom array, *n2* values per atom
- :math:`n1 = 1, n2 > 0, n3 > 0 \to` per-atom tensor, *n2* x *n3* values per atom

New syntax:  ``ID group-ID STORE/ATOM n1 n2 gflag rflag``

- :math:`n1 = 1, n2 = 0 \to` per-atom vector, single value per atom
- :math:`n1 > 1, n2 = 0 \to` per-atom array, *n1* values per atom
- :math:`n1 > 0, n2 > 0 \to` per-atom tensor, *n1* x *n2* values per atom
- *gflag* = 0/1, *no*/*yes* communicate per-atom values with ghost atoms
- *rflag* = 0/1, *no*/*yes* store per-atom values in restart file

Since this is an internal fix, there is no user visible change.

Use Output::get_dump_by_id() instead of Output::find_dump()
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 15Sep2022

The accessor function to individual dump style instances has been changed
from ``Output::find_dump()`` returning the index of the dump instance in
the list of dumps to ``Output::get_dump_by_id()`` returning a pointer to
the dump directly.  Example:

Old:

.. code-block:: c++

   int idump = output->find_dump(arg[iarg+1]);
   if (idump < 0)
     error->all(FLERR,"Dump ID in hyper command does not exist");
   memory->grow(dumplist,ndump+1,"hyper:dumplist");
   dumplist[ndump++] = idump;

   [...]

   if (dumpflag)
     for (int idump = 0; idump < ndump; idump++)
       output->dump[dumplist[idump]]->write();

New:

.. code-block:: c++

   auto idump = output->get_dump_by_id(arg[iarg+1]);
   if (!idump) error->all(FLERR,"Dump ID {} in hyper command does not exist", arg[iarg+1]);
   dumplist.emplace_back(idump);

   [...]

   if (dumpflag) for (auto idump : dumplist) idump->write();

This change is **required** or else the code will not compile.

Refactored grid communication using Grid3d/Grid2d classes instead of GridComm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 22Dec2022

The ``GridComm`` class was for creating and communicating distributed
grids was replaced by the ``Grid3d`` class with added functionality.
A ``Grid2d`` class was also added for additional flexibility.

The new functionality and commands using the two grid classes are
discussed on the following documentation pages:

- :doc:`Howto_grid`
- :doc:`Developer_grid`

If you have custom LAMMPS code, which uses the GridComm class, here are some notes
on how to adapt it for using the Grid3d class.

(1) The constructor has changed to allow the ``Grid3d`` / ``Grid2d``
    classes to partition the global grid across processors, both for
    owned and ghost grid cells.  Previously any class which called
    ``GridComm`` performed the partitioning itself and that information
    was passed in the ``GridComm::GridComm()`` constructor.  There are
    several "set" functions which can be called to alter how ``Grid3d``
    / ``Grid2d`` perform the partitioning.  They should be sufficient
    for most use cases of the grid classes.

(2) The partitioning is triggered by the ``setup_grid()`` method.

(3) The ``setup()`` method of the ``GridComm`` class has been replaced
    by the ``setup_comm()`` method in the new grid classes.  The syntax
    for the ``forward_comm()`` and ``reverse_comm()`` methods is
    slightly altered as is the syntax of the associated pack/unpack
    callback methods.  But the functionality of these operations is the
    same as before.

(4) The new ``Grid3d`` / ``Grid2d`` classes have additional
    functionality for dynamic load-balancing of grids and their
    associated data across processors.  This did not exist in the
    ``GridComm`` class.

This and more is explained in detail on the :doc:`Developer_grid` page.
The following LAMMPS source files can be used as illustrative examples
for how the new grid classes are used by computes, fixes, and various
KSpace solvers which use distributed FFT grids:

- ``src/fix_ave_grid.cpp``
- ``src/compute_property_grid.cpp``
- ``src/EXTRA-FIX/fix_ttm_grid.cpp``
- ``src/KSPACE/pppm.cpp``

This change is **required** or else the code will not compile.

FLERR as first argument to minimum image functions in Domain class
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: 12Jun2025

The ``Domain::minimum_image()`` and ``Domain::minimum_image_big()``
functions were changed to take the ``FLERR`` macros as first argument.
This way the error message indicates *where* the function was called
instead of pointing to the implementation of the function.  Example:

Old:

.. code-block:: c++

   double delx1 = x[i1][0] - x[i2][0];
   double dely1 = x[i1][1] - x[i2][1];
   double delz1 = x[i1][2] - x[i2][2];
   domain->minimum_image(delx1, dely1, delz1);
   double r1 = sqrt(delx1 * delx1 + dely1 * dely1 + delz1 * delz1);

   double delx2 = x[i3][0] - x[i2][0];
   double dely2 = x[i3][1] - x[i2][1];
   double delz2 = x[i3][2] - x[i2][2];
   domain->minimum_image_big(delx2, dely2, delz2);
   double r2 = sqrt(delx2 * delx2 + dely2 * dely2 + delz2 * delz2);

New:

.. code-block:: c++

   double delx1 = x[i1][0] - x[i2][0];
   double dely1 = x[i1][1] - x[i2][1];
   double delz1 = x[i1][2] - x[i2][2];
   domain->minimum_image(FLERR, delx1, dely1, delz1);
   double r1 = sqrt(delx1 * delx1 + dely1 * dely1 + delz1 * delz1);

   double delx2 = x[i3][0] - x[i2][0];
   double dely2 = x[i3][1] - x[i2][1];
   double delz2 = x[i3][2] - x[i2][2];
   domain->minimum_image_big(FLERR, delx2, dely2, delz2);
   double r2 = sqrt(delx2 * delx2 + dely2 * dely2 + delz2 * delz2);

This change is **required** or else the code will not compile.

Use utils::logmesg() instead of error->warning()
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionchanged:: TBD

The ``Error::message()`` method has been removed since its functionality
has been superseded by the :cpp:func:`utils::logmesg` function.

Old:

.. code-block:: c++

   if (comm->me == 0) {
     error->message(FLERR, "INFO: About to read data file: {}", filename);
  }

New:

.. code-block:: c++

   if (comm->me == 0) utils::logmesg(lmp, "INFO: About to read data file: {}\n", filename);

This change is **required** or else the code will not compile.
