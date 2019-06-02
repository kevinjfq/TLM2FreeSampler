FROM ubuntu:16.04

RUN apt-get -y update && \
    apt-get -y install \
		apt-transport-https \
		libgtk2.0-0 \
		libxss1 \
		libasound2 \
		wget \
		clang \
		cmake \
		make \
		libgtest-dev \
		vim \
		curl \
		gnupg \
		git && \
	curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > /etc/apt/trusted.gpg.d/microsoft.gpg && \
	sh -c 'echo "deb [arch=amd64] https://packages.microsoft.com/repos/vscode stable main" > /etc/apt/sources.list.d/vscode.list' && \
	apt-get -y update && \
	apt-get -y install code

# Install the 'Catch2' unittest framework.  It's a single header file.  #include <catch2/catch.hpp>
# ADD https://github.com/catchorg/Catch2/releases/download/v2.2.3/catch.hpp /usr/include/catch2/catch.hpp
# Another approach...
RUN mkdir /usr/include/catch2 && \
	curl https://raw.githubusercontent.com/catchorg/Catch2/master/single_include/catch2/catch.hpp >> /usr/include/catch2/catch.hpp && \
	chmod 644 /usr/include/catch2/catch.hpp

# Build GTest library
RUN cd /usr/src/gtest && \
	cmake . && \
	make && \
	cp *.a /usr/lib && \
	mkdir /usr/local/lib/gtest && \
	ln -s /usr/lib/libgtest.a /usr/local/lib/gtest/libgtest.a && \
	ln -s /usr/lib/libgtest_main.a /usr/local/lib/gtest/libgtest_main.a

# Get the RapidJSON Dependency
# Move the RapidJSON header files to the include path
RUN git clone https://github.com/miloyip/rapidjson.git && \
	cp -r rapidjson/include/rapidjson/ /usr/local/include

ENV CC /usr/bin/clang 
ENV CXX /usr/bin/clang++ 
ENV DOWNLOADS /opt/downloads 
ENV ACCELLERA_SYSTEMC_VER 2.3.2 
ENV ACCELLERA_SYSTEMC_FILE_NOEXT systemc-$ACCELLERA_SYSTEMC_VER 
ENV ACCELLERA_SYSTEMC_FILE $ACCELLERA_SYSTEMC_FILE_NOEXT.tar.gz 
ENV ACCELLERA_SYSTEMC_URL http://www.accellera.org/images/downloads/standards/systemc/$ACCELLERA_SYSTEMC_FILE 
ENV SYSTEMC_HOME /opt/tools/accellera/systemc 
ENV DEVELOPER_USER devuser 
ENV DEVELOPER_USER_HOME /home/$DEVELOPER_USER 
ENV DEVELOPER_USER_WORK $DEVELOPER_USER_HOME/work 
ENV EXAMPLE_PROJECT $DEVELOPER_USER_WORK/model_example
ENV TLM2FREESAMPLER_PROJECT $DEVELOPER_USER_WORK/TLM2FreeSampler

RUN mkdir -p $SYSTEMC_HOME && mkdir -p $DOWNLOADS && cd $DOWNLOADS && wget -c $ACCELLERA_SYSTEMC_URL && tar xfz $ACCELLERA_SYSTEMC_FILE && cd $ACCELLERA_SYSTEMC_FILE_NOEXT && ./configure 'CXXFLAGS=-std=c++11' --prefix=$SYSTEMC_HOME && make && make install 
RUN useradd -ms /bin/bash $DEVELOPER_USER 
RUN mkdir $DEVELOPER_USER_WORK && cd $DEVELOPER_USER_WORK && git clone https://github.com/engest/model_example.git && cd /home && chown -R devuser.devuser devuser && cd $EXAMPLE_PROJECT 
RUN cd $DEVELOPER_USER_WORK && git clone https://github.com/kevinjfq/tlm2freesampler.git && cd /home && chown -R devuser.devuser devuser && cd $TLM2FREESAMPLER_PROJECT 
USER $DEVELOPER_USER 
ENV HOME $DEVELOPER_USER_HOME 
RUN cd $DEVELOPER_USER_HOME && code --install-extension ms-vscode.cpptools 
RUN cd $DEVELOPER_USER_HOME && code --install-extension mitaki28.vscode-clang 
RUN cd $DEVELOPER_USER_HOME && code --install-extension twxs.cmake 
RUN cd $DEVELOPER_USER_HOME && code --install-extension vector-of-bool.cmake-tools 
RUN cd $DEVELOPER_USER_HOME && code --install-extension vscodevim.vim WORKDIR $EXAMPLE_PROJECT

COPY helloworld.sh /bin
CMD ["/bin/helloworld.sh"]
CMD ["echo", "hello world from the dockerfile"]
