git co builddev
TARGET=home/s/safe
rm -rf $TARGET
mkdir $TARGET/lib -p
mkdir $TARGET/include/muduo/base -p
mkdir $TARGET/include/muduo/net -p

cd ..
cp -rf build/release/lib/* $TARGET/lib
cp -rf muduo/muduo $TARGET/include
find $TARGET -name *.cc | xargs rm -rf
find $TARGET -name *.txt | xargs rm -rf

tar zcvf muduo.tar.gz home
