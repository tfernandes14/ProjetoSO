final: main.o drone_movement.o
	gcc main.o drone_movement.o -o final -g -lm -Wall -pthread -D_REENTRANT

main.o: main.c drone_movement.h
	gcc main.c -c -g -Wall -pthread

drone_movement.o: drone_movement.c drone_movement.h
	gcc drone_movement.c -c -g -lm -Wall

clean:
	rm *.o final
