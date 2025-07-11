Writing plugins
---------------

Plugins provide a mechanism to add functionality to a LAMMPS executable
without recompiling LAMMPS.  The functionality for this and the
:doc:`plugin command <plugin>` are implemented in the
:ref:`PLUGIN package <PKG-PLUGIN>` which must be installed to use plugins.

Plugins use the operating system's capability to load dynamic shared
object (DSO) files in a way similar shared libraries and then reference
specific functions in those DSOs.  Any DSO file with plugins has to
include an initialization function with a specific name,
"lammpsplugin_init", that has to follow specific rules described below.
When loading the DSO with the "plugin" command, this function is looked
up and called and will then register the contained plugin(s) with
LAMMPS.

When the environment variable ``LAMMPS_PLUGIN_PATH`` is set, then LAMMPS
will search the directory (or directories) listed in this path for files
with names that end in ``plugin.so`` (e.g. ``helloplugin.so``) and will
try to load the contained plugins automatically at start-up.  For
plugins that are loaded this way, the behavior of LAMMPS should be
identical to a binary where the corresponding code was compiled in
statically as a package.

From the programmer perspective this can work because of the object
oriented design of LAMMPS where all pair style commands are derived from
the class Pair, all fix style commands from the class Fix and so on and
usually only functions present in those base classes are called
directly.  When a :doc:`pair_style` command or :doc:`fix` command is
issued a new instance of such a derived class is created.  This is done
by a so-called factory function which is mapped to the style name.  Thus
when, for example, the LAMMPS processes the command ``pair_style lj/cut
2.5``, LAMMPS will look up the factory function for creating the
``PairLJCut`` class and then execute it.  The return value of that
function is a ``Pair *`` pointer and the pointer will be assigned to the
location for the currently active pair style.

A DSO file with a plugin thus has to implement such a factory function
and register it with LAMMPS so that it gets added to the map of available
styles of the given category.  To register a plugin with LAMMPS an
initialization function has to be present in the DSO file called
``lammpsplugin_init`` which is called with three ``void *`` arguments:
a pointer to the current LAMMPS instance, a pointer to the opened DSO
handle, and a pointer to the registration function.  The registration
function takes two arguments: a pointer to a ``lammpsplugin_t`` struct
with information about the plugin and a pointer to the current LAMMPS
instance.  Please see below for an example of how the registration is
done.

Members of ``lammpsplugin_t``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :widths: 15 85

   * - Member
     - Description
   * - version
     - LAMMPS Version string the plugin was compiled for
   * - style
     - Style of the plugin (pair, bond, fix, command, etc.)
   * - name
     - Name of the plugin style
   * - info
     - String with information about the plugin
   * - author
     - String with the name and email of the author
   * - creator.v1
     - Pointer to factory function for pair, bond, angle, dihedral, improper, kspace, command, or minimize styles
   * - creator.v2
     - Pointer to factory function for compute, fix, region, or run styles
   * - handle
     - Pointer to the open DSO file handle

Only one of the two alternate creator entries can be used at a time and
which of those is determined by the style of plugin. The "creator.v1"
element is for factory functions of supported styles computing forces
(i.e. pair, bond, angle, dihedral, or improper styles), command styles,
or minimize styles and the function takes as single argument the pointer
to the LAMMPS instance. The factory function is cast to the
``lammpsplugin_factory1`` type before assignment.  The "creator.v2"
element is for factory functions creating an instance of a fix, compute,
region, or run style and takes three arguments: a pointer to the LAMMPS
instance, an integer with the length of the argument list and a ``char
**`` pointer to the list of arguments. The factory function pointer
needs to be cast to the ``lammpsplugin_factory2`` type before
assignment.

Pair style example
^^^^^^^^^^^^^^^^^^

As an example, a hypothetical pair style plugin "morse2" implemented in
a class ``PairMorse2`` in the files ``pair_morse2.h`` and
``pair_morse2.cpp`` with the factory function and initialization
function would look like this:

.. code-block:: c++

  #include "lammpsplugin.h"
  #include "version.h"
  #include "pair_morse2.h"

  using namespace LAMMPS_NS;

  static Pair *morse2creator(LAMMPS *lmp)
  {
    return new PairMorse2(lmp);
  }

  extern "C" void lammpsplugin_init(void *lmp, void *handle, void *regfunc)
  {
    lammpsplugin_regfunc register_plugin = (lammpsplugin_regfunc) regfunc;
    lammpsplugin_t plugin;

    plugin.version = LAMMPS_VERSION;
    plugin.style   = "pair";
    plugin.name    = "morse2";
    plugin.info    = "Morse2 variant pair style v1.0";
    plugin.author  = "Axel Kohlmeyer (akohlmey@gmail.com)";
    plugin.creator.v1 = (lammpsplugin_factory1 *) &morse2creator;
    plugin.handle  = handle;
    (*register_plugin)(&plugin,lmp);
  }

The factory function in this example is called ``morse2creator()``.  It
receives a pointer to the LAMMPS class as only argument and thus has to
be assigned to the *creator.v1* member of the plugin struct and cast to
the ``lammpsplugin_factory1`` function pointer type.  It returns a
pointer to the allocated class instance derived from the ``Pair`` class.
This function may be declared static to avoid clashes with other
plugins.  The name of the derived class, ``PairMorse2``, however must be
unique inside the entire LAMMPS executable.

Fix style example
^^^^^^^^^^^^^^^^^

If the factory function is for a fix or compute, which take three
arguments (a pointer to the LAMMPS class, the number of arguments and the
list of argument strings), then the pointer type is ``lammpsplugin_factory2``
and it must be assigned to the *creator.v2* member of the plugin struct.
Below is an example for that:

.. code-block:: c++

  #include "lammpsplugin.h"
  #include "version.h"
  #include "fix_nve2.h"

  using namespace LAMMPS_NS;

  static Fix *nve2creator(LAMMPS *lmp, int argc, char **argv)
  {
    return new FixNVE2(lmp,argc,argv);
  }

  extern "C" void lammpsplugin_init(void *lmp, void *handle, void *regfunc)
  {
    lammpsplugin_regfunc register_plugin = (lammpsplugin_regfunc) regfunc;
    lammpsplugin_t plugin;

    plugin.version = LAMMPS_VERSION;
    plugin.style   = "fix";
    plugin.name    = "nve2";
    plugin.info    = "NVE2 variant fix style v1.0";
    plugin.author  = "Axel Kohlmeyer (akohlmey@gmail.com)";
    plugin.creator.v2 = (lammpsplugin_factory2 *) &nve2creator;
    plugin.handle  = handle;
    (*register_plugin)(&plugin,lmp);
  }

Command style example
^^^^^^^^^^^^^^^^^^^^^
Command styles also use the first variant of factory function as
demonstrated in the following example, which also shows that the
implementation of the plugin class may be within the same source
file as the plugin interface code:

.. code-block:: c++

   #include "lammpsplugin.h"

   #include "comm.h"
   #include "error.h"
   #include "command.h"
   #include "version.h"

   #include <cstring>

   namespace LAMMPS_NS {
     class Hello : public Command {
      public:
       Hello(class LAMMPS *lmp) : Command(lmp) {};
       void command(int, char **);
     };
   }

   using namespace LAMMPS_NS;

   void Hello::command(int argc, char **argv)
   {
      if (argc != 1) error->all(FLERR,"Illegal hello command");
      if (comm->me == 0)
        utils::logmesg(lmp,fmt::format("Hello, {}!\n",argv[0]));
   }

   static void hellocreator(LAMMPS *lmp)
   {
     return new Hello(lmp);
   }

   extern "C" void lammpsplugin_init(void *lmp, void *handle, void *regfunc)
   {
     lammpsplugin_t plugin;
     lammpsplugin_regfunc register_plugin = (lammpsplugin_regfunc) regfunc;

     plugin.version = LAMMPS_VERSION;
     plugin.style   = "command";
     plugin.name    = "hello";
     plugin.info    = "Hello world command v1.1";
     plugin.author  = "Axel Kohlmeyer (akohlmey@gmail.com)";
     plugin.creator.v1 = (lammpsplugin_factory1 *) &hellocreator;
     plugin.handle  = handle;
     (*register_plugin)(&plugin,lmp);
   }

Additional Details
^^^^^^^^^^^^^^^^^^

The initialization function **must** be called ``lammpsplugin_init``, it
**must** have C bindings and it takes three void pointers as arguments.
The first is a pointer to the LAMMPS class that calls it and it needs to
be passed to the registration function.  The second argument is a
pointer to the internal handle of the DSO file, this needs to be added
to the plugin info struct, so that the DSO can be closed and unloaded
when all its contained plugins are unloaded.  The third argument is a
function pointer to the registration function and needs to be stored
in a variable of ``lammpsplugin_regfunc`` type and then called with a
pointer to the ``lammpsplugin_t`` struct and the pointer to the LAMMPS
instance as arguments to register a single plugin.  There may be multiple
calls to multiple plugins in the same initialization function.

To register a plugin a struct of the ``lammpsplugin_t`` needs to be filled
with relevant info: current LAMMPS version string, kind of style, name of
style, info string, author string, pointer to factory function, and the
DSO handle.  The registration function is called with a pointer to the address
of this struct and the pointer of the LAMMPS class.  The registration function
will then add the factory function of the plugin style to the respective
style map under the provided name.  It will also make a copy of the struct
in a global list of all loaded plugins and update the reference counter for
loaded plugins from this specific DSO file.

The pair style itself (i.e. the PairMorse2 class in this example) can be
written just like any other pair style that is included in LAMMPS.  For
a plugin, the use of the ``PairStyle`` macro in the section encapsulated
by ``#ifdef PAIR_CLASS`` is not needed, since the mapping of the class
name to the style name is done by the plugin registration function with
the information from the ``lammpsplugin_t`` struct.  It may be included
in case the new code is intended to be later included in LAMMPS directly.

A plugin may be registered under an existing style name.  In that case
the plugin will override the existing code.  This can be used to modify
the behavior of existing styles or to debug new versions of them without
having to re-compile or re-install all of LAMMPS.

.. versionchanged:: 12Jun2025

When using the :doc:`clear <clear>` command, plugins are not unloaded
but restored to their respective style maps.  This also applies when
multiple LAMMPS instances are created and deleted through the library
interface.  The :doc:`plugin load <plugin>` load command may be issued
again, but for existing plugins they will be skipped.  To replace
plugins they must be explicitly unloaded with :doc:`plugin unload
<plugin>`.  When multiple LAMMPS instances are created concurrently, any
loaded plugins will be added to the global list of plugins, but are not
immediately available to any LAMMPS instance that was created before
loading the plugin.  To "import" such plugins, the :doc:`plugin restore
<plugin>` may be used.  Plugins are only removed when they are explicitly
unloaded or the LAMMPS interface is "finalized".

Compiling plugins
^^^^^^^^^^^^^^^^^

Plugins need to be compiled with the same compilers and libraries
(e.g. MPI) and compilation settings (MPI on/off, OpenMP, integer sizes)
as the LAMMPS executable and library.  Otherwise the plugin will likely
not load due to mismatches in the function signatures (LAMMPS is C++ so
scope, type, and number of arguments are encoded into the symbol names
and thus differences in them will lead to failed plugin load commands).
Compilation of the plugin can be managed via both, CMake or traditional
GNU makefiles.  Some examples that can be used as a template are in the
``examples/plugins`` folder.  The CMake script code has some small
adjustments to allow building the plugins for running unit tests with
them.

Another example that converts the KIM package into a plugin can be found
in the ``examples/kim/plugin`` folder.  No changes to the sources of the
KIM package themselves are needed; only the plugin interface and loader
code needs to be added.  This example only supports building with CMake,
but is probably a more typical example. To compile you need to run CMake
with ``-DLAMMPS_SOURCE_DIR=<path/to/lammps/src/folder>``.  Other
configuration setting are identical to those for compiling LAMMPS.

A second example for a plugin from a package is in the
``examples/PACKAGES/pace/plugin`` folder that will create a plugin from
the ML-PACE package.  In this case the bulk of the code is in a static
external library that is being downloaded and compiled first and then
combined with the pair style wrapper and the plugin loader.  This
example also contains a NSIS script that can be used to create an
Installer package for Windows (the mutual licensing terms of the
external library and LAMMPS conflict when distributing binaries, so the
ML-PACE package cannot be linked statically, but the LAMMPS headers
required to build the plugin are also available under a less restrictive
license).  This will automatically set the required environment variable
and launching a (compatible) LAMMPS binary will load and register the
plugin and the ML-PACE package can then be used as it was linked into
LAMMPS.
