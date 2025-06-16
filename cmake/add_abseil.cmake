include(FetchContent)

FetchContent_Declare(
  abseil
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG        20230802.0
)

FetchContent_MakeAvailable(abseil)