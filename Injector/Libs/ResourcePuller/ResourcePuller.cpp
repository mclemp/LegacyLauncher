#include "ResourcePuller.h"
#include "windows.h"

ResourcePuller::ResourceData ResourcePuller::LoadResourceData(uintptr_t module, int id) {
    HRSRC res = FindResource((HMODULE)module, MAKEINTRESOURCE(id), RT_RCDATA);
    HGLOBAL data = LoadResource((HMODULE)module, res);

    return { (unsigned char*)LockResource(data), SizeofResource((HMODULE)module, res) };
}