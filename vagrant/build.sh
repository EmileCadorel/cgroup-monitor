export INSTALL_DIR=/home/vagrant
VERSION=$1

rm -rf ${INSTALL_DIR}/dio
mkdir -p ${INSTALL_DIR}/dio/bin
mkdir -p ${INSTALL_DIR}/dio/src
mkdir -p ${INSTALL_DIR}/dio/install

cd ${INSTALL_DIR}/dio/src
git clone https://gitlab.inria.fr/ecadorel/cgroup-monitor.git
mkdir ${INSTALL_DIR}/dio/src/.build
cd ${INSTALL_DIR}/dio/src/.build

cmake ..
make -j4
make install DESTDIR=${INSTALL_DIR}/dio/install

mkdir -p ${INSTALL_DIR}/dio/install/var/lib/dio
cp -r ${INSTALL_DIR}/dio/src/lib/* ${INSTALL_DIR}/dio/install/var/lib/dio/

cd ${INSTALL_DIR}/dio/bin
fpm -s dir -t deb -n libdio-${VERSION} -v ${VERSION} -C ${INSTALL_DIR}/dio/install \
    -p qemu-kvm \
    -p libvirt-daemon-system \
    -p libvirt-clients \
    -p bridge-utils \
    -p virtinst \
    -p virt-manager \
    -p nfs-common \
    -p libguestfs-tools \
    -p dnsmasq-utils \
    usr/bin /var/lib/dio/cpu-market.json /var/lib/dio/daemon.json /var/lib/dio/mem-market.json /var/lib/dio/keys/key /var/lib/dio/keys/key.pub

