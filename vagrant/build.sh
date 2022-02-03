export INSTALL_DIR=/home/vagrant
export VERSION=1.0

rm -rf ${INSTALL_DIR}/dio
mkdir -p ${INSTALL_DIR}/dio/bin
mkdir -p ${INSTALL_DIR}/dio/src
mkdir -p ${INSTALL_DIR}/dio/install

cd ${INSTALL_DIR}/dio/src
git clone https://gitlab.inria.fr/ecadorel/cgroup-monitor.git
mkdir ${INSTALL_DIR}/dio/src/cgroup-monitor/.build
cd ${INSTALL_DIR}/dio/src/cgroup-monitor/.build

cmake ..
make -j4
make install DESTDIR=${INSTALL_DIR}/dio/install

mkdir -p ${INSTALL_DIR}/dio/install/usr/lib/dio
cp -r ${INSTALL_DIR}/dio/src/cgroup-monitor/libs/* ${INSTALL_DIR}/dio/install/usr/lib/dio/

cd ${INSTALL_DIR}/dio/bin
sudo fpm -s dir -t deb -n libdio-${VERSION} -v ${VERSION} -C ${INSTALL_DIR}/dio/install \
    -p libdio_${VERSION}.deb \
    -d qemu-kvm \
    -d libvirt-daemon-system \
    -d libvirt-clients \
    -d bridge-utils \
    -d virtinst \
    -d virt-manager \
    -d nfs-common \
    -d libguestfs-tools \
    -d dnsmasq-utils \
    usr/bin usr/lib/dio

