### What is this repository?
This repository contains Tiangram, an API REST-based message application that is in developement. The objective of this REST application is to explote C++'s efficiency to offer a secure message application with almost no dependencies. Also to let you launch instantly a centralized messaging server based on a monolithic C++ code. 

### TODO
1. Implement SQLite as DB that stores important data (Chats are stored in a specialized file).
2. Eliminate asynchronous dependencies such as Write After Write, Read After Write...
3. Ensure there are no data leaks.
4. Add a cookies mechanism to store sessions.  
5. Jump from HTTP to HTTPS.