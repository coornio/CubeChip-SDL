CMAKE_MINIMUM_REQUIRED(VERSION 3.25)

add_library(
	atomic_queue INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue.h
	${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue_mutex.h
	${CMAKE_CURRENT_SOURCE_DIR}/barrier.h
	${CMAKE_CURRENT_SOURCE_DIR}/defs.h
	${CMAKE_CURRENT_SOURCE_DIR}/spinlock.h
)

target_include_directories(
	atomic_queue
	INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
)
