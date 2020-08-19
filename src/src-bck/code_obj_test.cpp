#include <cstring>
#include <fstream>
#include <cstdlib>
#include <iostream>

#include <elf.h>
#include "libelf.h"

#include "kmt_test.h"
#include "amd_elf_image.hpp"
#include "amd_hsa_code.hpp"

using namespace amd;
using namespace amd::elf;
using namespace amd::hsa::code;

const string CodeBinFile = "/home/feifei/projects/kmt_test/out/isaPackedFp16.bin";
void * CodeBinBuff;
size_t CodeBinSize;
AmdHsaCode * CodeObject;

string trans_elf_type(uint64_t type)
{
	switch (type)
	{
	case ET_NONE:	return "NONE";
	case ET_REL:	return "REL";
	case ET_EXEC:	return "EXEC";
	case ET_DYN:	return "DYN";
	case ET_CORE:	return "CORE";
	case ET_NUM:	return "NUM";
	case ET_LOOS:	return "LOOS";
	case ET_HIOS:	return "HIOS";
	case ET_LOPROC:	return "LOPROC";
	case ET_HIPROC:	return "HIPROC";
	}
	return "ERROR";
}

bool load_code_bin_file()
{
	printf("\n=======================\n");
	printf("load code bin file:\n");

	ifstream fs_in(CodeBinFile.c_str(), ios::binary | ios::ate);

	CodeBinSize = string::size_type(fs_in.tellg());
	printf("\topen bin file \"%s\" size = %.3f(KB).\n", CodeBinFile.c_str(), CodeBinSize / 1024.0);

	char * ptr = (char*)HsaAllocCPU(CodeBinSize);
	assert(ptr != NULL);
	CodeBinBuff = ptr;

	fs_in.seekg(0, ios::beg);
	std::copy(istreambuf_iterator<char>(fs_in), istreambuf_iterator<char>(), ptr);

	printf("\tcopy bin file %.3f(KB) to 0x%016lX.\n", CodeBinSize / 1024.0, CodeBinBuff);
	printf("=======================\n");
}
void create_elf_image()
{
	printf("\n=======================\n");
	printf("create elf image:\n");

	amd::elf::Image * ElfImage;
	size_t ElfImgSize;

	ElfImgSize = ElfSize(CodeBinBuff);
	printf("\telf size = %.3f(KB).\n", ElfImgSize / 1024.0);

	ElfImage = NewElf64Image();
	ElfImage->initAsBuffer(CodeBinBuff, ElfImgSize);

	uint16_t elf_machine = ElfImage->Machine();
	uint16_t elf_type = ElfImage->Type();
	uint32_t elf_eflags = ElfImage->EFlags();
	printf("\tmachine = %d.\n", elf_machine);
	printf("\ttype = %s.\n", trans_elf_type(elf_type).c_str());
	printf("\tflags = 0x%04X.\n", elf_eflags);
	

	size_t segment_cnt = ElfImage->segmentCount();
	printf("\tsegment number = %d.\n", segment_cnt);
	for (uint32_t i = 0; i < segment_cnt; i++)
	{
		Segment * seg = ElfImage->segment(i);
		printf("[%02d]: ", i);
		printf("seg idx = %d, ", seg->getSegmentIndex());
		printf("type = %d, ", seg->type());
		printf("mem size = %ld, ", seg->memSize());
		printf("img size = %ld, ", seg->imageSize());
		printf("addr = 0x%016lX, ", seg->vaddr());
		printf("offset = 0x%016lX, ", seg->offset());
		printf("flag = 0x%016lX, ", seg->flags());
		printf("\n");
	}


	return;
	size_t section_cnt = ElfImage->sectionCount();
	printf("\tsection number = %d.\n", section_cnt);
	for (uint32_t i = 0; i < section_cnt; i++)
	{
		Section * sec = ElfImage->section(i);
		if (sec->Name() != "")
			printf("[%d]: %s.\n", i, sec->Name().c_str());
	}
}
void create_code_obj()
{
	printf("\n=======================\n");
	printf("create code object:\n");

	CodeObject = new AmdHsaCode();
	CodeObject->InitAsBuffer(CodeBinBuff, 0);

	// code objects info ====================================
	std::string codeIsa;
	CodeObject->GetNoteIsa(codeIsa);
	uint32_t majorVersion, minorVersion;
	CodeObject->GetNoteCodeObjectVersion(&majorVersion, &minorVersion);
	printf("\tisa = %s, obj version = %d.%d\n", codeIsa.c_str(), majorVersion, minorVersion);


	// segments info ====================================
	size_t seg_num = CodeObject->DataSegmentCount();
	printf("\tsegment number = %d.\n", seg_num);
	for (uint32_t i = 0; i < seg_num; i++)
	{
		amd::hsa::code::Segment * seg = CodeObject->DataSegment(i);
		printf("\t\t[%02d]: ", i);
		printf("idx = %d, ", seg->getSegmentIndex());
		printf("type = %d, ", seg->type());
		printf("mem size = %4ld, ", seg->memSize());
		printf("img size = %4ld, ", seg->imageSize());
		printf("addr = 0x%08X, ", seg->vaddr());
		printf("offset = 0x%08X, ", seg->offset());
		printf("flag = 0x%02X", seg->flags());
		printf("\n");
	}


	// sections info ====================================
	size_t sct_num = CodeObject->DataSectionCount();
	printf("\tsection number = %d.\n", sct_num);
	for (uint32_t i = 0; i < sct_num; i++)
	{
		amd::hsa::code::Section * sct = CodeObject->DataSection(i);
		printf("\t\t[%02d]: name = %s.\n", i, sct->Name().c_str());
	}

	// symbols info ====================================
	size_t sym_num = CodeObject->SymbolCount();
	printf("\tsymbol number = %d.\n", sym_num);
	for (uint32_t i = 0; i < sym_num; i++)
	{
		amd::hsa::code::Symbol * sym = CodeObject->GetSymbol(i);
		printf("\t\t-----------------------\n");
		printf("\t\tSymbol[%d]: %s.\n", i, sym->GetSymbolName().c_str());
		printf("\t\tDeclaration: %s,", sym->IsDeclaration() ? "YES" : "NO");
		printf(" Agent: %s,", sym->IsAgent() ? "YES" : "NO");
		printf(" Variable symbol: %s,", sym->IsVariableSymbol() ? "YES" : "NO");
		printf(" Kernel symbol: %s.\n", sym->IsKernelSymbol() ? "YES" : "NO");
		printf("\t\tsize = %u.\n", sym->Size());
	}
	printf("=======================\n");


	printf("\n**********************************\n");
	printf("* Print                          *\n");
	printf("**********************************\n");
	CodeObject->Print(std::cout);
	printf("\n**********************************\n");
	printf("* PrintNotes                     *\n");
	printf("**********************************\n");
	CodeObject->PrintNotes(std::cout);
	printf("\n**********************************\n");
	printf("* PrintSegments                  *\n");
	printf("**********************************\n");
	CodeObject->PrintSegments(std::cout);
	printf("\n**********************************\n");
	printf("* PrintSections                 *\n");
	printf("**********************************\n");
	CodeObject->PrintSections(std::cout);
	printf("\n**********************************\n");
	printf("* PrintSymbols                   *\n");
	printf("**********************************\n");
	CodeObject->PrintSymbols(std::cout);
	printf("\n**********************************\n");
	printf("* PrintMachineCode               *\n");
	printf("**********************************\n");
	CodeObject->PrintMachineCode(std::cout);
	printf("\n**********************************\n");
	printf("**********************************\n");
}

void CodeObjTest()
{
	KmtInitMem();

	printf("**********************************\n");
	printf("* code object test               *\n");
	printf("**********************************\n");

	load_code_bin_file();
	//create_elf_image();
	create_code_obj();

	printf("\n");
	KmtDeInitMem();
}
