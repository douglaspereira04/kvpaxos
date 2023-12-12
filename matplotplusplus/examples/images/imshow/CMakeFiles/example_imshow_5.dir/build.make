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
include examples/images/imshow/CMakeFiles/example_imshow_5.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include examples/images/imshow/CMakeFiles/example_imshow_5.dir/compiler_depend.make

# Include the progress variables for this target.
include examples/images/imshow/CMakeFiles/example_imshow_5.dir/progress.make

# Include the compile flags for this target's objects.
include examples/images/imshow/CMakeFiles/example_imshow_5.dir/flags.make

examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o: examples/images/imshow/CMakeFiles/example_imshow_5.dir/flags.make
examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o: examples/images/imshow/imshow_5.cpp
examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o: examples/images/imshow/CMakeFiles/example_imshow_5.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/douglas/Documentos/non-blocking-cpp/matplotplusplus/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o -MF CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o.d -o CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o -c /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow/imshow_5.cpp

examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/example_imshow_5.dir/imshow_5.cpp.i"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow/imshow_5.cpp > CMakeFiles/example_imshow_5.dir/imshow_5.cpp.i

examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/example_imshow_5.dir/imshow_5.cpp.s"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow/imshow_5.cpp -o CMakeFiles/example_imshow_5.dir/imshow_5.cpp.s

# Object files for target example_imshow_5
example_imshow_5_OBJECTS = \
"CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o"

# External object files for target example_imshow_5
example_imshow_5_EXTERNAL_OBJECTS =

examples/images/imshow/example_imshow_5: examples/images/imshow/CMakeFiles/example_imshow_5.dir/imshow_5.cpp.o
examples/images/imshow/example_imshow_5: examples/images/imshow/CMakeFiles/example_imshow_5.dir/build.make
examples/images/imshow/example_imshow_5: source/matplot/libmatplot.a
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/libjpeg.so
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/libtiff.so
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/libz.so
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/libpng.so
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/libz.so
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/libpng.so
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/liblapack.so
examples/images/imshow/example_imshow_5: /usr/lib/x86_64-linux-gnu/libblas.so
examples/images/imshow/example_imshow_5: source/3rd_party/libnodesoup.a
examples/images/imshow/example_imshow_5: examples/images/imshow/CMakeFiles/example_imshow_5.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/douglas/Documentos/non-blocking-cpp/matplotplusplus/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable example_imshow_5"
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/example_imshow_5.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/images/imshow/CMakeFiles/example_imshow_5.dir/build: examples/images/imshow/example_imshow_5
.PHONY : examples/images/imshow/CMakeFiles/example_imshow_5.dir/build

examples/images/imshow/CMakeFiles/example_imshow_5.dir/clean:
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow && $(CMAKE_COMMAND) -P CMakeFiles/example_imshow_5.dir/cmake_clean.cmake
.PHONY : examples/images/imshow/CMakeFiles/example_imshow_5.dir/clean

examples/images/imshow/CMakeFiles/example_imshow_5.dir/depend:
	cd /home/douglas/Documentos/non-blocking-cpp/matplotplusplus && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/douglas/Documentos/non-blocking-cpp/matplotplusplus /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow /home/douglas/Documentos/non-blocking-cpp/matplotplusplus /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow /home/douglas/Documentos/non-blocking-cpp/matplotplusplus/examples/images/imshow/CMakeFiles/example_imshow_5.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/images/imshow/CMakeFiles/example_imshow_5.dir/depend

