This simple example reproduces an issue encountered when using a
periodic load balancer with Charm++ version 7.

To reproduce the issue:

git clone https://github.com/UIUC-PPL/charm.git

cd charm

./build LIBS multicore-linux-x86_64 --build-shared --enable-randomized-msgq --enable-error-checking --enable-tracing --enable-tracing-commthread --enable-charmdebug -g -O0

make

cd tests/charm++/load_balancing

git clone https://github.com/kidder/charm-periodic-lb.git

cd charm-periodic-lb

make test


The test will randomly fail when using more than one processor for
version 7 or later of Charm++, but pass for version 6.10.2