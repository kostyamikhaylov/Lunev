TAG = dict
DEP1 = main
DEP2 = obj
DEPS = $(DEP1).o $(DEP2).c


$(TAG): $(DEPS)
	gcc -g -Wall $^ -o $@
	rm -f *.o

%.o: %.c
	gcc -g -c $^ -o $@

clean:
	rm -f *.o $(TAG)

