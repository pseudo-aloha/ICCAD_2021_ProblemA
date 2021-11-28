cd abc
make libabc.a
cd ../aiger
chmod +x ./configure.sh
./configure.sh
make
cd ../src
ln -s ../abc/libabc.a ./lib/libabc.a
make
cd ..
