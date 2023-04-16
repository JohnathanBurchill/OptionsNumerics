# OptionsNumerics: Docker/Dockerfile

# Copyright (C) 2023  Johnathan K Burchill

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

FROM ubuntu
RUN apt -y update
RUN apt -y upgrade
RUN apt -y install wget
RUN apt -y install gcc
RUN apt -y install g++
RUN apt -y install cmake
RUN apt -y install make
RUN apt -y install bash
RUN apt -y install git
RUN apt -y install gdb
RUN apt -y install cgdb
RUN apt -y install vim
RUN apt -y install pkg-config
RUN apt -y install libncurses-dev
RUN apt -y install openssl
RUN apt -y install libssl-dev
RUN apt -y install gnutls-dev
RUN useradd hacker
USER hacker
WORKDIR /home/hacker
RUN cd /home/hacker && \
git clone https://github.com/akheron/jansson && \
cd jansson && \
mkdir build 
RUN cd /home/hacker/jansson/build && \
cmake .. -DJANSSON_BUILD_DOCS=OFF && \
make
USER root
RUN cd /home/hacker/jansson/build && \
make install && \
ldconfig
USER hacker
RUN cd /home/hacker && \
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/curl/curl.git
RUN cd /home/hacker/curl && \
mkdir build && \
cd build && \
cmake .. && \
make
USER root
RUN cd /home/hacker/curl/build && \
make install && \
ldconfig
USER hacker
ENV SHELL /bin/bash
WORKDIR /home/hacker/src
VOLUME ["/home/hacker/src"]
ENTRYPOINT ["/bin/bash"]

