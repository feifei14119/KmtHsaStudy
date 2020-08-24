import subprocess
import sys, os, re
import shutil

Target = "example.out"
Kernel = "asm-kernel.co"

def execCmd(cmd):		
	r = os.popen(cmd)  
	text = r.read() 
	print(text)
	r.close()  
	return text 
	
def BuildKernel():	
	if os.path.exists("./" + Kernel):
		os.remove("./" + Kernel)		
	cmd = '/opt/rocm/bin/hcc -x assembler -target amdgcn-amd-amdhsa -mcpu=gfx906 -mno-code-object-v3 asm-kernel.s -o ' + Kernel
	print(cmd)
	text = execCmd(cmd)
	print(text)
def BuildTarget():	
	if os.path.exists("./" + Target):
		os.remove("./" + Target)		
	cmd = 'hipcc example.cpp dispatch.cpp -I/opt/rocm/include/ -L/opt/rocm/lib -lhsa-runtime64 -O0 -w -std=c++11 -o ' + Target
	print(cmd)
	text = execCmd(cmd)
	print(text)

def RunBuild():	
	BuildKernel()
	BuildTarget()

if __name__ == '__main__':
	RunBuild()
	exit()
	
	
