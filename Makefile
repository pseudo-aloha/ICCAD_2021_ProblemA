

all:
	chmod +x ./build.sh
	./build.sh
clean:
	cd ../aiger
	make clean
	cd ./eco
	make clean

