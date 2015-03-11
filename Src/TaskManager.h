#pragma once

#include "Platform.h"
#include "ConcurrentQueue.h"

#define MT_MAX_THREAD_COUNT (1)
#define MT_MAX_FIBERS_COUNT (128)
#define MT_SCHEDULER_STACK_SIZE (16384)
#define MT_FIBER_STACK_SIZE (16384)

#ifdef Yield
#undef Yield
#endif

namespace MT
{

	namespace TaskGroup
	{
		enum Type
		{
			GROUP_0 = 0,
			GROUP_1 = 1,
			GROUP_2 = 2,

			COUNT = 3
		};
	}


	struct TaskDesc;
	struct ThreadContext;
	class TaskManager;


	typedef void (MT_CALL_CONV *TTaskEntryPoint)(MT::ThreadContext & context, void* userData);


	struct FiberContext
	{
		TaskDesc * activeTask;
		ThreadContext * activeContext;

		FiberContext()
			: activeTask(nullptr)
			, activeContext(nullptr)
		{}
	};


	struct ThreadGroupEvents
	{
		MT::Event threadQueueEmpty[MT_MAX_THREAD_COUNT];
		ThreadGroupEvents();
	};


	struct FiberDesc
	{
		MT::Fiber fiber;
		MT::FiberContext * fiberContext;

		FiberDesc(MT::Fiber _fiber, MT::FiberContext * _fiberContext)
			: fiber(_fiber)
			, fiberContext(_fiberContext)
		{}

		bool IsValid() const
		{
			return (fiber != nullptr && fiberContext != nullptr);
		}

		static FiberDesc Empty()
		{
			return FiberDesc(nullptr, nullptr);
		}
	};


	struct TaskDesc
	{
		FiberDesc activeFiber;

		TTaskEntryPoint taskFunc;
		void* userData;

		TaskDesc()
			: taskFunc(nullptr)
			, userData(nullptr)
			, activeFiber(FiberDesc::Empty())
		{

		}

		TaskDesc(TTaskEntryPoint _taskEntry, void* _userData)
			: taskFunc(_taskEntry)
			, userData(_userData)
			, activeFiber(FiberDesc::Empty())
		{
		}
	};


	struct ThreadContext
	{
		MT::TaskManager* taskManager;
		MT::Thread thread;
		MT::Fiber schedulerFiber;
		MT::ConcurrentQueue<MT::TaskDesc> queue[TaskGroup::COUNT];
		MT::Event* queueEmptyEvent[TaskGroup::COUNT];
		MT::Event hasNewEvents;
		MT::TaskGroup::Type activeGroup;

		ThreadContext();
		void Yield();
	};




	class TaskManager
	{
		uint32 newTaskThreadIndex;

		int32 fibersCount;
		int32 threadsCount;
		ThreadContext threadContext[MT_MAX_THREAD_COUNT];
		ThreadGroupEvents groupDoneEvents[TaskGroup::COUNT];

		MT::ConcurrentQueue<FiberDesc> freeFibers;
		MT::FiberContext fiberContext[MT_MAX_FIBERS_COUNT];

		FiberDesc RequestFiber();
		void ReleaseFiber(FiberDesc fiberDesc);


	public:

		TaskManager();
		~TaskManager();

		
		template<typename T, int size>
		void RunTasks(TaskGroup::Type taskGroup, const T(&taskDesc)[size])
		{
			for (int i = 0; i < size; i++)
			{
				ThreadContext & context = threadContext[newTaskThreadIndex];
				newTaskThreadIndex = (newTaskThreadIndex + 1) % (uint32)threadsCount;

				//TODO: can be write more effective implementation here, just split to threads before submitting tasks to queue
				context.queue[taskGroup].Push(taskDesc[i]);
				context.queueEmptyEvent[taskGroup]->Reset();
				context.hasNewEvents.Signal();
			}
		}

		bool WaitGroup(MT::TaskGroup::Type group, uint32 milliseconds);

		static uint32 MT_CALL_CONV SchedulerThreadMain( void* userData );
		static void MT_CALL_CONV FiberMain(void* userData);

		static void ExecuteTask (MT::ThreadContext& context, MT::TaskDesc & taskDesc);

	};
}