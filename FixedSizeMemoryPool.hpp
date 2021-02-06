#pragma once

#include <cstddef>
#include <memory>

class FixedSizeMemoryPool
{
  public:

	FixedSizeMemoryPool(
		size_t segmentSize, 
		size_t defaultGrowth = 64) noexcept :
		_segmentSize{ segmentSize < sizeof(void*) ? sizeof(void*) : segmentSize },
		_defaultGrowthSize{ defaultGrowth == 0 ? 1 : defaultGrowth }
	{
	}

	FixedSizeMemoryPool(const FixedSizeMemoryPool& other) = delete;

	FixedSizeMemoryPool(FixedSizeMemoryPool&& other) = delete;

    ~FixedSizeMemoryPool()
    {
		while (_lastAllocPage != nullptr)
		{
			void* nextPage = NextOf(_lastAllocPage);

			free(_lastAllocPage);
			
			_lastAllocPage = nextPage;
		}
    }

    inline void* Allocate() noexcept
    {
		if (IsEmpty())
		{
			if (!Grow(_defaultGrowthSize))
			{
				return nullptr;
			}
		}

		void* ret = _frontSegment;

		_frontSegment = NextOf(_frontSegment);

		return ret;
    }

	inline void Deallocate(void* ptr) noexcept
	{
		NextOf(ptr) = _frontSegment;

		_frontSegment = ptr;
	}

	inline size_t GetSegmentSize() const noexcept
	{
		return _segmentSize;
	}

    inline bool IsEmpty() const noexcept
    {
		return _frontSegment == nullptr;
    }

	bool Grow(size_t segmentCount)
	{
		const size_t newPageSize = sizeof(void*) + (_segmentSize * segmentCount);
		
		char* ptr = static_cast<char*>(malloc(newPageSize));
		if (ptr == nullptr)
		{
			return false;
		}

		NextOf(ptr) = _lastAllocPage;
		_lastAllocPage = ptr;

		char* endOfNewPage = ptr + newPageSize;
		ptr += sizeof(void*); 

		while (ptr != endOfNewPage)
		{
			NextOf(ptr) = _frontSegment;
			_frontSegment = ptr;
			ptr += _segmentSize;
		}

		return true;
	}

  private:

    static inline void*& NextOf(void* pageOrSegment) noexcept
    {
		return *static_cast<void**>(pageOrSegment);
    }

    void* _lastAllocPage = nullptr;
    void* _frontSegment = nullptr;
	const size_t _segmentSize;
    const size_t _defaultGrowthSize;
};