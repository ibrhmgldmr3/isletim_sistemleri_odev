Anakural:derle calistir

derle:
	g++ -I ./include/ -o ./lib/AVLagac.o -c ./src/AVLagac.cpp
	g++ -I ./include/ -o ./lib/AVLAgaclari.o -c ./src/AVLAgaclari.cpp
	g++ -I ./include/ -o ./lib/AvlDugum.o -c ./src/AvlDugum.cpp
	g++ -I ./include/ -o ./lib/Yigin.o -c ./src/Yigin.cpp
	g++ -I ./include/ -o ./bin/Main ./lib/AVLagac.o ./lib/AVLAgaclari.o ./lib/AvlDugum.o ./lib/Yigin.o ./src/main.cpp


calistir:
	cls
	./bin/Main.exe