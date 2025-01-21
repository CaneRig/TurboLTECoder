mkdir Build

g++ Turbo/Turbo.cpp Turbo/Turbo/Decoders/SSE_Windowed/SSE_Windowed.cpp -Ofast -msse4.1 -std=c++2a -o Build/g++-build.o
