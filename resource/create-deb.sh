
mkdir .build
cd .build
cmake ..
make -j8
make install DESTDIR=/tmp/dio/
mkdir /tmp/dio/DEBIAN/
cp ../resource/control /tmp/dio/DEBIAN/control
cp ../resource/postinst /tmp/dio/DEBIAN/postinst
cp ../resource/postrm /tmp/dio/DEBIAN/postrm
cp ../resource/prerm /tmp/dio/DEBIAN/prerm
chmod 755 /tmp/dio/DEBIAN/postinst
chmod 755 /tmp/dio/DEBIAN/postrm
chmod 755 /tmp/dio/DEBIAN/prerm

dpkg-deb --build /tmp/dio
mv /tmp/dio.deb ./dio_1.0.deb
