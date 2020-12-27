all: demo

demo: demo.c
	gcc -o $@ $<

clean:
	rm -f demo
