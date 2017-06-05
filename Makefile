all: julia exemple1 exemple1_2 exemple2 exemple3

julia:
	gcc -g -O3 -ffast-math -Wall -pthread `pkg-config --cflags gtk+-3.0` julia.c conduct.c `pkg-config --libs gtk+-3.0` -lm -lrt -lpthread -o julia

exemple1:
	gcc -o exemple1 exemple1.c conduct.c -lrt -lpthread

exemple1_2:
	gcc -o exemple1_2 exemple1_2.c conduct.c -lrt -lpthread

exemple2:
	gcc -o exemple2 exemple2.c conduct.c -lrt -lpthread

exemple3:
	gcc -o exemple3_serveur exemple3_serveur.c conduct.c -lrt -lpthread
	gcc -o exemple3_client exemple3_client.c conduct.c -lrt -lpthread

clean:
	rm julia
	rm exemple1
	rm exemple1_2
	rm exemple2
	rm exemple3_client
	rm exemple3_serveur