#include "kmthsa.h"
#include "amd_hsa_code.hpp"

enum hsa_ext_symbol_info_t 
{
	HSA_EXT_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT_SIZE = 100,
	HSA_EXT_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT_ALIGN = 101,
};
typedef uint32_t hsa_symbol_info32_t;
typedef hsa_executable_symbol_t hsa_symbol_t;

class Symbol
{
public:
	static hsa_symbol_t Handle(Symbol *symbol)
	{
		hsa_symbol_t symbol_handle = { reinterpret_cast<uint64_t>(symbol) };
		return symbol_handle;
	}

	static Symbol* Object(hsa_symbol_t symbol_handle)
	{
		Symbol *symbol = reinterpret_cast<Symbol*>(symbol_handle.handle);
		return symbol;
	}

	virtual ~Symbol() {}

	virtual bool GetInfo(hsa_symbol_info32_t symbol_info, void *value) = 0;

	virtual hsa_agent_t GetAgent() = 0;

protected:
	Symbol() {}

private:
	Symbol(const Symbol &s);
	Symbol& operator=(const Symbol &s);
};
class SymbolImpl : public Symbol
{
public:
	virtual ~SymbolImpl() {}

	bool IsKernel() const {		return HSA_SYMBOL_KIND_KERNEL == kind;	}
	bool IsVariable() const {		return HSA_SYMBOL_KIND_VARIABLE == kind;	}

	bool is_loaded;
	hsa_symbol_kind_t kind;
	std::string module_name;
	std::string symbol_name;
	hsa_symbol_linkage_t linkage;
	bool is_definition;
	uint64_t address;
	hsa_agent_t agent;

	hsa_agent_t GetAgent() override {		return agent;	}

protected:
	SymbolImpl(const bool &_is_loaded,
		const hsa_symbol_kind_t &_kind,
		const std::string &_module_name,
		const std::string &_symbol_name,
		const hsa_symbol_linkage_t &_linkage,
		const bool &_is_definition,
		const uint64_t &_address = 0)
		: is_loaded(_is_loaded)
		, kind(_kind)
		, module_name(_module_name)
		, symbol_name(_symbol_name)
		, linkage(_linkage)
		, is_definition(_is_definition)
		, address(_address) {}

	virtual bool GetInfo(hsa_symbol_info32_t symbol_info, void* value) override;

private:
	SymbolImpl(const SymbolImpl &s);
	SymbolImpl& operator=(const SymbolImpl &s);
};
class KernelSymbol final : public SymbolImpl 
{
public:
	KernelSymbol(const bool &_is_loaded,
		const std::string &_module_name,
		const std::string &_symbol_name,
		const hsa_symbol_linkage_t &_linkage,
		const bool &_is_definition,
		const uint32_t &_kernarg_segment_size,
		const uint32_t &_kernarg_segment_alignment,
		const uint32_t &_group_segment_size,
		const uint32_t &_private_segment_size,
		const bool &_is_dynamic_callstack,
		const uint32_t &_size,
		const uint32_t &_alignment,
		const uint64_t &_address = 0)
		: SymbolImpl(_is_loaded,
			HSA_SYMBOL_KIND_KERNEL,
			_module_name,
			_symbol_name,
			_linkage,
			_is_definition,
			_address)
		, full_name(_module_name.empty() ? _symbol_name : _module_name + "::" + _symbol_name)
		, kernarg_segment_size(_kernarg_segment_size)
		, kernarg_segment_alignment(_kernarg_segment_alignment)
		, group_segment_size(_group_segment_size)
		, private_segment_size(_private_segment_size)
		, is_dynamic_callstack(_is_dynamic_callstack)
		, size(_size)
		, alignment(_alignment) {}

	~KernelSymbol() {}

	bool GetInfo(hsa_symbol_info32_t symbol_info, void *value);

	std::string full_name;
	uint32_t kernarg_segment_size;
	uint32_t kernarg_segment_alignment;
	uint32_t group_segment_size;
	uint32_t private_segment_size;
	bool is_dynamic_callstack;
	uint32_t size;
	uint32_t alignment;
	amd_runtime_loader_debug_info_t debug_info;

private:
	KernelSymbol(const KernelSymbol &ks);
	KernelSymbol& operator=(const KernelSymbol &ks);
};

bool SymbolImpl::GetInfo(hsa_symbol_info32_t symbol_info, void *value) 
{
	switch (symbol_info)
	{
	case HSA_CODE_SYMBOL_INFO_TYPE:
		*((hsa_symbol_kind_t*)value) = kind;
		break;
	case HSA_CODE_SYMBOL_INFO_NAME_LENGTH:
		*((uint32_t*)value) = symbol_name.size();
		break;
	case HSA_CODE_SYMBOL_INFO_NAME:
		memset(value, 0x0, symbol_name.size());
		memcpy(value, symbol_name.c_str(), symbol_name.size());
		break;
	case HSA_CODE_SYMBOL_INFO_MODULE_NAME_LENGTH:
		*((uint32_t*)value) = module_name.size();
		break;
	case HSA_CODE_SYMBOL_INFO_MODULE_NAME:
		memset(value, 0x0, module_name.size());
		memcpy(value, module_name.c_str(), module_name.size());
		break;
	case HSA_CODE_SYMBOL_INFO_LINKAGE:
		*((hsa_symbol_linkage_t*)value) = linkage;
		break;
	case HSA_CODE_SYMBOL_INFO_IS_DEFINITION:
		*((bool*)value) = is_definition;
		break;
	case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_CALL_CONVENTION:
		*((uint32_t*)value) = 0;
		break;
	case HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT:
	case HSA_EXECUTABLE_SYMBOL_INFO_VARIABLE_ADDRESS:
		if (!is_loaded) 	return false;
		*((uint64_t*)value) = address;
		break;
	case HSA_EXECUTABLE_SYMBOL_INFO_AGENT:
		if (!is_loaded)		return false;
		*((hsa_agent_t*)value) = agent;
		break;
	default:		return false;
	}

	return true;
}
bool KernelSymbol::GetInfo(hsa_symbol_info32_t symbol_info, void *value)
{
	switch (symbol_info) 
	{
	case HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_SIZE:
		*((uint32_t*)value) = kernarg_segment_size;
		break;
	case HSA_CODE_SYMBOL_INFO_KERNEL_KERNARG_SEGMENT_ALIGNMENT:
		*((uint32_t*)value) = kernarg_segment_alignment;
		break;
	case HSA_CODE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE:
		*((uint32_t*)value) = group_segment_size;
		break;
	case HSA_CODE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE:
		*((uint32_t*)value) = private_segment_size;
		break;
	case HSA_CODE_SYMBOL_INFO_KERNEL_DYNAMIC_CALLSTACK:
		*((bool*)value) = is_dynamic_callstack;
		break;
	case HSA_EXT_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT_SIZE:
		*((uint32_t*)value) = size;
		break;
	case HSA_EXT_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT_ALIGN:
		*((uint32_t*)value) = alignment;
		break;
	default:
		return SymbolImpl::GetInfo(symbol_info, value);
	}

	return true;
}

// ==================================================================
/*
 * <amd_elf_iamge>  amd::elf
 * <amd_hsa_code>   amd::hsa::code
 * <amd_hsa_loader> amd::hsa::loader
 */
typedef amd::hsa::code::AmdHsaCode AmdHsaCode;

void * CodeBinBuff;
AmdHsaCode * CodeObject;

void * CtxHstPtr;
void * CtxDevPtr;
uint32_t CtxMemSize;

uint32_t KernelArgsSize;
void * KernelSymbolAddress;
void * KernelArgsAddress;

// ==================================================================

bool read_code_bin_file(string fileName)
{
	printf("read_code_bin_file\n");

	size_t file_size;
	ifstream fs_in(fileName.c_str(), ios::binary | ios::ate);

	file_size = string::size_type(fs_in.tellg());
	//printf("\topen bin file \"%s\" size = %.3f(KB).\n", fileName.c_str(), file_size / 1024.0);

	char * ptr = (char*)HsaAllocCPU(file_size);
	assert(ptr != NULL);
	CodeBinBuff = ptr;

	fs_in.seekg(0, ios::beg);
	std::copy(istreambuf_iterator<char>(fs_in), istreambuf_iterator<char>(), ptr);

	//printf("\tcopy bin file %.3f(KB) to 0x%016lX.\n", file_size / 1024.0, CodeBinBuff);
}
void create_code_object()
{
	printf("create_code_object\n");

	CodeObject = new AmdHsaCode();
	CodeObject->InitAsBuffer(CodeBinBuff, 0);

	int seg_cnt = CodeObject->DataSegmentCount();
	uint64_t size = CodeObject->DataSegment(seg_cnt - 1)->vaddr() + CodeObject->DataSegment(seg_cnt - 1)->memSize();
	printf("seg_cnt = %d\n", seg_cnt);
	printf("vaddr = 0x%016lX\n", CodeObject->DataSegment(seg_cnt - 1)->vaddr());
	printf("memSize = %ld\n", CodeObject->DataSegment(seg_cnt - 1)->memSize());
	printf("context alloc segment, size = %ld\n", size);
	//CodeObject->Print(std::cout);
}
void context_alloc_memory()
{
	printf("context_alloc_memory\n");

	int seg_cnt = CodeObject->DataSegmentCount();
	amd::hsa::code::Segment * last_seg = CodeObject->DataSegment(seg_cnt - 1);
	CtxMemSize = last_seg->vaddr() + last_seg->memSize();

	CtxDevPtr = HsaAllocGPU(CtxMemSize);
	CtxHstPtr = HsaAllocCPU(CtxMemSize);

	printf("\tdev_addr = 0x%016lX, hst_addr = 0x%016lX, size = %ld\n", CtxDevPtr, CtxHstPtr, CtxMemSize);
}
void load_code_segment()
{
	printf("load_code_segment\n");

	int seg_cnt = CodeObject->DataSegmentCount();
	
	amd::hsa::code::Segment * seg;
	char * seg_addr0 = (char*)CodeObject->DataSegment(0)->vaddr();
	for (size_t i = 0; i < seg_cnt; ++i)
	{
		seg = CodeObject->DataSegment(i);
		char * seg_addr = (char*)seg->vaddr();
		char * src_addr = (char*)seg->data();
		uint64_t offset = seg_addr - seg_addr0;
		uint64_t img_size = seg->imageSize();
		printf("seg[%d], data = 0x%016lX, addr = 0x%016lX, offset = 0x%08X, size = 0x%08X\n", i, src_addr, seg_addr, offset, img_size);
		memcpy((char*)CtxHstPtr + offset, src_addr, img_size);
		printf("copy, dst = 0x%016lX, src = 0x%016lX\n", (char*)CtxHstPtr + offset, src_addr);
	}
}
void load_kenrel_symbol()
{
	printf("load_kenrel_symbol\n");

	amd::hsa::code::Symbol* sym;
	amd::hsa::code::Section* sec;
	amd::hsa::code::Segment * seg;
	amd_kernel_code_t akc;

	for (size_t i = 0; i < CodeObject->SymbolCount(); ++i)
	{
		sym = CodeObject->GetSymbol(i);
		sec = sym->GetSection();

		if (!sym->IsDefinition())	continue;
		if (!sym->IsKernelSymbol())	continue;

		printf("\tkernel symbol[%d/%d]: %s\n\n", i, CodeObject->SymbolCount(), sym->Name().c_str());
		sec = sym->GetSection();
		sec->getData(sym->SectionOffset(), &akc, sizeof(akc));

		uint32_t kernarg_segment_size = uint32_t(akc.kernarg_segment_byte_size);
		uint32_t kernarg_segment_alignment = uint32_t(1 << akc.kernarg_segment_alignment);
		uint32_t group_segment_size = uint32_t(akc.workgroup_group_segment_byte_size);
		uint32_t private_segment_size = uint32_t(akc.workitem_private_segment_byte_size);
		bool is_dynamic_callstack = AMD_HSA_BITS_GET(akc.kernel_code_properties, AMD_KERNEL_CODE_PROPERTIES_IS_DYNAMIC_CALLSTACK) ? true : false;
		uint64_t sym_size = sym->Size();
		uint64_t sec_size = sec->size();
		uint64_t sec_offset = sym->SectionOffset();
		uint64_t size = sec_size - sec_offset;
		printf("\tsym_size = %ld, sec_size = %ld, offset = %ld\n", sym_size, sec_size, sec_offset);
		printf("\tkernarg_segment_size = %d\n", kernarg_segment_size);
		printf("\tkernarg_segment_alignment = %d\n", kernarg_segment_alignment);
		printf("\tgroup_segment_size = %d\n", group_segment_size);
		printf("\tprivate_segment_size = %d\n", private_segment_size);
		printf("\tis_dynamic_callstack = %d\n", is_dynamic_callstack);
		printf("\n");

		uint64_t sym_addr = sym->VAddr();
		printf("\tsym_addr = 0x%016lX\n", sym_addr);
		for (int j = 0; j < CodeObject->DataSegmentCount(); j++)
		{
			seg = CodeObject->DataSegment(j);
			uint64_t seg_addr = seg->vaddr();
			printf("\tseg[%d]: sym_addr = 0x%016lX\n", j, seg_addr);
			if(sym_addr <= seg_addr)
				break;
		}

		uint64_t address = (uint64_t)CtxDevPtr + (uint64_t)seg->vaddr();
		printf("\taddress = 0x%016lX\n", address);

		/*KernelSymbol *kernel_symbol = new KernelSymbol(true,
			sym->GetModuleName(),
			sym->GetSymbolName(),
			sym->Linkage(),
			true, // sym->IsDefinition()
			kernarg_segment_size,
			kernarg_segment_alignment,
			group_segment_size,
			private_segment_size,
			is_dynamic_callstack,
			size,
			256,
			address);*/

		printf("\tsym->GetModuleName = %s\n", sym->GetModuleName().c_str());
		printf("\tsym->GetSymbolName = %s\n", sym->GetSymbolName().c_str());
		printf("\tsym->Linkage = %d\n", sym->Linkage());

		KernelSymbolAddress = (void*)address;
	}
}
void freeze_executable()
{
	printf("freeze_executable\n");
	HsaSdmaCopy(CtxDevPtr, CtxHstPtr, CtxMemSize);
	printf("DmaCopy: dst = 0x%016lX, src = 0x%016lX, size = %d\n", (uint64_t)CtxDevPtr, (uint64_t)CtxHstPtr, CtxMemSize);
}

// ==================================================================

void HsaLoadKernel(string fileName)
{
	printf("HsaLoadCode\n");

	read_code_bin_file(fileName);
	create_code_object();
	context_alloc_memory();
	load_code_segment();
	load_kenrel_symbol();
	freeze_executable();

	printf("\n");
}
void HsaSetKernelArgs(void * kernArgBuff)
{
	printf("HsaSetKernelArgs\n");
	KernelArgsSize = 1024;
	KernelArgsAddress = HsaAllocCPU(KernelArgsSize);
	memcpy(KernelArgsAddress, kernArgBuff, KernelArgsSize);
}

