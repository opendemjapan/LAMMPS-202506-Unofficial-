# preset that turns off all packages that require some form of external
# library or special compiler (fortran or cuda) or equivalent.

set(PACKAGES_WITH_LIB
  ADIOS
  APIP
  ATC
  AWPMD
  COMPRESS
  ELECTRODE
  GPU
  H5MD
  KIM
  KOKKOS
  LATBOLTZ
  LEPTON
  MACHDYN
  MDI
  ML-HDNNP
  ML-PACE
  ML-QUIP
  MOLFILE
  NETCDF
  PLUMED
  PYTHON
  QMMM
  SCAFACOS
  VORONOI
  VTK)

foreach(PKG ${PACKAGES_WITH_LIB})
  set(PKG_${PKG} OFF CACHE BOOL "" FORCE)
endforeach()
