# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/douglas/Documentos/non-blocking-cpp/matplotplusplus

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/douglas/Documentos/non-blocking-cpp/matplotplusplus

# Include any dependencies generated for this target.
include examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/compiler_depend.make

# Include the progress variables for this target.
include examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/progress.make

# Include the compile flags for this target's objects.
include examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/flags.make

examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.o: examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/flags.make
examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.o: examples/appearance/multiplot/hold/hold_2.cpp
examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.o: examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/douglas/Documentos/non-blocking-cpp/matplotplusplus/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.o"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.o -MF CMakeFiles/example_hold_2.dir/hold_2.cpp.o.d -o CMakeFiles/example_hold_2.dir/hold_2.cpp.o -c /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold/hold_2.cpp

examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/example_hold_2.dir/hold_2.cpp.i"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold/hold_2.cpp > CMakeFiles/example_hold_2.dir/hold_2.cpp.i

examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/example_hold_2.dir/hold_2.cpp.s"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold/hold_2.cpp -o CMakeFiles/example_hold_2.dir/hold_2.cpp.s

# Object files for target example_hold_2
example_hold_2_OBJECTS = \
"CMakeFiles/example_hold_2.dir/hold_2.cpp.o"

# External object files for target example_hold_2
example_hold_2_EXTERNAL_OBJECTS =

examples/appearance/multiplot/hold/example_hold_2: examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/hold_2.cpp.o
examples/appearance/multiplot/hold/example_hold_2: examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/build.make
examples/appearance/multiplot/hold/example_hold_2: source/matplot/libmatplot.a
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/libjpeg.so
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/libtiff.so
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/libz.so
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/libpng.so
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/libz.so
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/libpng.so
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/liblapack.so
examples/appearance/multiplot/hold/example_hold_2: /usr/lib/x86_64-linux-gnu/libblas.so
examples/appearance/multiplot/hold/example_hold_2: source/3rd_party/libnodesoup.a
examples/appearance/multiplot/hold/example_hold_2: examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/douglas/Documentos/non-blocking-cpp/matplotplusplus/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable example_hold_2"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/example_hold_2.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/build: examples/appearance/multiplot/hold/example_hold_2
.PHONY : examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/build

examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/clean:
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold && $(CMAKE_COMMAND) -P CMakeFiles/example_hold_2.dir/cmake_clean.cmake
.PHONY : examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/clean

examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/depend:
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/douglas/Documentos/non-blocking-cpp/matplotplusplus /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold /home/douglas/Documentos/non-blocking-cpp/matplotplusplus /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/appearance/multiplot/hold/CMakeFiles/example_hold_2.dir/depend

