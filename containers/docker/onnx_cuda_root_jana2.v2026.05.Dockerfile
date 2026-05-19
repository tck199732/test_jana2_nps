ARG CUDA_IMAGE=nvcr.io/nvidia/cuda:12.8.1-cudnn-devel-ubuntu24.04

FROM ${CUDA_IMAGE} AS onnx-builder

ARG DEBIAN_FRONTEND=noninteractive
ARG ONNXRUNTIME_VERSION=1.21.0
ARG CMAKE_VERSION=4.0.0
ARG EIGEN_COMMIT=1d8b82b0740839c0de7f1242a3585e3390ff5f33

ENV LANG=en_US.UTF-8 \
    LANGUAGE=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    build-essential \
    ca-certificates \
    git \
    libtool \
    locales \
    locales-all \
    pkg-config \
    python3-full \
    software-properties-common \
    unzip \
    wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN bash -o pipefail -c "cd /opt \
    && wget -q https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh \
    && bash cmake-${CMAKE_VERSION}-linux-x86_64.sh \
        --prefix=/usr/local \
        --exclude-subdir \
        --skip-license \
    && rm -f cmake-${CMAKE_VERSION}-linux-x86_64.sh"

RUN bash -o pipefail -c "cd /opt \
    && git clone --recursive --branch v${ONNXRUNTIME_VERSION} https://github.com/Microsoft/onnxruntime \
    && wget -q -O eigen.zip https://gitlab.com/libeigen/eigen/-/archive/${EIGEN_COMMIT}/eigen-${EIGEN_COMMIT}.zip \
    && unzip -q eigen.zip -d /opt/eigen-source \
    && rm -f eigen.zip \
    && cd /opt/onnxruntime \
    && ./build.sh \
        --allow_running_as_root \
        --cuda_home /usr/local/cuda \
        --cudnn_home /usr/lib/x86_64-linux-gnu/ \
        --use_cuda \
        --config RelWithDebInfo \
        --build_shared_lib \
        --skip_tests \
        --parallel "$(nproc)" \
        --cmake_extra_defines FETCHCONTENT_SOURCE_DIR_EIGEN=/opt/eigen-source/eigen-${EIGEN_COMMIT} \
    && cd build/Linux/RelWithDebInfo \
    && make install \
    && rm -rf /opt/onnxruntime /opt/eigen-source"


FROM ${CUDA_IMAGE} AS root-builder

ARG DEBIAN_FRONTEND=noninteractive
ARG ROOT_BIN=root_v6.34.00.Linux-ubuntu24.04-x86_64-gcc13.2.tar.gz

ENV LANG=C.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    curl \
    davix-dev \
    dcap-dev \
    fonts-freefont-ttf \
    g++ \
    gcc \
    gfortran \
    git \
    libafterimage-dev \
    libcfitsio-dev \
    libfcgi-dev \
    libfftw3-dev \
    libfreetype6-dev \
    libftgl-dev \
    libgfal2-dev \
    libgif-dev \
    libgl2ps-dev \
    libglew-dev \
    libglu-dev \
    libgraphviz-dev \
    libgsl-dev \
    libjpeg-dev \
    liblz4-dev \
    liblzma-dev \
    libmysqlclient-dev \
    libpcre2-dev \
    libpcre3-dev \
    libpng-dev \
    libpq-dev \
    libsqlite3-dev \
    libssl-dev \
    libtbb-dev \
    libtiff-dev \
    libvdt-dev \
    libx11-dev \
    libxext-dev \
    libxft-dev \
    libxml2-dev \
    libxpm-dev \
    libxxhash-dev \
    libz-dev \
    libzstd-dev \
    locales \
    make \
    nlohmann-json3-dev \
    protobuf-compiler \
    python-is-python3 \
    python3-dev \
    python3-numpy \
    srm-ifce-dev \
    unixodbc-dev \
    unzip \
    wget \
    xrootd-plugins \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN bash -o pipefail -c "mkdir -p /opt/root \
    && cd /opt/root \
    && ln -sf /usr/share/zoneinfo/UTC /etc/localtime \
    && wget -q https://root.cern/download/${ROOT_BIN} \
    && tar --strip-components=1 -xzf ${ROOT_BIN} \
    && rm -f ${ROOT_BIN}"


FROM root-builder AS jana-builder

ARG DEBIAN_FRONTEND=noninteractive
ARG JANA2_VERSION=v2026.02.00

ENV ROOTSYS=/opt/root \
    JANA_HOME=/opt/JANA2 \
    PATH=/opt/root/bin:/opt/JANA2/bin:${PATH} \
    PYTHONPATH=/opt/root/lib:${PYTHONPATH} \
    CLING_STANDARD_PCH=none \
    LD_LIBRARY_PATH=/opt/root/lib:/opt/JANA2/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/JANA2/cmake/lib/JANA:${CMAKE_PREFIX_PATH}

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    libx11-dev \
    libxext-dev \
    libxft-dev \
    libxpm-dev \
    python3-dev \
    unzip \
    wget \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN bash -o pipefail -c "git clone --branch ${JANA2_VERSION} https://github.com/JeffersonLab/JANA2 /opt/JANA2 \
    && cmake -S /opt/JANA2 -B /opt/JANA2/build -DCMAKE_INSTALL_PREFIX=/opt/JANA2 \
    && cmake --build /opt/JANA2/build --target install -j$(nproc) \
    && rm -rf /opt/JANA2/build /opt/JANA2/.git"


FROM ${CUDA_IMAGE}

ARG DEBIAN_FRONTEND=noninteractive

ENV ROOTSYS=/opt/root \
    JANA_HOME=/opt/JANA2 \
    PATH=/usr/local/bin:/opt/root/bin:/opt/JANA2/bin:${PATH} \
    PYTHONPATH=/opt/root/lib:${PYTHONPATH} \
    CLING_STANDARD_PCH=none \
    LD_LIBRARY_PATH=/usr/local/lib:/opt/root/lib:/opt/JANA2/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/opt/JANA2/cmake/lib/JANA:${CMAKE_PREFIX_PATH}

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    build-essential \
    ca-certificates \
    curl \
    davix-dev \
    dcap-dev \
    fonts-freefont-ttf \
    git \
    libafterimage-dev \
    libcfitsio-dev \
    libfcgi-dev \
    libfftw3-dev \
    libfreetype6-dev \
    libftgl-dev \
    libgfal2-dev \
    libgif-dev \
    libgl2ps-dev \
    libglew-dev \
    libglu-dev \
    libgraphviz-dev \
    libgsl-dev \
    libjpeg-dev \
    liblz4-dev \
    liblzma-dev \
    libmysqlclient-dev \
    libpcre2-dev \
    libpcre3-dev \
    libpng-dev \
    libpq-dev \
    libsqlite3-dev \
    libssl-dev \
    libtbb-dev \
    libtool \
    libtiff-dev \
    libvdt-dev \
    libx11-dev \
    libxext-dev \
    libxft-dev \
    libxml2-dev \
    libxpm-dev \
    libxxhash-dev \
    libz-dev \
    libzstd-dev \
    locales \
    locales-all \
    pkg-config \
    python3 \
    python-is-python3 \
    python3-dev \
    python3-full \
    python3-numpy \
    srm-ifce-dev \
    software-properties-common \
    unixodbc-dev \
    unzip \
    wget \
    xrootd-plugins \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY --from=onnx-builder /usr/local/bin/ /usr/local/bin/
COPY --from=onnx-builder /usr/local/include/ /usr/local/include/
COPY --from=onnx-builder /usr/local/lib/ /usr/local/lib/
COPY --from=onnx-builder /usr/local/share/cmake-4.0 /usr/local/share/cmake-4.0
COPY --from=root-builder /opt/root /opt/root
COPY --from=jana-builder /opt/JANA2 /opt/JANA2

RUN echo /usr/local/lib > /etc/ld.so.conf.d/onnxruntime.conf \
    && echo /opt/root/lib > /etc/ld.so.conf.d/root.conf \
    && echo /opt/JANA2/lib > /etc/ld.so.conf.d/jana2.conf \
    && ldconfig

ENV ROOTSYS=/opt/root \
    JANA_HOME=/opt/JANA2 \
    PATH=/usr/local/bin:/opt/root/bin:/opt/JANA2/bin:${PATH} \
    PYTHONPATH=/opt/root/lib:/opt/root/lib/root:${PYTHONPATH} \
    CLING_STANDARD_PCH=none \
    LD_LIBRARY_PATH=/usr/local/lib:/opt/root/lib:/opt/JANA2/lib:${LD_LIBRARY_PATH} \
    CMAKE_PREFIX_PATH=/usr/local:/opt/root:/opt/JANA2:/opt/JANA2/cmake/lib/JANA:${CMAKE_PREFIX_PATH}

CMD ["/bin/bash"]
