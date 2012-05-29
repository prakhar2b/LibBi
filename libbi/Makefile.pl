##
## Generate a Makefile for compilation.
##
## @author Lawrence Murray <lawrence.murray@csiro.au>
## $Rev$
## $Date$
##

$SRCDIR = 'src';
$BUILDDIR = 'build';
$SHAREDLIB = 'libbi.so';
$STATICLIB = 'libbi.a';

# Compilers
$GCC = 'g++';
$ICC = 'icpc';
$CUDACC = 'nvcc';

# Common compile flags
$CPPINCLUDES = '-Isrc -I/tools/cuda/4.0/cuda/include/ -I/usr/local/cuda/include -I/tools/magma/1.0.0rc5/cuda4.0/include -I/usr/local/atlas/include';
$CXXFLAGS = "-Wall -fPIC `nc-config --cflags` $CPPINCLUDES";
$CUDACCFLAGS = "-arch sm_13 -Xptxas=\"-v\" -Xcompiler=\"-Wall -fPIC -fopenmp\" $CPPINCLUDES";
$LINKFLAGS = '';

# GCC options
$GCC_CXXFLAGS = '-fopenmp -Wno-parentheses'; # thrust gives lots of parentheses warnings, hiding others

# Intel C++ compiler options
$ICC_CXXFLAGS = '-malign-double -openmp -wd424 -wd981 -wd383 -wd1572 -wd869 -wd304 -wd444 -wd1418 -wd1782 -wd271';

# Release flags
$RELEASE_CXXFLAGS = ' -O3 -funroll-loops -fomit-frame-pointer -g';
$RELEASE_CUDACCFLAGS = ' -O3 -Xcompiler="-O3 -funroll-loops -fomit-frame-pointer -g"';

# Debugging flags
$DEBUG_CXXFLAGS = ' -g3';
$DEBUG_CUDACCFLAGS = ' -g';

# Profiling flags
$PROFILE_CXXFLAGS = ' -O1 -pg -g3';
$PROFILE_CUDACCFLAGS = ' -O1 -Xcompiler="-O1 -pg -g"';
$PROFILE_LINKFLAGS = ' -pg -g3';

# Disassembly flags
$DISASSEMBLE_CUDACCFLAGS = ' -keep';

# Walk through source
@files = ($SRCDIR);
while (@files) {
  $file = shift @files;
  if (-d $file) {
    # recurse into directory
    opendir(DIR, $file);
    push(@files, map { "$file/$_" } grep { !/^\./ } readdir(DIR));
    closedir(DIR);
  } elsif (-f $file && $file =~ /\.(cu|c|cpp)$/) {
    $ext = $1;

    # target name
    $target = $file;
    $target =~ s/^$SRCDIR/$BUILDDIR/;
    $target =~ s/\.\w+$/.$ext.o/;

    # determine compiler and appropriate flags
    if ($file =~ /\.cu$/) {
      $cc = $CUDACC;
      $ccstr = "\$(CUDACC)";
      $flags = $CUDACCFLAGS;
      $flagstr = "\$(CUDACCFLAGS)";
    } else {
      $cc = $GCC;
      $ccstr = "\$(CXX)";
      $flags = $CXXFLAGS;
      $flagstr = "\$(CXXFLAGS)";
    }
  
    # determine dependencies of this source and construct Makefile target
    $target =~ /(.*)\//;
    $dir = $1;
    $dirs{$dir} = 1;

    $command = `$cc $flags -M $file`;
    $command =~ s/.*?\:\w*//;
    $command = "$target: " . $command;
    $command .= "\tmkdir -p $dir\n";
    $command .= "\t$ccstr -o $target $flagstr -c $file\n";
    #$command .= "\trm -f *.linkinfo\n";
    push(@targets, $target);
    push(@commands, $command);
  }
}

# Write Makefile
print <<End;
ifdef ENABLE_CONFIG
include config.mk
endif

CXXFLAGS=$CXXFLAGS
LINKFLAGS=$LINKFLAGS
CUDACC=$CUDACC
CUDACCFLAGS=$CUDACCFLAGS

GCC_CXXFLAGS=$GCC_CXXFLAGS
GCC_LINKFLAGS=$GCC_LINKFLAGS
ICC_CXXFLAGS=$ICC_CXXFLAGS
ICC_LINKFLAGS=$ICC_LINKFLAGS

DEBUG_CXXFLAGS=$DEBUG_CXXFLAGS
DEBUG_CUDACCFLAGS=$DEBUG_CUDACCFLAGS
DEBUG_LINKFLAGS=$DEBUG_LINKFLAGS

RELEASE_CXXFLAGS=$RELEASE_CXXFLAGS
RELEASE_CUDACCFLAGS=$RELEASE_CUDACCFLAGS
RELEASE_LINKFLAGS=$RELEASE_LINKFLAGS

PROFILE_CXXFLAGS=$PROFILE_CXXFLAGS
PROFILE_CUDACCFLAGS=$PROFILE_CUDACCFLAGS
PROFILE_LINKFLAGS=$PROFILE_LINKFLAGS

DISASSEMBLE_CXXFLAGS=$DISASSEMBLE_CXXFLAGS
DISASSEMBLE_CUDACCFLAGS=$DISASSEMBLE_CUDACCFLAGS
DISASSEMBLE_LINKFLAGS=$DISASSEMBLE_LINKFLAGS

ifdef ENABLE_INTEL
CXX=$ICC
LINKER=$ICC
CXXFLAGS += \$(ICC_CXXFLAGS)
LINKFLAGS += \$(ICC_LINKFLAGS)
else
CXX=$GCC
LINKER=$GCC
CXXFLAGS += \$(GCC_CXXFLAGS)
LINKFLAGS += \$(GCC_LINKFLAGS)
endif

ifdef ENABLE_DOUBLE
CUDACCFLAGS += -DENABLE_DOUBLE
CXXFLAGS += -DENABLE_DOUBLE
else
ifdef ENABLE_FAST_MATH
CUDACCFLAGS += -use_fast_math
endif
ifdef ENABLE_TEXTURE
CUDACCFLAGS += -DENABLE_TEXTURE
CXXFLAGS += -DENABLE_TEXTURE
endif
endif

ifdef ENABLE_CPU
CUDACCFLAGS += -DENABLE_CPU -Xcompiler -DTHRUST_DEVICE_BACKEND=THRUST_DEVICE_BACKEND_OMP
CXXFLAGS += -DENABLE_CPU -DTHRUST_DEVICE_BACKEND=THRUST_DEVICE_BACKEND_OMP
endif

ifdef ENABLE_SSE
CXXFLAGS += -DENABLE_SSE -msse3
CUDACCFLAGS += -DENABLE_SSE
endif

ifdef ENABLE_MKL
CXXFLAGS += -DENABLE_MKL
CUDACCFLAGS += -DENABLE_MKL
endif

ifdef ENABLE_ODE
if \$(ENABLE_ODE) == "dopri5"
CUDACCFLAGS += -DENABLE_ODE_DOPRI5
CXXFLAGS += -DENABLE_ODE_DOPRI5
elif \$(ENABLE_ODE) == "rk4"
CUDACCFLAGS += -DENABLE_ODE_RK4
CXXFLAGS += -DENABLE_ODE_RK4
else
CUDACCFLAGS += -DENABLE_ODE_RK43
CXXFLAGS += -DENABLE_ODE_RK43
endif

ifdef NDEBUG
CUDACCFLAGS += -DNDEBUG
CXXFLAGS += -DNDEBUG
endif

ifdef DEBUG
CUDACCFLAGS += \$(DEBUG_CUDACCFLAGS)
CXXFLAGS += \$(DEBUG_CXXFLAGS)
LINKFLAGS += \$(DEBUG_LINKFLAGS)
endif

ifdef RELEASE
CUDACCFLAGS += \$(RELEASE_CUDACCFLAGS)
CXXFLAGS += \$(RELEASE_CXXFLAGS)
LINKFLAGS += \$(RELEASE_LINKFLAGS)
endif

ifdef PROFILE
CUDACCFLAGS += \$(PROFILE_CUDACCFLAGS)
CXXFLAGS += \$(PROFILE_CXXFLAGS)
LINKFLAGS += \$(PROFILE_LINKFLAGS)
endif

ifdef DISASSEMBLE
CUDACCFLAGS += \$(DISASSEMBLE_CUDACCFLAGS)
CXXFLAGS += \$(DISASSEMBLE_CXXFLAGS)
LINKFLAGS += \$(DISASSEMBLE_LINKFLAGS)
endif

End

# All target
print "all: $BUILDDIR/$SHAREDLIB\n\n";

# Artefacts
print "$BUILDDIR/$SHAREDLIB: ";
print join(" \\\n    ", @targets);
print "\n";
print "\t\$(CXX) -shared -o $BUILDDIR/$SHAREDLIB \$(LINKFLAGS) ";
print join(' ', @targets);
print "\n\n";

#print "$BUILDDIR/$STATICLIB: ";
#print join(" \\\n    ", @targets);
#print "\n";
#print "\tar rcs $BUILDDIR/$STATICLIB \$(LINKFLAGS) ";
#print join(' ', @targets);
#print "\n\n";

# Targets
foreach $command (@commands) {
  print "$command\n";
}

# Build directory tree targets
foreach $dir (sort keys %dirs) {
  print "$dir:\n\tmkdir -p $dir\n\n";
}

# Clean target
print "clean:\n\trm -rf $BUILDDIR";
