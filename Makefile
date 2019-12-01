CXX=g++-7
CPPFLAGS= -m32 -Iinclude/
LDLIBS=

all: CPPFLAGS += -O2
all: gmsv_ghttp_linux.dll

debug: CPPFLAGS += -g -O2
debug: gmsv_ghttp_linux.dll

gmsv_ghttp_linux.dll:
	${CXX} ${CPPFLAGS} -static ${LDLIBS} -fPIC -c ghttp.cpp -o ghttp.o
	${CXX} ${CPPFLAGS} ${LDLIBS} -fPIC -shared ghttp.o lib/libcurl.a -o gmsv_ghttp_linux.dll

clean:
	rm ghttp.o 
	rm gmsv_ghttp_linux.dll 