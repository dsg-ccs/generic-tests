FROM ubuntu:22.04
RUN apt update
RUN apt install -y build-essential
RUN DEBIAN_FRONTEND=noninteractive TZ=America/NewYork apt-get install tzdata
RUN apt-get install -y python3 python3-pip ipython3
RUN apt-get install -y ninja-build
RUN apt-get install -y libglib2.0-dev
RUN apt-get install -y slirp
RUN apt-get install -y gcc-arm-linux-gnueabi
RUN apt-get install -y gcc-aarch64-linux-gnu
RUN apt-get install -y gdb
RUN apt-get install -y gdb-multiarch
RUN apt-get install -y swig bison flex
RUN apt-get install -y libssl-dev
RUN apt-get install -y bc
RUN apt-get install -y gcc-mips-linux-gnu
RUN apt-get install -y meson
RUN apt-get install -y cpio
RUN apt-get install -y python3.10-venv
RUN apt-get install -y xtightvncviewer
RUN apt-get install -y pkgconf
RUN apt-get install -y file
RUN apt-get install -y libncurses5-dev libncursesw5-dev
RUN apt-get install -y libgnutls28-dev  # for u-boot
RUN apt-get install -y git 
RUN ln -s /usr/bin/ipython3 /usr/bin/ipy
WORKDIR /usr/app
RUN python3 -m venv /usr/app/venv
ENV PATH="/usr/app/venv/bin:$PATH"
RUN pip3 install capstone
RUN pip3 install intervaltree
RUN pip3 install tomli
RUN pip3 install sphinx
RUN pip3 install angr
RUN pip3 install ipython
RUN apt-get install -y libpixman-1-dev
RUN pip3 install cxxfilt
RUN apt-get install -y strace file
RUN apt-get install -y binutils-arm-linux-gnueabi
RUN apt-get install -y binutils-aarch64-linux-gnu
RUN apt-get install -y binutils-mips-linux-gnu
RUN apt-get install -y gcc-powerpc-linux-gnu 


