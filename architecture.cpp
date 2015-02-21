#include "binaryninjaapi.h"

using namespace BinaryNinja;
using namespace std;


InstructionInfo::InstructionInfo()
{
	length = 0;
	branchCount = 0;
}


void InstructionInfo::AddBranch(BNBranchType type, uint64_t target)
{
	if (branchCount >= BN_MAX_INSTRUCTION_BRANCHES)
		return;
	branchType[branchCount] = type;
	branchTarget[branchCount++] = target;
}


InstructionTextToken::InstructionTextToken(): type(TextToken), value(0)
{
}


InstructionTextToken::InstructionTextToken(BNInstructionTextTokenType t, const std::string& txt, uint64_t val) :
	type(t), text(txt), value(val)
{
}


Architecture::Architecture(BNArchitecture* arch): m_arch(arch)
{
}


Architecture::Architecture(const string& name): m_nameForRegister(name)
{
}


bool Architecture::GetInstructionInfoCallback(void* ctxt, BNBinaryView* data, uint64_t addr, BNInstructionInfo* result)
{
	Architecture* arch = (Architecture*)ctxt;
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));

	InstructionInfo info;
	bool ok = arch->GetInstructionInfo(view, addr, info);
	*result = info;
	return ok;
}


bool Architecture::GetInstructionTextCallback(void* ctxt, BNBinaryView* data, uint64_t addr,
                                              BNInstructionTextToken** result, size_t* count)
{
	Architecture* arch = (Architecture*)ctxt;
	Ref<BinaryView> view = new BinaryView(BNNewViewReference(data));

	vector<InstructionTextToken> tokens;
	bool ok = arch->GetInstructionText(view, addr, tokens);
	if (!ok)
	{
		*result = nullptr;
		*count = 0;
		return false;
	}

	*count = tokens.size();
	*result = new BNInstructionTextToken[tokens.size()];
	for (size_t i = 0; i < tokens.size(); i++)
	{
		(*result)[i].type = tokens[i].type;
		(*result)[i].text = BNAllocString(tokens[i].text.c_str());
		(*result)[i].value = tokens[i].value;
	}
	return true;
}


void Architecture::FreeInstructionTextCallback(BNInstructionTextToken* tokens, size_t count)
{
	for (size_t i = 0; i < count; i++)
		BNFreeString(tokens[i].text);
	delete[] tokens;
}


void Architecture::Register(Architecture* arch)
{
	BNCustomArchitecture callbacks;
	callbacks.context = arch;
	callbacks.getInstructionInfo = GetInstructionInfoCallback;
	callbacks.getInstructionText = GetInstructionTextCallback;
	callbacks.freeInstructionText = FreeInstructionTextCallback;
	arch->m_arch = BNRegisterArchitecture(arch->m_nameForRegister.c_str(), &callbacks);
}


Ref<Architecture> Architecture::GetByName(const string& name)
{
	BNArchitecture* arch = BNGetArchitectureByName(name.c_str());
	if (!arch)
		return nullptr;
	return new CoreArchitecture(arch);
}


vector<Ref<Architecture>> Architecture::GetList()
{
	BNArchitecture** archs;
	size_t count;
	archs = BNGetArchitectureList(&count);

	vector<Ref<Architecture>> result;
	for (size_t i = 0; i < count; i++)
		result.push_back(new CoreArchitecture(BNNewArchitectureReference(archs[i])));

	BNFreeArchitectureList(archs, count);
	return result;
}


string Architecture::GetName() const
{
	char* name = BNGetArchitectureName(m_arch);
	string result = name;
	BNFreeString(name);
	return result;
}


CoreArchitecture::CoreArchitecture(BNArchitecture* arch): Architecture(arch)
{
}


bool CoreArchitecture::GetInstructionInfo(BinaryView* view, uint64_t addr, InstructionInfo& result)
{
	return BNGetInstructionInfo(m_arch, view->GetViewObject(), addr, &result);
}


bool CoreArchitecture::GetInstructionText(BinaryView* view, uint64_t addr, std::vector<InstructionTextToken>& result)
{
	BNInstructionTextToken* tokens = nullptr;
	size_t count = 0;
	if (!BNGetInstructionText(m_arch, view->GetViewObject(), addr, &tokens, &count))
		return false;

	for (size_t i = 0; i < count; i++)
		result.push_back(InstructionTextToken(tokens[i].type, tokens[i].text, tokens[i].value));

	BNFreeInstructionText(tokens, count);
	return true;
}
