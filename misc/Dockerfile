FROM danger89/cmake:4.4

RUN apt-get update \
 && apt-get install -y libgtkmm-3.0-dev curl libcurl4-openssl-dev xvfb \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
