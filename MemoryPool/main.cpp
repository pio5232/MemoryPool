// Pool Node 안전성 테스트 필요
#include <iostream>
#include <crtdbg.h>
#include <thread>
#include <chrono>
#include "MemoryPool.h"
//C_Utility::CCrashDump dump;

const int allocSize = 40;
const int ExeCntPerLoop = 3200; // 100 ~ 10000
const int threadCnt = 8;


struct sts
{
	sts() {}
	~sts() {}
	char c[allocSize]{};
};
int loopCnt = 0100000;
void CheckMyPool()
{
	//C_Memory::PoolInfo::Init();
	{
		std::thread t[threadCnt];

		for (int j = 0; j < threadCnt; j++)
		{
			t[j] = std::thread([=]() {
				poolMgr.Nothing();
				int st = 0;
				void** arr = new void* [ExeCntPerLoop];

				auto start = std::chrono::high_resolution_clock::now();
				while (st < loopCnt)
				{
					for (int i = 0; i < ExeCntPerLoop; i++)
					{
						arr[i] = static_cast<int*>(poolMgr.Alloc(sizeof(sts)));
					}

					for (int i = 0; i < ExeCntPerLoop; i++)
					{
						poolMgr.Free(arr[i]);
					}

					++st;
				};

				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = end - start;
				printf("%d 스레드 %lf 초 경과\n", j, elapsed);
				delete[] arr;
				}
			);
		}

		/*{
			int st = 0;
			void** arr = new void* [ExeCntPerLoop];
			{
				auto start = std::chrono::high_resolution_clock::now();
				while (st < loopCnt)
				{
					for (int i = 0; i < ExeCntPerLoop; i++)
					{
						arr[i] = poolMgr.Alloc(sizeof(sts));
					}

					for (int i = 0; i < ExeCntPerLoop; i++)
					{
						poolMgr.Free(arr[i]);
					}

					++st;
				}

				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = end - start;
				printf("main 스레드 %lf 초 경과\n", elapsed);
			}
			delete[] arr;
		}*/
		for (auto& th : t)
		{
			if (th.joinable())
				th.join();
		}
	}
}
void CheckNew()
{
	std::thread t[threadCnt];

	for (int j = 0; j < threadCnt; j++)
	{
		t[j] = std::thread([=]() {
			int st = 0;
			void** arr = new void* [ExeCntPerLoop];

			auto start = std::chrono::high_resolution_clock::now();
			while (st < loopCnt)
			{
				for (int i = 0; i < ExeCntPerLoop; i++)
				{
					arr[i] = new sts;
				}

				for (int i = 0; i < ExeCntPerLoop; i++)
				{
					delete (arr[i]);
				}

				++st;
			};

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> elapsed = end - start;
			printf("%d 스레드 %lf 초 경과\n", j, elapsed);
			delete[] arr;
			}
		);
	}

	{
		//int st = 0;
		//void** arr = new void* [ExeCntPerLoop];
		//{
		//	auto start = std::chrono::high_resolution_clock::now();
		//	while (st < loopCnt)
		//	{
		//		for (int i = 0; i < ExeCntPerLoop; i++)
		//		{
		//			arr[i] = new sts;
		//		}

		//		for (int i = 0; i < ExeCntPerLoop; i++)
		//		{
		//			delete (arr[i]);
		//		}

		//		++st;
		//	}

		//	auto end = std::chrono::high_resolution_clock::now();
		//	std::chrono::duration<double> elapsed = end - start;
		//	printf("new - main 스레드 %lf 초 경과\n", elapsed);
		//}
		//delete[] arr;

		for (auto& th : t)
		{
			if (th.joinable())
				th.join();
		}
	}
}
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	CheckMyPool();

	std::cout << "============================================\n";

	CheckNew();
}
