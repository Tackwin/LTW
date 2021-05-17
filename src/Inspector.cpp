#include "Inspector.hpp"

#include "imgui/imgui.h"

#include "xstd.hpp"

size_t _MARKER_ADDRESS_ = 0;

PSYMBOL_INFO get_marker_symbol() noexcept {
	thread_local char buffer[256];
	PSYMBOL_INFO s = (PSYMBOL_INFO)buffer;
	s->SizeOfStruct = sizeof(SYMBOL_INFO);
	s->MaxNameLen = 256 - s->SizeOfStruct;
	SymFromName(GetCurrentProcess(), "_MARKER_ADDRESS_", s);
	return s;
}

void* translate_ptr(void* sym_space_ptr) noexcept {
	size_t dt_marker_sym_space = (size_t)sym_space_ptr - _MARKER_ADDRESS_;
	return (void*)((size_t)&_MARKER_ADDRESS_ + dt_marker_sym_space);
}

void handle_debug() noexcept {
	thread_local bool dirty = true;
	thread_local char search_buffer[1024];

	thread_local std::map<std::string, Value> top_values;


	auto is_open = ImGui::Begin("Debug");
	defer { ImGui::End(); };
	if (!is_open) return;

	dirty |= ImGui::InputText("Search", search_buffer, 1024);

	if (dirty) {
		top_values.clear();

		auto handle = GetCurrentProcess();
		thread_local auto res = SymInitialize(handle, nullptr, false);
		thread_local size_t pdb_size = file::get_file_size("LTW.pdb");
		thread_local auto base_dll = SymLoadModule(
			handle, nullptr, "LTW.pdb", nullptr, base_of_dll, pdb_size
		);

		auto enum_f = [] (PSYMBOL_INFO sym_info, ULONG sym_size, void* user) -> BOOL {
			auto& values = *(std::map<std::string, Value>*)user;

			values[std::string(sym_info->Name)] = construct_variable((DWORD64)0, sym_info);
			return true;
		};

		SymEnumSymbols(handle, base_of_dll, search_buffer, enum_f, &top_values);

		_MARKER_ADDRESS_ = get_marker_symbol()->Address;
	}
	dirty = false;

	ImGui::Separator();

	auto handle_value = [](Value& value) {
		switch(value.type) {
			case Value::Int: {
				ImGui::InputInt("", (int*)translate_ptr(value.address));
			} break;
			case Value::Float: {
				ImGui::InputFloat("", (float*)translate_ptr(value.address));
			} break;
			case Value::Bool: {
				ImGui::Checkbox("", (bool*)translate_ptr(value.address));
			} break;
			case Value::UInt: {
				int x = *(std::uint32_t*)translate_ptr(value.address);
				ImGui::InputInt("", &x);
				*(std::uint32_t*)translate_ptr(value.address) = x;
			} break;
			case Value::Long: {
				int x = *(std::int64_t*)translate_ptr(value.address);
				ImGui::InputInt("", &x);
				*(std::int64_t*)translate_ptr(value.address) = x;
			} break;
			case Value::ULong: {
				int x = *(std::uint64_t*)translate_ptr(value.address);
				ImGui::InputInt("", &x);
				*(std::uint64_t*)translate_ptr(value.address) = x;
			} break;
			case Value::Struct: {
				ImGui::Text("{}");
			} break;
			default: {
				ImGui::Text("?");
			} break;
		}
	};

	std::function<void(const char*, const char*, Value&)> handle_variable;
	handle_variable = [&](const char* prefix, const char* name, Value& value) noexcept {
		ImGui::PushID(name);
		defer { ImGui::PopID(); };
	
		bool toggle_expanded = false;

		ImGui::TableNextColumn();
		ImGui::Text("%s%s", prefix, name);
		toggle_expanded |= ImGui::IsItemClicked();
		ImGui::TableNextColumn();
		handle_value(value);
		toggle_expanded |= ImGui::IsItemClicked();

		value.expanded ^= toggle_expanded;

		if (value.expanded && value.type == value.Struct) {
			if (!value.members_loaded) {
				auto handle = GetCurrentProcess();
				char buffer[512] = { 0 };
				PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
				symbol->MaxNameLen = 512 - sizeof(SYMBOL_INFO);
				SymFromIndex(handle, base_of_dll, (DWORD)value.index, symbol);

				DWORD n_children = 0;
				SymGetTypeInfo(
					handle, base_of_dll, symbol->TypeIndex, TI_GET_CHILDRENCOUNT, &n_children
				);

				size_t find_children_size =
					sizeof(TI_FINDCHILDREN_PARAMS) + n_children * sizeof(ULONG);
				auto* find_children = (TI_FINDCHILDREN_PARAMS*)_alloca(find_children_size);
				memset(find_children, 0, find_children_size);
				find_children->Count = n_children;

				SymGetTypeInfo(
					handle, base_of_dll, symbol->TypeIndex, TI_FINDCHILDREN, find_children
				);

				for (size_t i = 0; i < n_children; ++i) if (
					check_tag(find_children->ChildId[i], SymTag::Data) ||
					check_tag(find_children->ChildId[i], SymTag::BaseClass)
				) {
					char child_buffer[512] = { 0 };
					PSYMBOL_INFO child_symbol = (PSYMBOL_INFO)child_buffer;
					child_symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
					child_symbol->MaxNameLen = 512 - sizeof(SYMBOL_INFO);
					SymFromIndex(handle, base_of_dll, find_children->ChildId[i], child_symbol);

					value.members[std::string(child_symbol->Name)] =
						construct_variable((DWORD64)value.address, child_symbol);
				}

				value.members_loaded = true;
			}

			std::string pre = std::string(prefix) + "    ";
			for (auto& [child_name, child_value] : value.members) {
				handle_variable(pre.c_str(), child_name.c_str(), child_value);
			}
		}
	};

	ImGui::BeginTable("Inspector", 2, ImGuiTableFlags_RowBg);

	size_t max_item = 100;
	if (*search_buffer) for (auto& [name, value] : top_values) {
		handle_variable("", name.c_str(), value);
		ImGui::TableNextRow();
		if (--max_item == 0) break;
	}
	ImGui::EndTable();

};


Value construct_variable(void* address, PSYMBOL_INFO symbol) noexcept {
	thread_local auto handle = GetCurrentProcess();

	Value res;
	res.address = address;

	DWORD tag =  0;
	SymGetTypeInfo(handle, base_of_dll, symbol->TypeIndex, TI_GET_SYMTAG, &tag);

	res.index = symbol->Index;

	res.type = res.Unknown;
	switch ((SymTag)tag) {
		case SymTag::BaseType: {

			DWORD base_type = 0;
			SymGetTypeInfo(handle, base_of_dll, symbol->TypeIndex, TI_GET_BASETYPE, &base_type);

			switch ((BasicType)base_type) {
				case BasicType::Int   : res.type = res.Int;   break;
				case BasicType::Float : res.type = res.Float; break;
				case BasicType::Bool  : res.type = res.Bool;  break;
				case BasicType::UInt  : {
					res.type = res.UInt;  break;
				}
				case BasicType::Long  : res.type = res.Long;  break;
				case BasicType::ULong : res.type = res.ULong; break;
				// >TODO(Tackwin): the rest
				default: break;
			}

		} break;
		case SymTag::UDT: {
			res.type = res.Struct;
		} break;
		case SymTag::BaseClass: {
			res.type = res.Struct;
		}
		default: break;
		// >TODO(Tackwin): The rest (pointer, references and base classes)
	}


	return res;
}

Value construct_variable(DWORD64 parent, PSYMBOL_INFO symbol) noexcept {
	thread_local auto handle = GetCurrentProcess();

	void* address = nullptr;
	if (parent) {
		DWORD offset = 0;
		SymGetTypeInfo(handle, base_of_dll, symbol->Index, TI_GET_OFFSET, &offset);

		address = (void*)(parent + offset);
	} else {
		Value res;
		address = (void*)symbol->Address;
	}

	return construct_variable(address, symbol);
}

bool check_tag(ULONG Index, SymTag Tag) noexcept {
	DWORD s = (DWORD)(SymTag::Null);

	if(!SymGetTypeInfo(GetCurrentProcess(), base_of_dll, Index, TI_GET_SYMTAG, &s)) {
		DWORD ErrCode = GetLastError();
		return false;
	}

	return (SymTag)s == Tag;
}