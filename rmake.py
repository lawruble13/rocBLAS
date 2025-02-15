#!/usr/bin/python3

"""Copyright (C) 2020-2022 Advanced Micro Devices, Inc. All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell cop-
   ies of the Software, and to permit persons to whom the Software is furnished
   to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM-
   PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNE-
   CTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""

import re
import sys
import os
import platform
import subprocess
import argparse
import pathlib
from fnmatch import fnmatchcase

args = {}
param = {}
OS_info = {}

def parse_args():
    """Parse command-line arguments"""
    global OS_info

    parser = argparse.ArgumentParser(description="""Checks build arguments""")

    parser.add_argument('-a', '--architecture', dest='gpu_architecture', required=False, default="all",
                        help='Set GPU architectures, e.g. all, gfx000, gfx803, gfx906:xnack-, gfx1030, gfx1100, gfx1101, gfx1102 (optional, default: all)')

    parser.add_argument(       '--address-sanitizer', dest='address_sanitizer', required=False, default=False,
                        help='uild with address sanitizer enabled. (optional, default: False')

    parser.add_argument('-b', '--branch', dest='tensile_tag', type=str, required=False, default="",
                        help='Specify the Tensile repository branch or tag to use.(eg. develop, mybranch or <commit hash> )')

    parser.add_argument(      '--build_dir', type=str, required=False, default = "build",
                        help='Specify path to configure & build process output directory.(optional, default: ./build)')

    parser.add_argument('-c', '--clients', dest='build_clients', required=False, default = False, action='store_true',
                        help='Build the library clients benchmark and gtest (optional, default: False,Generated binaries will be located at builddir/clients/staging)')

    parser.add_argument(     '--clients_no_fortran', required=False, default=False, action='store_true',
                        help='When building clients, build them without Fortran API testing or Fortran examples. (optional, default:False')

    parser.add_argument(     '--clients-only', dest='clients_only', required=False, default = False, action='store_true',
                        help='Skip building the library and only build the clients with a pre-built library.')

    parser.add_argument(      '--cmake-arg', dest='cmake_args', type=str, required=False, default="",
                        help='Forward the given arguments to CMake when configuring the build.')

    parser.add_argument(      '--cmake-darg', dest='cmake_dargs', required=False, action='append', default=[],
                        help='List of additional cmake defines for builds (optional, e.g. CMAKE)')

    parser.add_argument(      '--codecoverage', required=False, default=False, action='store_true',
                        help='Code coverage build. Requires Debug (-g|--debug) or RelWithDebInfo mode (-k|--relwithdebinfo), (optional, default: False)')

    parser.add_argument('-f', '--fork', dest='tensile_fork', type=str, required=False, default="",
                        help='Specify the username to fork the Tensile GitHub repository (e.g., ROCmSoftwarePlatform or MyUserName)')

    parser.add_argument('-g', '--debug', required=False, default = False,  action='store_true',
                        help='Build in Debug mode (optional, default: False)')

    parser.add_argument('-i', '--install', required=False, default = False, dest='install', action='store_true',
                        help='Generate and install library package after build. (optional, default: False)')

    parser.add_argument('-j', '--jobs', type=int, required=False, default = OS_info["NUM_PROC"],
                        help='Specify number of parallel jobs to launch, increases memory usage (Default logical core count) ')

    parser.add_argument('-k', '--relwithdebinfo', required=False, default = False, action='store_true',
                        help='Build in Release with Debug Info (optional, default: False)')

    parser.add_argument('-l', '--logic', dest='tensile_logic', type=str, required=False, default="asm_full",
                        help='Specify the Tensile logic target, e.g., asm_full, asm_lite, etc. (optional, default: asm_full)')

    parser.add_argument(    '--lazy-library-loading', dest='tensile_lazy_library_loading', required=False, default=True, action='store_true',
                        help='Enable on-demand loading of Tensile Library files, speeds up the rocblas initialization. (Default is enabled)')

    parser.add_argument(    '--no-lazy-library-loading', dest='tensile_lazy_library_loading', required=False, default=True, action='store_false',
                        help='Disable on-demand loading of Tensile Library files. (Default is enabled)')

    parser.add_argument(     '--library-path', dest='library_dir_installed', type=str, required=False, default = "",
                        help='Specify path to a pre-built rocBLAS library, when building clients only using --clients-only flag. (optional, default: /opt/rocm/rocblas)')

    parser.add_argument('-n', '--no_tensile', dest='build_tensile', required=False, default=True, action='store_false',
                        help='Build a subset of rocBLAS library which does not require Tensile.')

    parser.add_argument(     '--no-merge-files', dest='merge_files', required=False, default=True, action='store_false',
                        help='Disable Tensile_MERGE_FILES (optional)')

    parser.add_argument(     '--merge-architectures', dest='merge_architectures', required=False, default=False, action='store_true',
                        help='Merge TensileLibrary files for different architectures into single file (optional, behavior in ROCm 5.1 and earlier)')

    parser.add_argument(     '--no-merge-architectures', dest='merge_architectures', required=False, default=False, action='store_false',
                        help='Keep TensileLibrary files separated by architecture (optional)')

    parser.add_argument(     '--no-msgpack', dest='tensile_msgpack_backend', required=False, default=True, action='store_false',
                        help='Build Tensile backend not to use MessagePack and so use YAML (optional)')

    parser.add_argument( '-r', '--relocatable', required=False, default=False, action='store_true',
                        help='Linux only: Add RUNPATH (based on ROCM_RPATH) and remove ldconf entry.')

    parser.add_argument(      '--rm-legacy-include-dir', dest='rm_legacy_include_dir', required=False, default=False, action='store_true',
                        help='Remove legacy include dir Packaging added for file/folder reorg backward compatibility.')

    parser.add_argument(      '--rocm_dev', type=str, required=False, default = "",
                        help='Specify specific rocm-dev version (e.g. 4.5.0).')

    parser.add_argument(      '--run_header_testing', required=False, default=False, action='store_true',
                        help='Run post build header testing. (options, default: False')

    parser.add_argument(      '--skip_ld_conf_entry', required=False, default = False,
                        help='Linux only: Skip ld.so.conf entry.')

    parser.add_argument(      '--static', required=False, default = False, dest='static_lib', action='store_true',
                        help='Build rocblas as a static library. (optional, default: False)')

    parser.add_argument('-t', '--test_local_path', dest='tensile_test_local_path', type=str, required=False, default="",
                        help='Use a local path for Tensile instead of remote GIT repo (optional)')

    parser.add_argument(      '--upgrade_tensile_venv_pip', required=False, default=False,
                        help='Upgrade PIP version during Tensile installation (optional, default: False)')

    parser.add_argument('-u', '--use-custom-version', dest='tensile_version', type=str, required=False, default="",
                        help='Ignore Tensile version and just use the Tensile tag (optional)')

    parser.add_argument('-v', '--verbose', required=False, default = False, action='store_true',
                        help='Verbose build (optional, default: False)')

    parser.add_argument('-X', '--exclude-checks', dest='exclude_checks', required=False, default = False, action='store_true',
                        help='Exclude compiler and configuration checks (optional, default: False)')

    return parser.parse_args()

def os_detect():
    global OS_info
    if os.name == "nt":
        OS_info["ID"] = platform.system()
    else:
        inf_file = "/etc/os-release"
        if os.path.exists(inf_file):
            with open(inf_file) as f:
                for line in f:
                    if "=" in line:
                        k,v = line.strip().split("=")
                        OS_info[k] = v.replace('"','')
    OS_info["NUM_PROC"] = os.cpu_count()
    print(OS_info)

def create_dir(dir_path):
    full_path = ""
    if os.path.isabs(dir_path):
        full_path = dir_path
    else:
        full_path = os.path.join( os.getcwd(), dir_path )
    pathlib.Path(full_path).mkdir(parents=True, exist_ok=True)
    return

def delete_dir(dir_path) :
    if (not os.path.exists(dir_path)):
        return
    if os.name == "nt":
        run_cmd( "RMDIR" , f"/S /Q {dir_path}")
    else:
        run_cmd( "rm" , f"-rf {dir_path}")

def cmake_path(os_path):
    if os.name == "nt":
        return os_path.replace("\\", "/")
    else:
        return os.path.realpath(os_path);

def config_cmd():
    global args
    global OS_info
    cwd_path = os.getcwd()
    cmake_executable = "cmake"
    cmake_options = []
    src_path = cmake_path(cwd_path)
    cmake_platform_opts = []
    if os.name == "nt":
        generator = f"-G Ninja"
        cmake_options.append( generator )

        # not really rocm path as none exist, HIP_DIR set in toolchain is more important
        rocm_path = os.getenv( 'ROCM_CMAKE_PATH', "C:/github/rocm-cmake-master/share/rocm")
        # CPACK_PACKAGING_INSTALL_PREFIX= defined as blank as it is appended to end of path for archive creation
        cmake_platform_opts.append( f"-DCPACK_PACKAGING_INSTALL_PREFIX=" )
        cmake_platform_opts.append( f'-DCMAKE_INSTALL_PREFIX="C:/hipSDK"' )
        toolchain = os.path.join( src_path, "toolchain-windows.cmake" )
    else:
        rocm_path = os.getenv( 'ROCM_PATH', "/opt/rocm")
        cmake_platform_opts.append( f"-DROCM_DIR:PATH={rocm_path} -DCPACK_PACKAGING_INSTALL_PREFIX={rocm_path}" )
        cmake_platform_opts.append( f'-DCMAKE_INSTALL_PREFIX="rocblas-install"' )
        toolchain = "toolchain-linux.cmake"

    print( f"Build source path: {src_path}")

    tools = f"-DCMAKE_TOOLCHAIN_FILE={toolchain}"
    cmake_options.append( tools )

    cmake_options.extend( cmake_platform_opts )

    if args.cmake_args:
        cmake_options.append( args.cmake_args )

    cmake_base_options = f"-DROCM_PATH={rocm_path} -DCMAKE_PREFIX_PATH:PATH={rocm_path}"
    cmake_options.append( cmake_base_options )

    # packaging options
    cmake_pack_options = f"-DCPACK_SET_DESTDIR=OFF"
    cmake_options.append( cmake_pack_options )

    if os.getenv('CMAKE_CXX_COMPILER_LAUNCHER'):
        cmake_options.append( f'-DCMAKE_CXX_COMPILER_LAUNCHER={os.getenv("CMAKE_CXX_COMPILER_LAUNCHER")}' )



    # build type
    cmake_config = ""
    build_dir = os.path.realpath(args.build_dir)
    if args.debug:
        build_path = os.path.join(build_dir, "debug")
        cmake_config="Debug"
    elif args.relwithdebinfo:
        build_path = os.path.join(build_dir, "release-debug")
        cmake_config="RelWithDebInfo"
    else:
        build_path = os.path.join(build_dir, "release")
        cmake_config="Release"

    cmake_options.append( f"-DCMAKE_BUILD_TYPE={cmake_config}" )

    if args.codecoverage:
        if args.debug or args.relwithdebinfo:
            cmake_options.append( f"-DBUILD_CODE_COVERAGE=ON" )
        else:
            os.exit( "*** Code coverage is not supported for Release build! Aborting. ***" )

    if args.address_sanitizer:
        cmake_options.append( f"-DBUILD_ADDRESS_SANITIZER=ON" )

    # clean
    delete_dir( build_path )

    create_dir( os.path.join(build_path, "clients") )
    os.chdir( build_path )

    if args.static_lib:
        cmake_options.append( f"-DBUILD_SHARED_LIBS=OFF" )

    if args.relocatable:
        rocm_rpath = os.getenv( 'ROCM_RPATH', "/opt/rocm/lib:/opt/rocm/lib64")
        cmake_options.append( f'-DCMAKE_SHARED_LINKER_FLAGS=" -Wl,--enable-new-dtags -Wl,--rpath,{rocm_rpath}"' )

    if args.skip_ld_conf_entry or args.relocatable:
        cmake_options.append( f"-DROCM_DISABLE_LDCONFIG=ON" )

    if args.build_clients:
        cmake_build_dir = cmake_path(build_dir)
        cmake_options.append( f"-DBUILD_CLIENTS_TESTS=ON -DBUILD_CLIENTS_BENCHMARKS=ON -DBUILD_CLIENTS_SAMPLES=ON -DBUILD_DIR={cmake_build_dir}" )
        if os.name != "nt":
            cmake_options.append( f"-DLINK_BLIS=ON" )
        if args.clients_no_fortran:
            cmake_options.append( f"-DBUILD_FORTRAN_CLIENTS=OFF" )

    if args.clients_only:
        if args.library_dir_installed:
            library_dir = args.library_dir_installed
        else:
            library_dir = f"{rocm_path}/rocblas"
        cmake_lib_dir = cmake_path(library_dir)
        cmake_options.append( f"-DSKIP_LIBRARY=ON -DROCBLAS_LIBRARY_DIR={cmake_lib_dir}" )

    # not just for tensile
    cmake_options.append( f"-DAMDGPU_TARGETS={args.gpu_architecture}" )

    if not args.build_tensile:
        cmake_options.append( f"-DBUILD_WITH_TENSILE=OFF" )
    else:
        cmake_options.append( f"-DTensile_CODE_OBJECT_VERSION=V3" )
        if args.tensile_logic:
            cmake_options.append( f"-DTensile_LOGIC={args.tensile_logic}" )
        if args.tensile_fork:
            cmake_options.append( f"-Dtensile_fork={args.tensile_fork}" )
        if args.tensile_tag:
            cmake_options.append( f"-Dtensile_tag={args.tensile_tag}" )
        if args.tensile_test_local_path:
            cmake_options.append( f"-DTensile_TEST_LOCAL_PATH={args.tensile_test_local_path}" )
        if args.tensile_version:
            cmake_options.append( f"-DTENSILE_VERSION={args.tensile_version}" )
        if not args.merge_files:
            cmake_options.append( f"-DTensile_MERGE_FILES=OFF" )
        if args.upgrade_tensile_venv_pip:
            cmake_options.append( f"-DTENSILE_VENV_UPGRADE_PIP=ON" )
        if not args.merge_architectures:
            cmake_options.append( f"-DTensile_SEPARATE_ARCHITECTURES=ON" )
        if args.tensile_lazy_library_loading:
            cmake_options.append( f"-DTensile_LAZY_LIBRARY_LOADING=ON" )
        if args.tensile_msgpack_backend:
            cmake_options.append( f"-DTensile_LIBRARY_FORMAT=msgpack" )
        else:
            cmake_options.append( f"-DTensile_LIBRARY_FORMAT=yaml" )
        if args.jobs != OS_info["NUM_PROC"]:
            cmake_options.append( f"-DTensile_CPU_THREADS={str(args.jobs)}" )

    if args.rm_legacy_include_dir:
        cmake_options.append( f"-DBUILD_FILE_REORG_BACKWARD_COMPATIBILITY=OFF" )
    else:
        cmake_options.append( f"-DBUILD_FILE_REORG_BACKWARD_COMPATIBILITY=ON" )

    if args.run_header_testing:
        cmake_options.append( f"-DRUN_HEADER_TESTING=ON")

    if args.exclude_checks:
        cmake_options.append( f"-DCONFIG_NO_COMPILER_CHECKS=ON" )

    if args.cmake_dargs:
        for i in args.cmake_dargs:
          cmake_options.append( f"-D{i}" )

    cmake_options.append( f"{src_path}")
    cmd_opts = " ".join(cmake_options)

    return cmake_executable, cmd_opts


def make_cmd():
    global args
    global OS_info

    make_options = []

    if os.name == "nt":
        # the CMAKE_BUILD_PARALLEL_LEVEL currently doesn't work for windows build, so using -j
        #make_executable = f"cmake.exe -DCMAKE_BUILD_PARALLEL_LEVEL=4 --build . " # ninja
        make_executable = f"ninja.exe -j {args.jobs}"
        if args.verbose:
          make_options.append( "--verbose" )
        make_options.append( "all" ) # for cmake "--target all" )
        if args.install:
          make_options.append( "package install" ) # for cmake "--target package --target install" )
    else:
        make_executable = f"make -j{args.jobs}"
        if args.verbose:
          make_options.append( "VERBOSE=1" )
        if not args.clients_only:
         make_options.append( "install" )
    cmd_opts = " ".join(make_options)

    return make_executable, cmd_opts

def run_cmd(exe, opts):
    program = f"{exe} {opts}"
    print(program)
    proc = subprocess.run(program, check=True, stderr=subprocess.STDOUT, shell=True)
    return proc.returncode

def main():
    global args
    os_detect()
    args = parse_args()

    # configure
    exe, opts = config_cmd()
    run_cmd(exe, opts)

    # make
    exe, opts = make_cmd()
    run_cmd(exe, opts)

if __name__ == '__main__':
    main()

