.PHONY: all clean
all: README.html

README.html: README.md
	markdown $< > $@

clean:
	rm -f README.html
