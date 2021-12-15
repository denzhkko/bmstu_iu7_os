.PHONY: all clean

all: producers-consumers readers-writers

producers-consumers: out/producers-consumers.c.o
	$(CC) -o $@ $^

readers-writers: out/readers-writers.c.o
	$(CC) -o $@ $^

out/%.o: src/%
	@mkdir -p out/
	$(CC) -o $@ -c $<

clean:
	rm -f readers-writers
	rm -f producers–consumers
	rm -f -r out
