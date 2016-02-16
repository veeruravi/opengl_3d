all: sample2D

sample2D: Sample_GL3_2D.cpp glad.c
	g++ -g -o sample2D veeru.cpp glad.c -w -lGL -ldl -lglfw -lftgl -lSOIL -I/usr/local/include -I/usr/local/include/freetype2 -L/usr/local/lib -lao -lmpg123 -std=c++11 -lpthread;./sample2D
clean:
	rm sample2D
