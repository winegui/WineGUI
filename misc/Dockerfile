FROM danger89/cmake:4.0

RUN apt-get update \
 && apt-get install -y libgtkmm-3.0-dev curl libcurl4-openssl-dev \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
