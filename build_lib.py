import subprocess
import sys
import os
import shutil

GCC_PATH = r"D:\eclipse_neon_2016q3_x86\GNU Tools ARM Embedded\2016q3\bin;"
ARMCC_PATH = r"C:\Keil_v5\ARM\ARMCC\bin;"
AWTK_ROOT = './output/awtk'



AWTK_GCC_LIB_NAME = 'libawtk.a'
AWTK_ARMCC_LIB_NAME = 'awtk.lib'
OUTPUT = './output/lib'

def get_source_file(path,list):
  files = os.listdir(path)
  for file in files:
    f = os.path.join(path,file)
    if os.path.isfile(f):
      if not (file.endswith('.h')or file.endswith('.inc')):
		list.append(f)
    if os.path.isdir(f):
      get_source_file(f,list) 

def main():
    if os.path.exists(OUTPUT):
        shutil.rmtree(OUTPUT)

    os.mkdir(OUTPUT)    

    #arm-none-eabi-gcc
    os.environ['Path'] =GCC_PATH + os.environ['Path']
    scons_gcc_cmd = []
    scons_gcc_cmd.append('scons.bat')
    scons_gcc_cmd.append('COMPILER_TOOLS=gcc')
    scons_gcc_cmd.append('AWTK_LIB_NAME=libawtk.a')
    
    p_gcc = subprocess.Popen(scons_gcc_cmd)
    p_gcc.wait()

    if p_gcc.returncode == 0:
        shutil.move(AWTK_GCC_LIB_NAME,OUTPUT)
        scons_gcc_cmd.append('-c')
        p_clean = subprocess.Popen(scons_gcc_cmd)
        p_clean.wait()
    else:
        sys.exit(1)

    #ARMCC
    os.environ['Path'] = ARMCC_PATH + os.environ['Path']
    scons_armcc_cmd = []
    scons_armcc_cmd.append('scons.bat')
    scons_armcc_cmd.append('COMPILER_TOOLS=armcc')
    scons_armcc_cmd.append('AWTK_LIB_NAME=awtk.lib')
    
    p_armcc = subprocess.Popen(scons_armcc_cmd)
    p_armcc.wait()

    if p_armcc.returncode == 0:
        shutil.move(AWTK_ARMCC_LIB_NAME,OUTPUT)
        scons_armcc_cmd.append('-c')
        p_clean = subprocess.Popen(scons_armcc_cmd)
        p_clean.wait()
    else:
        sys.exit(1)

    remove_source_file()

def remove_source_file():
    all_file =[]
    get_source_file(AWTK_ROOT,all_file)

    for f in all_file:
        print 'remove file' + f
        os.remove(f)


if __name__ == "__main__":
    main();