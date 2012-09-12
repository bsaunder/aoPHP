#Compile/Install aophp
install: testrdp StringTokenizer MD5
	g++ -lm aophp.cpp StringTokenizer.o md5.o -o aophp

#Compile Debugger
testrdp: StringTokenizer
	g++ -lm testrdp.cpp StringTokenizer.o -o testrdp

#Delete aophp
remove:	clean
	rm aophp -f
	rm testrdp -f

#Compile MD5
MD5:	md5.cpp md5.h
	g++ -c md5.cpp

#Compile Stringtokenizer
StringTokenizer : StringTokenizer.cpp StringTokenizer.h
	g++ -c StringTokenizer.cpp

#Clean
clean:	
	rm *.*~ -f
	rm *.~ -f
	rm *~ -f
	rm *.o -f
	rm \#* -f
	rm test/\#* -f
	rm test/*~ -f
	rm *.php -f
