.. index:: compute bond/local

compute bond/local command
==========================

Syntax
""""""

.. code-block:: LAMMPS

   compute ID group-ID bond/local value1 value2 ... keyword args ...

* ID, group-ID are documented in :doc:`compute <compute>` command
* bond/local = style name of this compute command
* one or more values may be appended
* value = *dist* or *dx* or *dy* or *dz* or *engpot* or *force* or *fx* or *fy* or *fz* or *engvib* or *engrot* or *engtrans* or *omega* or *velvib* or *v_name* or *bN*

.. parsed-literal::

     *dist* = bond distance
     *engpot* = bond potential energy
     *force* = bond force

     *dx*,\ *dy*,\ *dz* = components of pairwise distance
     *fx*,\ *fy*,\ *fz* = components of bond force
     *engvib* = bond kinetic energy of vibration
     *engrot* = bond kinetic energy of rotation
     *engtrans* = bond kinetic energy of translation
     *omega* = magnitude of bond angular velocity
     *velvib* = vibrational velocity along the bond length
     *v_name* = equal-style variable with name (see below)
     *bN* = bond style specific quantities for allowed N values

* zero or more keyword/args pairs may be appended
* keyword = *set*

.. parsed-literal::

     *set* args = *dist* name
       *dist* = only currently allowed arg
       name = name of variable to set with distance (dist)

Examples
""""""""

.. code-block:: LAMMPS

   compute 1 all bond/local engpot
   compute 1 all bond/local dist engpot force

   compute 1 all bond/local dist fx fy fz b1 b2

   compute 1 all bond/local dist v_distsq set dist d

Description
"""""""""""

Define a computation that calculates properties of individual bond
interactions.  The number of datums generated, aggregated across all
processors, equals the number of bonds in the system, modified by the
group parameter as explained below.

All these properties are computed for the pair of atoms in a bond,
whether the two atoms represent a simple diatomic molecule, or are part
of some larger molecule.

.. versionchanged:: 12Jun2025

   The sign of *dx*, *dy*, *dz* is no longer determined by the atom IDs
   of the bonded atoms but by their order in the bond list to be
   consistent with *fx*, *fy*, and *fz*.

The value *dist* is the current length of the bond.  The values *dx*,
*dy*, and *dz* are the :math:`(x,y,z)` components of the distance vector
:math:`\vec{x_i} - \vec{x_j}` between the atoms in the bond.  The order
of the atoms is determined by the bond list and the respective atom-IDs
can be output with :doc:`compute property/local
<compute_property_local>`.

The value *engpot* is the potential energy for the bond,
based on the current separation of the pair of atoms in the bond.

The value *force* is the magnitude of the force acting between the pair
of atoms in the bond, which is positive for a repulsive force and
negative for an attractive force.

The values *fx*, *fy*, and *fz* are the :math:`(x,y,z)` components of
the force on the first atom *i* in the bond due to the second atom *j*.
Mathematically, they are obtained by multiplying the value of *force*
from above with a unit vector created from the *dx*, *dy*, and *dz*
components of the distance vector also described above.  For bond styles
that apply non-central forces, such as :doc:`bond_style bpm/rotational
<bond_bpm_rotational>`, these values only include the :math:`(x,y,z)`
components of the normal force component.

The remaining properties are all computed for motion of the two atoms
relative to the center of mass (COM) velocity of the two atoms in the
bond.

The value *engvib* is the vibrational kinetic energy of the two atoms
in the bond, which is simply :math:`\frac12 m_1 v_1^2 + \frac12 m_2 v_2^2,`
where :math:`v_1` and :math:`v_2` are the magnitude of the velocity of the two
atoms along the bond direction, after the COM velocity has been subtracted from
each.

The value *engrot* is the rotational kinetic energy of the two atoms
in the bond, which is simply :math:`\frac12 m_1 v_1^2 + \frac12 m_2 v_2^2,`
where :math:`v_1` and :math:`v_2` are the magnitude of the velocity of the two
atoms perpendicular to the bond direction, after the COM velocity has been
subtracted from each.

The value *engtrans* is the translational kinetic energy associated
with the motion of the COM of the system itself, namely :math:`\frac12(m_1+m_2)
V_{\mathrm{cm}}^2`, where `Vcm` = magnitude of the velocity of the COM.

Note that these three kinetic energy terms are simply a partitioning of
the summed kinetic energy of the two atoms themselves.  That is, the total
kinetic energy is
:math:`\frac12 m_1 v_1^2 + \frac12 m_2 v_2^2` = engvib + engrot + engtrans,
where :math:`v_1` and :math:`v_2` are the magnitude of the velocities of the
two atoms, without any adjustment for the COM velocity.

The value *omega* is the magnitude of the angular velocity of the
two atoms around their COM position.

The value *velvib* is the magnitude of the relative velocity of the
two atoms in the bond towards each other.  A negative value means the
two atoms are moving toward each other; a positive value means they are
moving apart.

The value *v_name* can be used together with the *set* keyword to
compute a user-specified function of the bond distance.  The *name*
specified for the *v_name* value is the name of an :doc:`equal-style
variable <variable>` which should evaluate a formula based on a
variable which stores the bond distance.  This other variable must be
the :doc:`internal-style variable <variable>` specified by the *set*
keyword.  It is an internal-style variable, because this command
resets its value directly.  The internal-style variable does not need
to be defined in the input script (though it can be); if it is not
defined, then the *set* option creates an :doc:`internal-style
variable <variable>` with the specified name.

As an example, these commands can be added to the bench/in.rhodo
script to compute the length\ :math:`^2` of every bond in the system and
output the statistics in various ways:

.. code-block:: LAMMPS

   variable dsq equal v_d*v_d

   compute 1 all property/local batom1 batom2 btype
   compute 2 all bond/local engpot dist v_dsq set dist d
   dump 1 all local 100 tmp.dump c_1[*] c_2[*]

   compute 3 all reduce ave c_2[*] inputs local
   thermo_style custom step temp press c_3[*]

   fix 10 all ave/histo 10 10 100 0 6 20 c_2[3] mode vector file tmp.histo

The :doc:`dump local <dump>` command will output the energy, length,
and length\ :math:`^2` for every bond in the system.  The
:doc:`thermo_style <thermo_style>` command will print the average of
those quantities via the :doc:`compute reduce <compute_reduce>` command
with thermo output, and the :doc:`fix ave/histo <fix_ave_histo>`
command will histogram the length\ :math:`^2` values and write them to a file.

A bond style may define additional bond quantities which can be
accessed as *b1* to *bN*, where N is defined by the bond style.  Most
bond styles do not define any additional quantities, so N = 0.  An
example of ones that do are the :doc:`BPM bond styles <Howto_bpm>`
which store the reference state between two particles. See
individual bond styles for details.

When using *bN* with bond style *hybrid*, the output will be the Nth
quantity from the sub-style that computes the bonded interaction
(based on bond type).  If that sub-style does not define a *bN*,
the output will be 0.0.  The maximum allowed N is the maximum number
of quantities provided by any sub-style.

----------

The local data stored by this command is generated by looping over all
the atoms owned on a processor and their bonds.  A bond will only be
included if both atoms in the bond are in the specified compute group.
Any bonds that have been broken (see the :doc:`bond_style <bond_style>`
command) by setting their bond type to 0 are not included.  Bonds that
have been turned off (see the :doc:`fix shake <fix_shake>` or
:doc:`delete_bonds <delete_bonds>` commands) by setting their bond type
negative are written into the file, but their energy will be 0.0.

Note that as atoms migrate from processor to processor, there will be
no consistent ordering of the entries within the local vector or array
from one timestep to the next.  The only consistency that is
guaranteed is that the ordering on a particular timestep will be the
same for local vectors or arrays generated by other compute commands.
For example, bond output from the :doc:`compute property/local <compute_property_local>` command can be combined
with data from this command and output by the :doc:`dump local <dump>`
command in a consistent way.

Here is an example of how to do this:

.. code-block:: LAMMPS

   compute 1 all property/local btype batom1 batom2
   compute 2 all bond/local dist engpot
   dump 1 all local 1000 tmp.dump index c_1[*] c_2[*]

Output info
"""""""""""

This compute calculates a local vector or local array depending on the
number of values.  The length of the vector or number of rows in the
array is the number of bonds.  If a single value is specified, a local
vector is produced.  If two or more values are specified, a local
array is produced where the number of columns = the number of values.
The vector or array can be accessed by any command that uses local
values from a compute as input.  See the :doc:`Howto output <Howto_output>` page for an overview of LAMMPS output
options.

The output for *dist* will be in distance :doc:`units <units>`. The
output for *velvib* will be in velocity :doc:`units <units>`. The output
for *omega* will be in velocity/distance :doc:`units <units>`. The
output for *engtrans*, *engvib*, *engrot*, and *engpot* will be in
energy :doc:`units <units>`. The output for *force* will be in force
:doc:`units <units>`.

Restrictions
""""""""""""
 none

Related commands
""""""""""""""""

:doc:`dump local <dump>`, :doc:`compute property/local <compute_property_local>`

Default
"""""""

none
