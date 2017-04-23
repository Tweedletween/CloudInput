from distutils.core import setup
from distutils.core import Extension
import os
import subprocess
import sys

module = Extension("simplepinyin", ["simplepinyin.cpp"])
module.language = 'c++'

if os.system("pkg-config --exists libpinyin") != 0:
    print("Please install libpinyin.", file=sys.stderr)
    sys.exit(1)

libpinyin_args = subprocess.check_output(["pkg-config", "--cflags", "--libs", "libpinyin"], universal_newlines=True).strip().split(" ")
libpinyin_data = subprocess.check_output(["pkg-config", "--variable=pkgdatadir", "libpinyin"], universal_newlines=True).strip()+"/data"
#print(libpinyin_data)
module.include_dirs = [arg.lstrip("-I") for arg in libpinyin_args if arg.startswith("-I")]
module.library_dirs = [arg.lstrip("-L") for arg in libpinyin_args if arg.startswith("-L")]
module.libraries = [arg.lstrip("-l") for arg in libpinyin_args if arg.startswith("-l")]
module.define_macros = [("LIBPINYIN_DATA", "\""+libpinyin_data+"\"")]

setup(name="simplepinyin", version="1.0",
      ext_modules=[module])
