# preset that will enable hip (clang/clang++) with support for MPI and OpenMP (on Linux boxes)

# prefer flang over gfortran, if available
find_program(CLANG_FORTRAN NAMES flang gfortran f95)
set(ENV{OMPI_FC} ${CLANG_FORTRAN})

set(CMAKE_CXX_COMPILER "hipcc" CACHE STRING "" FORCE)
set(CMAKE_C_COMPILER "hipcc" CACHE STRING "" FORCE)
set(CMAKE_Fortran_COMPILER ${CLANG_FORTRAN} CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Wall -Wextra -g -O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_Fortran_FLAGS_DEBUG "-Wall -Wextra -g -std=f2003" CACHE STRING "" FORCE)
set(CMAKE_Fortran_FLAGS_RELWITHDEBINFO "-Wall -Wextra -g -O2 -DNDEBUG -std=f2003" CACHE STRING "" FORCE)
set(CMAKE_Fortran_FLAGS_RELEASE "-O3 -DNDEBUG -std=f2003" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -g" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Wall -Wextra -g -O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" FORCE)

set(MPI_CXX "hipcc" CACHE STRING "" FORCE)
set(MPI_CXX_COMPILER "mpicxx" CACHE STRING "" FORCE)
set(MPI_C "hipcc" CACHE STRING "" FORCE)
set(MPI_C_COMPILER "mpicc" CACHE STRING "" FORCE)

# change as needed. This is for Fedora Linux 41 and 42
set(_libomp_root "/usr/lib/clang/18")
# we need to explicitly specify the include dir, since hipcc will
# compile each file twice and doesn't find omp.h the second time

unset(HAVE_OMP_H_INCLUDE CACHE)
set(OpenMP_C "hipcc" CACHE STRING "" FORCE)
set(OpenMP_C_FLAGS "-fopenmp=libomp -I${_libomp_root}/include" CACHE STRING "" FORCE)
set(OpenMP_C_LIB_NAMES "omp" CACHE STRING "" FORCE)
set(OpenMP_CXX "hipcc" CACHE STRING "" FORCE)
set(OpenMP_CXX_FLAGS "-fopenmp=libomp -I${_libomp_root}/include" CACHE STRING "" FORCE)
set(OpenMP_CXX_LIB_NAMES "omp" CACHE STRING "" FORCE)
set(OpenMP_omp_LIBRARY "libomp.so" CACHE PATH "" FORCE)
