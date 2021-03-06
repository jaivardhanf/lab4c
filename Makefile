#NAME: Jai Vardhan Fatehpuria
#EMAIL: jaivardhan.f@gmail.com
#ID: 804817305

default:
	gcc -o lab4c_tcp -Wall -Wextra -lm -lmraa -g lab4c_tcp.c
	gcc -o lab4c_tls -Wall -Wextra -lm -lmraa -g -lssl -lcrypto lab4c_tls.c

clean:
	rm -f lab4c-804817305.tar.gz lab4c_tcp lab4c_tls

dist:
	tar -czvf lab4c-804817305.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
