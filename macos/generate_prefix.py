#!/usr/bin/env python3
import argparse
import sys
import os
import os.path
import shutil
import subprocess
import glob

def fix_id(target, new_path):
  subprocess.check_output(["install_name_tool", "-id", new_path, target], stderr=subprocess.STDOUT).decode("utf-8")

def fix_dep(target, old_path, new_path):
  subprocess.check_output(["install_name_tool", "-change", old_path, new_path, target], stderr=subprocess.STDOUT).decode("utf-8")

def find_deps(path):
  result = []
  otool_output = subprocess.check_output(["otool", "-L", path]).decode("utf-8")
  for otool_line in otool_output.split("\n")[1:]:
    otool_line = otool_line[1:].split(" ")[0]
    if (otool_line.startswith("/usr/lib/")):
      continue
    if (otool_line.startswith("/System/Library/")):
      continue
    if (otool_line.startswith("/Library/")):
      continue
    if (otool_line.startswith("@rpath")):
      continue
    if (otool_line == ""):
      continue
    result.append(otool_line)
  return result

def copy_lib(dst_prefix, path, dst_subdir=""):
  dst_name = dst_subdir + os.path.basename(path)
  # https://joaoventura.net/blog/2016/embeddable-python-osx/
  # Note we will need to copy the site-packages and friends to actually run python code
  if dst_name == "Python":
    suffix = os.path.basename(os.path.dirname(path))
    dst_name = "libpython." + suffix + ".dylib"
  dst_path = os.path.join(dst_prefix, "lib", dst_name)

  our_fixed_path = os.path.join("@executable_path/../lib/", dst_name)

  if not os.path.exists(dst_path):
    shutil.copyfile(path, dst_path)
    os.chmod(dst_path, 0o655)
    libs = find_deps(dst_path)
    if not dst_path.endswith("so"):
      # For dylibs The first value output for the lib is it's "id" not a dependency
      fix_id(dst_path, our_fixed_path)
      libs = libs[1:]

    for lib_path in libs:
      fixed_path = copy_lib(dst_prefix, lib_path)
      fix_dep(dst_path, lib_path, fixed_path)
  
  return our_fixed_path

def copy_bin(dst_prefix, src_prefix, filename):
  src_path = os.path.join(src_prefix, "bin", filename)
  dst_path = os.path.join(dst_prefix, "bin", filename)
  shutil.copyfile(src_path, dst_path)
  os.chmod(dst_path, 0o755)
  libs = find_deps(dst_path)
  for lib_path in libs:
    fixed_path = copy_lib(dst_prefix, lib_path)
    fix_dep(dst_path, lib_path, fixed_path)

bin_filenames = ["nwchem", "mpirun", "obminimize"]

def build_prefix(src_prefx_path, prefix_name):
  if (os.path.exists(prefix_name)):
    shutil.rmtree(prefix_name)

  os.makedirs(os.path.join(prefix_name,"bin"),exist_ok=True)
  os.makedirs(os.path.join(prefix_name,"lib"),exist_ok=True)

  for fn in bin_filenames:
    copy_bin(prefix_name, src_prefx_path, fn)

  # OpenBabel plugins

  openbabel_plugin_dir = glob.glob(os.path.join(src_prefx_path, "lib","openbabel", "*"))[0]
  openbabel_version = os.path.basename(openbabel_plugin_dir)

  os.makedirs(os.path.join(prefix_name, "lib", "openbabel", openbabel_version), exist_ok=True)

  # print(glob.glob(os.path.join(src_prefx_path, "lib","openbabel", "*", "*")))

  openbabel_formats = ["mdlformat.so", "pdbformat.so", "plugin_forcefields.so", "plugin_charges.so"]

  for fmt in openbabel_formats:
    fn = os.path.join(openbabel_plugin_dir, fmt)
    copy_lib(prefix_name, fn, "openbabel/"+openbabel_version+"/")

  # OpenMPI plugins

  os.makedirs(os.path.join(prefix_name, "lib/openmpi"), exist_ok=True)
  os.makedirs(os.path.join(prefix_name, "lib/pmix"), exist_ok=True)

  openmpi_plugins = glob.glob(os.path.join(src_prefx_path, "lib", "openmpi", "*"))
  pmix_plugins = glob.glob(os.path.join(src_prefx_path, "lib", "pmix", "*"))

  for fn in openmpi_plugins:
    copy_lib(prefix_name, fn, "openmpi/")

  for fn in pmix_plugins:
    copy_lib(prefix_name, fn, "pmix/")

  # Codesign everything
  subprocess.check_output("codesign -f -s - " + prefix_name + "/lib/*.dylib", stderr=subprocess.STDOUT, shell=True)
  subprocess.check_output("codesign -f -s - " + prefix_name + "/bin/*", stderr=subprocess.STDOUT, shell=True)
  subprocess.check_output("codesign -f -s - " + prefix_name + "/lib/openbabel/*/*.so", stderr=subprocess.STDOUT, shell=True)
  subprocess.check_output("codesign -f -s - " + prefix_name + "/lib/openmpi/*.so", stderr=subprocess.STDOUT, shell=True)
  subprocess.check_output("codesign -f -s - " + prefix_name + "/lib/pmix/*.so", stderr=subprocess.STDOUT, shell=True)

def replace_tree(src, dst):
  if (os.path.isdir(dst)):
    shutil.rmtree(dst)
  shutil.copytree(src, dst)

def build_shared(src_prefx_path):
  os.makedirs("Resources/share", exist_ok=True)

  if (os.path.exists("Resources/arm") and not os.path.exists("Resources/arm/share")):
    os.symlink("../share", "Resources/arm/share")
  if (os.path.exists("Resources/x86") and not os.path.exists("Resources/x86/share")):
    os.symlink("../share", "Resources/x86/share")

  openbabel_data_dir = glob.glob(os.path.join(src_prefx_path, "share", "openbabel", "*"))[0]
  openbabel_version = os.path.basename(openbabel_data_dir)
  replace_tree(openbabel_data_dir, os.path.join("Resources/share/openbabel", openbabel_version))
  replace_tree(os.path.join(src_prefx_path, "share/openmpi"), "Resources/share/openmpi")
  replace_tree(os.path.join(src_prefx_path, "share/pmix"), "Resources/share/pmix")
  replace_tree(os.path.join(src_prefx_path, "share/nwchem/libraries"), "Resources/share/nwchem/libraries")

parser = argparse.ArgumentParser()
parser.add_argument('--x86', help='homebrew prefix for x86 binaries', metavar="x86_prefix")
parser.add_argument('--arm', help='homebrew prefix for ARM binaries', metavar="arm_prefix")

parsed_args = parser.parse_args(sys.argv[1:])
if (not parsed_args.arm and not parsed_args.x86):
  print("Error: At least one prefix must be given")
  parser.print_help()
  sys.exit(1)

share_prefix = None

if (parsed_args.x86):
  if (not os.path.isdir(parsed_args.x86)):
    print("Error: \"%s\" does not exist" % parsed_args.x86)
    sys.exit(1)
  build_prefix(parsed_args.x86, "Resources/x86")
  share_prefix = parsed_args.x86

if (parsed_args.arm):
  if (not os.path.isdir(parsed_args.arm)):
    print("Error: \"%s\" does not exist" % parsed_args.arm)
    sys.exit(1)
  build_prefix(parsed_args.arm, "Resources/arm")
  share_prefix = parsed_args.arm

build_shared(share_prefix)


