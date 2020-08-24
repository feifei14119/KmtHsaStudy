
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

typedef amd::hsa::code::AmdHsaCode AmdHsaCode;

void * CodeBinBuff;
AmdHsaCode * CodeObject;
KernelSymbol * KernelSymbolObj;

// ==================================================================

bool read_code_bin_file(string fileName)
{
	printf("\n=======================\n");
	printf("load code bin file:\n");

	size_t file_size;
	ifstream fs_in(fileName.c_str(), ios::binary | ios::ate);

	file_size = string::size_type(fs_in.tellg());
	printf("\topen bin file \"%s\" size = %.3f(KB).\n", fileName.c_str(), file_size / 1024.0);

	char * ptr = (char*)HsaAllocCPU(file_size);
	assert(ptr != NULL);
	CodeBinBuff = ptr;

	fs_in.seekg(0, ios::beg);
	std::copy(istreambuf_iterator<char>(fs_in), istreambuf_iterator<char>(), ptr);

	printf("\tcopy bin file %.3f(KB) to 0x%016lX.\n", file_size / 1024.0, CodeBinBuff);
	printf("=======================\n");
}
void create_code_object()
{
	printf("\n=======================\n");
	printf("create code object:\n");

	CodeObject = new AmdHsaCode();
	CodeObject->InitAsBuffer(CodeBinBuff, 0);

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
	printf("=======================\n");
}
void load_kenrel_symbol()
{
	printf("\n=======================\n");
	printf("load kernel symbol:\n");

	amd::hsa::code::Symbol* sym;
	amd::hsa::code::Section* sec;
	amd::hsa::code::Segment * seg;
	amd_kernel_code_t akc;

	for (size_t i = 0; i < CodeObject->SymbolCount(); ++i)
	{
		amd::hsa::code::Symbol* sym = CodeObject->GetSymbol(i);
		if (sym->IsDefinition() && sym->IsKernelSymbol())
		{
			printf("Kernel Symbol = %d/%d\n", i, CodeObject->SymbolCount());
			break;
		}
	}

	sec = sym->GetSection();
	sec->getData(sym->SectionOffset(), &akc, sizeof(akc));

	uint32_t kernarg_segment_size = uint32_t(akc.kernarg_segment_byte_size);
	uint32_t kernarg_segment_alignment = uint32_t(1 << akc.kernarg_segment_alignment);
	uint32_t group_segment_size = uint32_t(akc.workgroup_group_segment_byte_size);
	uint32_t private_segment_size = uint32_t(akc.workitem_private_segment_byte_size);
	bool is_dynamic_callstack = AMD_HSA_BITS_GET(akc.kernel_code_properties, AMD_KERNEL_CODE_PROPERTIES_IS_DYNAMIC_CALLSTACK) ? true : false;

	uint64_t size = sym->Size();
	uint64_t address = 0; // sym->sec->seg.addr
	for (size_t j = 0; j < CodeObject->DataSegmentCount(); j++)
	{
		seg = CodeObject->DataSegment(j);

		uint64_t sec_addr = sec->addr();
		uint64_t seg_vaddr = seg->vaddr();

		if (seg_vaddr <= sec_addr)
		{
			address = seg_vaddr;
			break;
		}
	}

	//KernelSymbolObj = new KernelSymbol(sym, akc);



			/*
			KernelSymbol *kernel_symbol = new KernelSymbol(true,
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
				address);
			kernel_symbol->debug_info.elf_raw = code->ElfData();
			kernel_symbol->debug_info.elf_size = code->ElfSize();
			kernel_symbol->debug_info.kernel_name = kernel_symbol->full_name.c_str();
			kernel_symbol->debug_info.owning_segment = (void*)SymbolSegment(agent, sym)->Address(sym->GetSection()->addr());
			symbol = kernel_symbol;

			// \todo kzhuravl 10/15/15 This is a debugger backdoor: needs to be
			// removed.
			uint64_t target_address = sym->GetSection()->addr() + sym->SectionOffset() + ((size_t)(&((amd_kernel_code_t*)0)->runtime_loader_kernel_symbol));
			uint64_t source_value = (uint64_t)(uintptr_t)&kernel_symbol->debug_info;
			SymbolSegment(agent, sym)->Copy(target_address, &source_value, sizeof(source_value));*/
		//}
	//}
	printf("=======================\n");
}

// ==================================================================

void HsaLoadCode(string fileName)
{
	read_code_bin_file(fileName);
	create_code_object();
	load_kenrel_symbol();
}

