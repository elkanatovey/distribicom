# Remote clion builds:
# docker build . -t clion/remote-env:1.0
# docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 --name clion/remote-env clion/remote-env:1.0
FROM ubuntu:latest
RUN apt-get update
RUN apt-get install -y ssh
RUN apt-get install -y build-essential
RUN apt-get install -y gcc \
    g++ \
    gdb \
    clang \
    cmake
RUN apt-get install -y  rsync
RUN apt-get install -y tar
RUN apt-get install -y python3
RUN apt-get install -y git
RUN apt-get install -y valgrind
RUN apt-get clean


RUN ( \
    echo 'LogLevel ERROR'; \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
    echo 'AllowUsers user'; \
  ) > /etc/ssh/sshd_config_test_clion \
  && mkdir /run/sshd

RUN useradd -m user \
  && yes password | passwd user

RUN usermod -s /bin/bash user
RUN mkdir -p /home/user/.ssh && \
    chown user /home/user/.ssh &&\
    chgrp user /home/user/.ssh &&\
    chmod 666 /home/user/.ssh &&\
    touch /home/user/.ssh/authorized_keys &&\
    chmod 666 /home/user/.ssh/authorized_keys

COPY deps.sh /deps.sh
RUN /deps.sh

RUN apt-get install -y vim
RUN apt-get install -y sudo

RUN apt-get -y install graphviz pip kcachegrind
RUN pip install gprof2dot # ./gprof2dot -f callgrind callgrind.out.x | dot -Tsvg -o output.svg
RUN apt-get -y install mutrace # to detect mutex contention.

# so you can install perf from source...
RUN wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.86.tar.xz && \
    tar -xf ./linux-5.15.86.tar.xz && \
    cd linux-5.15.86/tools/perf/ && \
    apt -y install flex bison && \
    make -C . && make install

#git clone https://github.com/brendangregg/FlameGraph
#cd FlameGraph
#/linux-4.14.173/tools/perf/perf record -F 99 -p {pid} -g -- sleep 60
#/linux-4.14.173/tools/perf/perf script | ./stackcollapse-perf.pl > out.perf-folded
#./flamegraph.pl out.perf-folded > perf-kernel.svg

ENV PATH="${PATH}:/root/bin"
CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_test_clion"]
