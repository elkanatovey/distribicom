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
    echo 'LogLevel DEBUG2'; \
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


CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_test_clion"]
