#pragma once
#include "Windows.h"
#include <dbghelp.h>
#include <map>
#include "OS/file.hpp"
#include "Psapi.h"

enum class SymTag {
    Null,
    Exe,
    Compiland,
    CompilandDetails,
    CompilandEnv,
    Function,
    Block,
    Data,
    Annotation,
    Label,
    PublicSymbol,
    UDT,
    Enum,
    FunctionType,
    PointerType,
    ArrayType,
    BaseType,
    Typedef,
    BaseClass,
    Friend,
    FunctionArgType,
    FuncDebugStart,
    FuncDebugEnd,
    UsingNamespace,
    VTableShape,
    VTable,
    Custom,
    Thunk,
    CustomType,
    ManagedType,
    Dimension,
    CallSite,
    InlineSite,
    BaseInterface,
    VectorType,
    MatrixType,
    HLSLType,
    Caller,
    Callee,
    Export,
    HeapAllocationSite,
    CoffGroup,
    Max
};

enum class BasicType
{
    NoType = 0,
    Void = 1,
    Char = 2,
    WChar = 3,
    Int = 6,
    UInt = 7,
    Float = 8,
    BCD = 9,
    Bool = 10,
    Long = 13,
    ULong = 14,
    Currency = 25,
    Date = 26,
    Variant = 27,
    Complex = 28,
    Bit = 29,
    BSTR = 30,
    Hresult = 31
};
struct Value {
	enum Kind {
		Unknown = 0,
		Int,
		UInt,
		Float,
		Bool,
		Long,
		ULong,
		Struct,
		Count
	};
	Kind type = Kind::Unknown;
	bool is_pointer = false;

	size_t index = 0;
	void* address = nullptr;

	bool expanded = false;

	bool members_loaded = false;
	std::map<std::string, Value> members;
};
extern size_t _MARKER_ADDRESS_;
constexpr size_t base_of_dll = 1ULL << 63ULL;

PSYMBOL_INFO get_marker_symbol() noexcept;
void* translate_ptr(void* sym_space_ptr) noexcept;
void handle_debug() noexcept;
Value construct_variable(void* address, PSYMBOL_INFO symbol) noexcept;
Value construct_variable(DWORD64 parent, PSYMBOL_INFO symbol) noexcept;
bool check_tag(ULONG Index, SymTag Tag) noexcept;