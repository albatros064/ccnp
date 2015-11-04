CC = gcc
CPPFLAGS =
CFLAGS = -pedantic

objects = main.o preprocess.o compile.o container.o message.o

preprocess: $(objects)
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS)


clean:
	-rm *.o preprocess
