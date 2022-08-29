FROM ubuntu:16.04

RUN apt-get update && apt --yes autoremove && apt-get -y install wget

RUN apt-get -y install libevent-dev && apt-get --yes install libgflags-dev
# rdma
RUN apt-get --yes install libibcm1 libibverbs1 ibverbs-utils librdmacm1 rdmacm-utils libdapl2 ibutils libibumad3 libmlx4-1 libmthca1 infiniband-diags  mstflint  perftest librdmacm-dev libmlx4-dev libibverbs-dev libevent-dev libibumad-dev

RUN apt-get -y install build-essential libssl-dev && \
    wget https://github.com/Kitware/CMake/releases/download/v3.24.1/cmake-3.24.1.tar.gz && \
    tar -zxvf cmake-3.24.1.tar.gz &&\
    cd cmake-3.24.1 && ./bootstrap && make -j16 && make install

RUN apt-get install -y unzip && wget https://github.com/fmtlib/fmt/releases/download/6.1.2/fmt-6.1.2.zip \
    && unzip fmt-6.1.2.zip && cd fmt-6.1.2/ \
    && cmake . && make -j16 && make install

