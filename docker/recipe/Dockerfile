FROM alpine:3.9 AS builder

WORKDIR /nmodl/src

RUN apk add --update build-base gcc g++ make cmake flex flex-dev bison git python3-dev

RUN pip3 install --trusted-host pypi.python.org jinja2 pyyaml pytest sympy


ARG NMODL_VERSION=master

RUN git clone --recursive https://github.com/BlueBrain/nmodl.git && \
    cd nmodl && \
    git checkout ${NMODL_VERSION}

WORKDIR /nmodl/src/nmodl

RUN python3 setup.py build

FROM alpine:3.9


RUN apk add --no-cache --update shadow python3 libgfortran libstdc++ openblas && \
    apk add --no-cache --update \
            --repository http://dl-cdn.alpinelinux.org/alpine/edge/testing gosu && \
    apk add --no-cache --virtual build-dependencies \
            build-base linux-headers openblas-dev freetype-dev \
            pkgconfig gfortran python3-dev && \
    pip3 install --no-cache-dir --trusted-host pypi.python.org \
                 jinja2 pyyaml pytest sympy numpy matplotlib jupyter && \
    apk del build-dependencies && \
    rm -rf /var/cache/apk/*

WORKDIR /usr/lib/python3.6/site-packages/nmodl

COPY --from=builder /nmodl/src/nmodl/build/lib.linux-x86_64-3.6/nmodl .

ENV LANG en_US.utf8
ENV SHELL=/bin/bash

ADD entrypoint /usr/bin/
ENTRYPOINT ["/usr/bin/entrypoint"]

EXPOSE 8888
WORKDIR /nmodl/notebooks

COPY --from=builder /nmodl/src/nmodl/docs/notebooks ./examples

CMD ["jupyter", "notebook", "--port=8888", "--no-browser", "--ip=0.0.0.0", "--allow-root", "--notebook-dir=/nmodl/notebooks"]

