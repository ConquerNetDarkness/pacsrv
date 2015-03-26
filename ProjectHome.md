pacman is one of the most essential tools in ArchLinux, similar to aptget in debian, and the others that almost every linux distribution has.

One of its failings is that it gets all packages from a series of mirror servers.  This situation works, but does not make it easy to dynamically download packages from the fastest server.

This project aims to provide a p2p layer that allows packages to be retrieved from a large distribution of users and servers.

An advantages from businesses on a LAN is that a single package server can be used as a proxy/cache for accessing the packages from outside of the LAN.  This reduces downloading and speeds up internal package updates from multiple installations.