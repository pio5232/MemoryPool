#pragma once

#include <Windows.h>
#include <queue>
#include <vector>
#include "Define.h"

namespace C_Memory
{
	enum : uint
	{
		NODE_ALIGN_SIZE = 16,
		POOL_ALIGN_SIZE = 32,

		POOL_SIZE_LEVEL_1 = 32,
		POOL_SIZE_LEVEL_2 = 128,
		POOL_SIZE_LEVEL_3 = 256,

		POOL_COUNT_LEVEL_1 = 1024 / POOL_SIZE_LEVEL_1,
		POOL_COUNT_LEVEL_2 = 1024 / POOL_SIZE_LEVEL_2,
		POOL_COUNT_LEVEL_3 = 2048 / POOL_SIZE_LEVEL_3,

		POOL_COUNT_TO_LEVEL_2 = POOL_COUNT_LEVEL_1 + POOL_COUNT_LEVEL_2,
		POOL_COUNT_TO_LEVEL_3 = POOL_COUNT_LEVEL_1 + POOL_COUNT_LEVEL_2 + POOL_COUNT_LEVEL_3,

		MEMORY_ALLOC_SIZE = 2000, //
		EXTRA_CHUNK_MOVE_SIZE = MEMORY_ALLOC_SIZE / 4,
		EXTRA_MEMORY_ALLOC_SIZE = MEMORY_ALLOC_SIZE / 10,
		MAX_ALLOC_SIZE = 4096
	};

	enum : MemoryGuard
	{
		GUARD_VALUE = 0xaabbccdd,
	};


	//------------------------------------------- For Test -----------------------------------------------
	class MemoryHeader
	{
	public:
		MemoryHeader(uint size) : _size(size) {}
		static inline void* Attach(MemoryHeader* header, uint size)
		{
			new (header) MemoryHeader(size);

			return static_cast<void*>(++header);
		}

		static inline MemoryHeader* Detach(void* ptr)
		{
			MemoryHeader* pMH = static_cast<MemoryHeader*>(ptr);

			return --pMH;
		}

		inline uint GetSize() const { return _size; }

		uint _size;
	};

	//-----------------------------------------------------------------------------------------------------

	class PoolInfo
	{
	public:
		static void Init()
		{
			uint size = 0;
			uint tableIndex = 0;

			for (size = 32; size <= 1024; size += 32)
			{
				while (tableIndex <= size)
				{
					if(tableIndex == size)
						_poolTable[tableIndex] = size / POOL_SIZE_LEVEL_1;
					else
						_poolTable[tableIndex] = (size + POOL_SIZE_LEVEL_1) / POOL_SIZE_LEVEL_1 - 1;

					tableIndex++;
				}
			}

			size -= 1024;
			for (; size <= 1024; size += 128)
			{
				while (tableIndex <= size)
				{
					if (tableIndex == size)
						_poolTable[tableIndex] = POOL_COUNT_LEVEL_1 + size / POOL_SIZE_LEVEL_2 - 1;
					else
						_poolTable[tableIndex] = POOL_COUNT_LEVEL_1 + (size + POOL_SIZE_LEVEL_2) / POOL_SIZE_LEVEL_2 - 1;

					tableIndex++;
				}
			}
			size -= 1024;

			for (; size <= 2048; size += 256)
			{
				while (tableIndex <= size)
				{
					if (tableIndex == size)
						_poolTable[tableIndex] = POOL_COUNT_TO_LEVEL_2 + size / POOL_SIZE_LEVEL_3 - 1;
					else
						_poolTable[tableIndex] = POOL_COUNT_TO_LEVEL_2 + (size + POOL_SIZE_LEVEL_3) / POOL_SIZE_LEVEL_3 - 1;

					tableIndex++;
				}
			}
		}
		static inline uint GetPoolIndex(uint size)
		{
			return _poolTable[size];
		}

		// MemoryPool을 template으로 만들다보니 placement로 호출하게하는 클래스의 타입도 template으로 할 수 있게 만들어야하지만 
		// 능력 부족으로 그러지 못했다.
		static constexpr uint consArray[POOL_COUNT_TO_LEVEL_3] = { 32 * 1, 32 * 2, 32 * 3, 32 * 4, 32 * 5, 32 * 6, 32 * 7, 32 * 8, 32 * 9,
			32 * 10, 32 * 11, 32 * 12, 32 * 13, 32 * 14, 32 * 15, 32 * 16, 32 * 17, 32 * 18, 32 * 19,
			32 * 20, 32 * 21, 32 * 22, 32 * 23, 32 * 24, 32 * 25, 32 * 26, 32 * 27, 32 * 28, 32 * 29,
			32 * 30, 32 * 31, 32 * 32,
			1024 + 128 * 1, 1024 + 128 * 2, 1024 + 128 * 3, 1024 + 128 * 4, 1024 + 128 * 5, 1024 + 128 * 6, 1024 + 128 * 7, 1024 + 128 * 8,
			2048 + 256 * 1, 2048 + 256 * 2, 2048 + 256 * 3, 2048 + 256 * 4, 2048 + 256 * 5, 2048 + 256 * 6, 2048 + 256 * 7, 2048 + 256 * 8
		};
		static uint _poolTable[MAX_ALLOC_SIZE + 1];
	};

	/*--------------------------
		   MemoryProtector
	--------------------------*/
	class MemoryProtector // Use Header + Guard 
	{
	public:
		MemoryProtector(uint size);

		static void* Attach(void* ptr, uint size);

		static MemoryProtector* Detach(void* ptr);

		inline uint GetSize() const { return _size; }

		static constexpr uint _rightGuardSpace = sizeof(MemoryGuard);// +sizeof(void*);
	private:
		uint _size; // header->size

		MemoryGuard _frontGuard; // 

	};

	/*-----------------
			Node
	------------------*/
	template <uint AllocSize>
	struct Node
	{
		char c[AllocSize]{};
		Node<AllocSize>* next;
	};


	struct NodeChunkBase
	{
		virtual ~NodeChunkBase() = 0;
	};
	/*-------------------
		  NodeChunk
	-------------------*/
	template <uint AllocSize>
	struct NodeChunk : public NodeChunkBase
	{
	public:
		using PoolNode = Node<AllocSize>;
		NodeChunk() : _head(nullptr), _size(0) {}
		~NodeChunk()
		{
			while (_head)
			{
				PoolNode* next = _head->next;

				_aligned_free(_head);

				_head = next;

				--_size;
			}

			if(_size)
			{
				std::cout << "NodeChunk Size is Not 0 - Error\n";

				TODO_TLS_LOG_ERROR;
			}
		}
		inline uint GetSize() const { return _size; }

		void* Export() // == Alloc
		{
			if (!_head)
				return nullptr;

			PoolNode* ret = _head;
			_head = _head->next;

			--_size;
			return ret;
		}

		void Regist(void* ptr) // == Free
		{
			PoolNode* newTop = static_cast<PoolNode*>(ptr);
			newTop->next = _head;
			_head = newTop;

			++_size;
		}


	private:
		inline PoolNode* GetHead() { return _head; }
		PoolNode* _head;
		uint _size;
	};


	/*-----------------------------------
		   IntegrationChunkManager
	-----------------------------------*/
	class IntegrationChunkManager // has Chunk Pool
	{
	public:
		static IntegrationChunkManager& GetInstance()
		{
			static IntegrationChunkManager instance;
			return instance;
		}

		// TODO_추가
		uint GetPoolSize(uint allocSize) const { return _chunkKeepingQ[PoolInfo::GetPoolIndex(allocSize)].size(); } // 추가
		void RegistChunk(uint poolSize, void* nodePtr);
		void* GetNodeChunk(uint poolSize);
		IntegrationChunkManager(const IntegrationChunkManager&) = delete;
		IntegrationChunkManager& operator = (const IntegrationChunkManager&) = delete;
	private:
		IntegrationChunkManager() { InitializeSRWLock(&_lock); }
		~IntegrationChunkManager();
		SRWLOCK _lock;
		std::queue<void*> _chunkKeepingQ[POOL_COUNT_TO_LEVEL_3];
	};



	class MemoryPoolBase
	{
	public:
		MemoryPoolBase() {}
		virtual ~MemoryPoolBase() = 0;

		virtual void* Alloc() abstract = 0;
		virtual void Free(void* ptr) abstract = 0;
	};

	/*---------------------------------------
		Memory Pool (memoryProtector + size)
	---------------------------------------*/
	template <uint AllocSize> // pool Allocation Size
	class MemoryPool final : public MemoryPoolBase
	{

	public:
		MemoryPool(uint managedCnt = MEMORY_ALLOC_SIZE) : _managedCount(managedCnt)
		{
			MakeChunk(_pInnerChunk);
			MakeChunk(_pExtraChunk);

			for (uint i = 0; i < _managedCount; i++)
			{
				_pInnerChunk->Regist(_aligned_malloc(AllocSize, 64));
			}

			TODO_TLS_LOG_SUCCESS;
		}
		virtual ~MemoryPool() override
		{
 			DeleteChunk(_pInnerChunk);
			DeleteChunk(_pExtraChunk);
		}
		inline void MakeChunk(NodeChunk<AllocSize>*& nodeChunk)
		{
			nodeChunk = static_cast<NodeChunk<AllocSize>*>(_aligned_malloc(sizeof(NodeChunk<AllocSize>), NODE_ALIGN_SIZE));
			new (nodeChunk) NodeChunk<AllocSize>();
		}

		void* Alloc() override
		{
			if (_pInnerChunk->GetSize() > 0)
			{
				return _pInnerChunk->Export();
			}

			if (_pExtraChunk->GetSize() > 0)
			{
				return _pExtraChunk->Export();
			}

			if (IntegrationChunkManager::GetInstance().GetPoolSize(AllocSize) > 0)
			{
				// TODO_추가
				void* newNodeChunk = IntegrationChunkManager::GetInstance().GetNodeChunk(AllocSize);

				if (newNodeChunk != nullptr)
				{
					DeleteChunk(_pInnerChunk);
					_pInnerChunk = reinterpret_cast<NodeChunk<AllocSize>*>(newNodeChunk);
					return _pInnerChunk->Export();
				}
			}
			
			for (uint i = 0; i < EXTRA_MEMORY_ALLOC_SIZE; i++)
			{
				_pInnerChunk->Regist(_aligned_malloc(AllocSize, 64));
			}
			return _pInnerChunk->Export();
			//return _aligned_malloc(AllocSize, 64);
		}

		void Free(void* ptr) override
		{
			if (_pInnerChunk->GetSize() < _managedCount)
			{
				_pInnerChunk->Regist(ptr);
				return;
			}

			_pExtraChunk->Regist(ptr); // Managing NodeChunk, if (NodeChunk Size -> 100, Regist NodeChunk to Global Manager)
			if (_pExtraChunk->GetSize() > EXTRA_CHUNK_MOVE_SIZE)
			{
				IntegrationChunkManager::GetInstance().RegistChunk(AllocSize, _pExtraChunk);

				MakeChunk(_pExtraChunk);
				return;
			}
		}
	private:
		NodeChunk<AllocSize>* _pInnerChunk;
		NodeChunk<AllocSize>* _pExtraChunk;

		const uint _managedCount;
	};



	/*---------------------------------------
			 Pool Manager ( 1 per Thread )
	---------------------------------------*/

	class PoolManager final
	{
	public:
		void* Alloc(uint size);
		void Free(void* ptr);

		PoolManager(const PoolManager&) = delete;
		PoolManager& operator = (const PoolManager&) = delete;

		PoolManager();
		~PoolManager();

		void Nothing() {};

	private:
		MemoryPoolBase* _pools[POOL_COUNT_TO_LEVEL_3];
	};

	// -------------------- Global ------------------------
	inline void DeleteChunk(NodeChunkBase* nodeChunkBase)
	{
		nodeChunkBase->~NodeChunkBase();
		_aligned_free(nodeChunkBase);
	}

}
	extern thread_local C_Memory::PoolManager poolMgr;