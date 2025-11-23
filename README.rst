-------------------------------------
ft_irc
-------------------------------------
A simple IRC server developed in C++
-------------------------------------

Introduction
............

Another educational project from my work at `42 Urduliz <https://www.42urduliz.com/>`_. The objective was to create an IRC Server in C++ (98) which could support a limited set of modes and local operation.

.. WARNING:: This is an education project. It would be reckless to use it for serious work, though I am pretty sure it can't do a *lot* of harm. I may try it on my home network for curiosity.

What I learned in this project
..............................

Key learning outcomes for me in this project include the following.

* Network connection handling in Linux.
* Using the epoll mechanism to avoid blocking in I/O operations.
* The IRC protocol[#]_ . A simple structure leading to a lot of complexity in interactions.
* Object-oriented design.

.. [#] Many thanks to the people behind `this rewriting of the IRC protocol <https://modern.ircdocs.horse/>`_ which we were able to use as a guide.

Notable features / achievements
...............................

During evaluation it withstood several underhand attempts to make it crash (e.g. using binary input and ASCII control codes).

The ACommand abstract class from which all the commands (KICK, JOIN, etc) inherit was a key breakthrough that allowed us to avoid mind-melting repetition in the main Server loop. It reduced to 3 essential functions:

- check the command parameters
- run the command
- add the command output to the Server's queue.

This meant that all the complex logic (e.g. in MODE) was encapsulated and abstracted away, allowing us to divide the tasks and providing the basis for easy extension of the Server by adding new supported commands (e.g. OPER was under consideration but deemed out-of-scope).

Another approach to increasing readability of the code and enabling quicker expansion of features was the way we handled numeric replies. There are 100s of these in the protocol. We implemented them as enums so that when coding a command we could call ``_reply(msg, ERR_NEEDMOREPARAMS)`` and understand at a glance what the code was intended to do.

Finally, not an achievement but something that made me proud, was that an early version of this managed to crash clients when it sent replies to them. Obviously this is naughty but to see longstanding software have the same kind of issues that you work so hard to avoid is an excellent inoculation against impostor syndrome.

Out of scope / remaining limitations
....................................

* Only a limited set of modes were required, though the scaffolding exists to add more.
* We created a *server* not a *client* and were only assessed on one single reference client (we used `Hexchat <https://hexchat.github.io/>`_, though during development I also tested with `Konversation <https://konversation.kde.org/>`_ and `KVIrc <https://kvirc.net/>`_).
* There was to be no Server-to-Server communication which simplified some things greatly. In designing the Server call I hoped to avoid any assumptions that would limit the ability to extend in this direction, e.g. assigning space to record locations, but I would expect it to be much harder to extend this aspect.
* There was no persistence of rooms or users across reboots.
* The code remains single threaded.
  We didn't have time to experiment with spinning off threads to see if it improved the responsiveness or capacity of the server. If we did, I would have first tried creating a thread for each ACommand that we execute in Server::_processQueue(). The potential for operations getting out of order would make this a potentially "interesting" problem, though, especially around channel and user management.

Conclusion
..........

Despite it being an old protocol, IRC still has some appeal. We showed that it is possible to code a reasonable server for it in a couple of months, so it is clearly pretty approachable compared with more "modern" protocols. It seems well-suited for this kind of object oriented implementation, too.
