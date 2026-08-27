#include "memory_tracker.hpp"

#if defined(ENGINE_ENABLE_MEMORY_PROFILER)

#include <new>

void* operator new(std::size_t size)
{
	if (void* pointer = Ludus::Memory::Allocate(size, alignof(std::max_align_t)))
		return pointer;
	throw std::bad_alloc();
}

void* operator new[](std::size_t size)
{
	return ::operator new(size);
}

void operator delete(void* pointer) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete[](void* pointer) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete(void* pointer, std::size_t) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete[](void* pointer, std::size_t) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
	if (void* pointer = Ludus::Memory::Allocate(size, static_cast<std::size_t>(alignment)))
		return pointer;
	throw std::bad_alloc();
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
	return ::operator new(size, alignment);
}

void operator delete(void* pointer, std::align_val_t) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete[](void* pointer, std::align_val_t) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
	return Ludus::Memory::Allocate(size, alignof(std::max_align_t));
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
	return Ludus::Memory::Allocate(size, alignof(std::max_align_t));
}

void operator delete(void* pointer, const std::nothrow_t&) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete[](void* pointer, const std::nothrow_t&) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void* operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	return Ludus::Memory::Allocate(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
	return Ludus::Memory::Allocate(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* pointer, std::align_val_t, const std::nothrow_t&) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

void operator delete[](void* pointer, std::align_val_t, const std::nothrow_t&) noexcept
{
	Ludus::Memory::Deallocate(pointer);
}

#endif
