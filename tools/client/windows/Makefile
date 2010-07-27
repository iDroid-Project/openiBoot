all: linux
	@echo "Please choose either macosx, linux, or windows"

linux:
	gcc -o irecovery src/irecovery.c -g -I./include -L. -lirecovery -lreadline

macosx:
	gcc -o irecovery src/irecovery.c -I./include -L. -lirecovery -lreadline -lusb-1.0
	
windows:
	gcc -o irecovery irecovery.c -I. -lirecovery -lreadline
	
install:
	cp irecovery /usr/local/bin/irecovery

uninstall:
	rm -rf /usr/local/bin/irecovery
		
clean:
	rm -rf irecovery 
				
