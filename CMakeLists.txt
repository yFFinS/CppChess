cmake_minimum_required(VERSION 3.22)
project(CppChessAi CXX)

set(CMAKE_CXX_STANDARD 23)

set(SRC_LIST src/Main.cpp src/core/Common.h src/core/Lookups.cpp src/core/Lookups.h src/core/moves/MoveGeneration.cpp src/core/moves/MoveGeneration.h src/core/Misc.h src/core/Board.cpp src/core/Board.h src/core/hash/Zobrist.cpp src/core/hash/Zobrist.h src/core/ScopedInitProfiler.h src/core/eval/IncrementalPieceSquareEvaluator.h src/core/eval/IncrementalPieceSquareEvaluator.cpp src/core/Fen.h src/core/Fen.cpp src/core/Misc.cpp src/core/Random.cpp src/ai/MoveSorter.h src/ai/Evaluation.cpp src/ai/Evaluation.h src/ai/Search.cpp src/ai/Facade.h src/ai/Facade.cpp src/ai/hash/TranspositionTable.h src/ai/hash/TranspositionTable.cpp src/database/BookMoveSelector.cpp src/database/BookMoveSelector.h)

#add_executable(CppChessAi ${SRC_LIST})
#add_library(CppChessAiLib SHARED src/Main.cpp src/core/Common.h src/core/Lookups.cpp src/core/Lookups.h src/core/moves/MoveGeneration.cpp src/core/moves/MoveGeneration.h src/core/Misc.h src/core/Board.cpp src/core/Board.h src/core/hash/Zobrist.cpp src/core/hash/Zobrist.h src/core/ScopedInitProfiler.h src/core/eval/IncrementalPieceSquareEvaluator.h src/core/eval/IncrementalPieceSquareEvaluator.cpp src/core/Fen.h src/core/Fen.cpp src/core/Misc.cpp src/core/Random.cpp src/ai/MoveSorter.h src/ai/Evaluation.cpp src/ai/Evaluation.h src/ai/Search.cpp src/ai/Facade.h src/ai/Facade.cpp  src/ai/hash/TranspositionTable.h src/ai/hash/TranspositionTable.cpp)

add_executable(CppChessAi ${SRC_LIST})
target_compile_options(CppChessAi PRIVATE $<$<CONFIG:Debug>:-Wall -Wextra -Wpedantic>)
target_compile_options(CppChessAi PRIVATE $<$<CONFIG:Release>:-O3 -Ofast -march=native -funroll-loops -frename-registers -flto>)

add_library(CppChessAiLib SHARED ${SRC_LIST})

add_executable(asan ${SRC_LIST})
target_compile_options(asan PRIVATE -Wall -Wextra -Wpedantic -fno-omit-frame-pointer -fsanitize=address)
target_link_options(asan PRIVATE -fno-omit-frame-pointer -fsanitize=address)

add_executable(tsan ${SRC_LIST})
target_compile_options(tsan PRIVATE -Wall -Wextra -Wpedantic -fno-omit-frame-pointer -fsanitize=thread)
target_link_options(tsan PRIVATE -fno-omit-frame-pointer -fsanitize=thread)


add_executable(unsan ${SRC_LIST})
target_compile_options(unsan PRIVATE -Wall -Wextra -Wpedantic -fno-omit-frame-pointer -fsanitize=undefined)
target_link_options(unsan PRIVATE -fno-omit-frame-pointer -fsanitize=undefined)