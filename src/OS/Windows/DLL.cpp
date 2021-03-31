#include "OS/DLL.hpp"

#include <windows.h>

DLL::DLL(DLL&& other) noexcept {
	*this = std::move(other);
}
DLL& DLL::operator=(DLL&& other) noexcept {
	this->~DLL();
	memcpy(this, &other, sizeof(other));
	memset(&other, 0, sizeof(other));
	return *this;
}
DLL::~DLL() noexcept {
	if (ptr) FreeLibrary((HINSTANCE)ptr);
	ptr = nullptr;
}


std::optional<DLL> load_dll(std::filesystem::path path) noexcept {
	DLL dll;

	auto temp = path;
	temp.replace_filename(path.filename().string() + "_game");

	CopyFile(path.native().c_str(), temp.native().c_str(), FALSE);
	dll.ptr = LoadLibrary(temp.native().c_str());
	return dll;
}

void* DLL::get_symbol_(std::string_view name) noexcept {
	return (void*)GetProcAddress((HINSTANCE)ptr, name.data());
}