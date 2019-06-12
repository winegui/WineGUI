FROM twelsby/gtkmmbuild
MAINTAINER Melroy van den Berg <melroy@melroy.org>

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y ninja-build doxygen