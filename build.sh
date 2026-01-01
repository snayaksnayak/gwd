mkdir build

gcc -g lex.c parse.c bigparse.c emit.c vm.c gwd.c
mv ./a.out ./build/a.out
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling average.gwd ..."
./build/a.out ./examples/average.gwd > ./build/average.out
mv ./out.ir.txt ./build/average.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling fib.gwd ..."
./build/a.out ./examples/fib.gwd > ./build/fib.out
mv ./out.ir.txt ./build/fib.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling minmax.gwd ..."
./build/a.out ./examples/minmax.gwd > ./build/minmax.out
mv ./out.ir.txt ./build/minmax.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling vector.gwd ..."
./build/a.out ./examples/vector.gwd > ./build/vector.out
mv ./out.ir.txt ./build/vector.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling charint.gwd ..."
./build/a.out ./examples/charint.gwd > ./build/charint.out
mv ./out.ir.txt ./build/charint.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling iarray.gwd ..."
./build/a.out ./examples/iarray.gwd > ./build/iarray.out
mv ./out.ir.txt ./build/iarray.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling carray.gwd ..."
./build/a.out ./examples/carray.gwd > ./build/carray.out
mv ./out.ir.txt ./build/carray.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling rec.gwd ..."
./build/a.out ./examples/rec.gwd > ./build/rec.out
mv ./out.ir.txt ./build/rec.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling icptr.gwd ..."
./build/a.out ./examples/icptr.gwd > ./build/icptr.out
mv ./out.ir.txt ./build/icptr.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling aptr.gwd ..."
./build/a.out ./examples/aptr.gwd > ./build/aptr.out
mv ./out.ir.txt ./build/aptr.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling rptr.gwd ..."
./build/a.out ./examples/rptr.gwd > ./build/rptr.out
mv ./out.ir.txt ./build/rptr.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling ref.gwd ..."
./build/a.out ./examples/ref.gwd > ./build/ref.out
mv ./out.ir.txt ./build/ref.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling func1.gwd ..."
./build/a.out ./examples/func1.gwd > ./build/func1.out
mv ./out.ir.txt ./build/func1.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling func2.gwd ..."
./build/a.out ./examples/func2.gwd > ./build/func2.out
mv ./out.ir.txt ./build/func2.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling func3.gwd ..."
./build/a.out ./examples/func3.gwd > ./build/func3.out
mv ./out.ir.txt ./build/func3.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling must.gwd ..."
./build/a.out ./examples/must.gwd > ./build/must.out
mv ./out.ir.txt ./build/must.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling poly.gwd ..."
./build/a.out ./examples/poly.gwd > ./build/poly.out
mv ./out.ir.txt ./build/poly.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling can.gwd ..."
./build/a.out ./examples/can.gwd > ./build/can.out
mv ./out.ir.txt ./build/can.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling newfree.gwd ..."
./build/a.out ./examples/newfree.gwd ./mm.gwd > ./build/newfree.out
mv ./out.ir.txt ./build/newfree.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi

echo "compiling logicop.gwd ..."
./build/a.out ./examples/logicop.gwd > ./build/logicop.out
mv ./out.ir.txt ./build/logicop.ir.txt
if [ $? -ne 0 ]; then  exit 1
fi
