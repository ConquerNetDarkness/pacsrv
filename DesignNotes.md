# Introduction #

When designing this project, we have to make decisions on how it should do things.  These are governed by the goals, but also by the technical ability to perform certain things.


# Why use a server/client? #

When pacman performs its operations, it downloads each file singly, one at a time.  Therefore, our pacman interface needs to do that as well.  If we used a single client that only exists while actually downloading the app, that seriously limits the amount of activity that can happen during that time, and most clients will only be able to talk while other clients are actively downloading as well (which might only take a second or two).

By having a server component, we can support the other clients (the nodes) that are trying to find the packages, even when we arent actively trying to download packages.