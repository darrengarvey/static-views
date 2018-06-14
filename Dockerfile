FROM gcc:8

WORKDIR /src

ENV BOOST_ROOT=boost-trunk
ENV SPHINX_ROOT=sphinx-trunk
ENV BREATHE_ROOT=breathe-trunk
ENV PYTHONPATH=${BREATHE_ROOT}/build/lib:${PYTHONPATH}

ENV TOOLSET=gcc
ENV COMPILER=gcc
ENV CXXFLAGS="-std=c++14"
ENV BENCHMARK=true

RUN apt update && apt install -y python3-setuptools python3-pip

RUN git clone --recursive -b master https://github.com/michaeljones/breathe ${BREATHE_ROOT}
RUN cd ${BREATHE_ROOT} && python3 setup.py build

RUN git clone --depth 1 https://github.com/boostorg/boost.git ${BOOST_ROOT}
# For now, let StaticViews explicitely depend on two libraries: Boost.Core 
# and Boost.Config. We thus get the following dependency "tree"
# 
# boost/config <--+-- boost/detail
#                 |-- boost/assert
#                 |-- boost/core
#                 +-- boost/type_traits
#
# boost/core   <--+-- boost/assert
#                 |-- boost/config
#                 |-- boost/predef
#                 |-- boost/static_assert
#                 +-- boost/type_traits
# Hence
RUN git -C ${BOOST_ROOT} submodule update --init \
    tools/build \
    libs/assert \
    libs/config \ 
    libs/core \
    libs/detail \
    libs/predef \
    libs/static_assert \
    libs/type_traits

ENV STATIC_VIEWS_ROOT=${BOOST_ROOT}/libs/static_views

RUN mkdir -p ${STATIC_VIEWS_ROOT}
ADD . ${STATIC_VIEWS_ROOT}/
RUN cd ${BOOST_ROOT} && ./bootstrap.sh && ./b2 headers
