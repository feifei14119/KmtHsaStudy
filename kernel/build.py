import subprocess
import sys, os, re
import shutil

Kernel = "VectorAdd.co"

def execCmd(cmd):		
	r = os.popen(cmd)  
	text = r.read() 
	print(text)
	r.close()  
	return text 
	
def BuildKernel():	
	if os.path.exists("./" + Kernel):
		os.remove("./" + Kernel)		
	cmd = '/opt/rocm/bin/hcc -x assembler -target amdgcn-amd-amdhsa -mcpu=gfx906 -mno-code-object-v3 VectorAdd.s -o ' + Kernel
	print(cmd)
	text = execCmd(cmd)
	print(text)

def RunBuild():	
	BuildKernel()

if __name__ == '__main__':
	RunBuild()
	exit()
	
	
