#pragma once
#include <cstdint>

class ResourcePuller {
public:
	struct ResourceData {
		unsigned char* data; size_t size;

		ResourceData(unsigned char* _data, size_t _size) : data(_data), size(_size) {}
	};

	//hmodule is 0x8 and so it uintptr, use it here so we dont need windows.h in the header file
	static ResourceData LoadResourceData(uintptr_t module, int id);
};

