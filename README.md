# eos_final
This is the repo for eos final project.
```
+-------------------+
|       main        |
+-------------------+
                   |
                   v
+-------------------+
|  signal(SIGINT)   | --->  sigint_handler (cleanup and exit)
+-------------------+
                   |
                   v
+-------------------+
| initialize robots |
+-------------------+
                   |
                   v
+-------------------+
|  create semaphores|
+-------------------+
                   |
                   v
+-------------------+
|  create socket    |
+-------------------+
                   |
                   v
+-------------------+
| listen for client |
+-------------------+
                   |
                   v
+-------------------+
| accept client     |
+-------------------+
                   |
                   v (client connected)
+-------------------+
| create new thread |
+-------------------+
                   |
                   v
+-------------------+
| command_receiver  |
+-------------------+
                   |
                   v
+-------------------+
| receive command   |
+-------------------+
                   |
                   v (client command received)
+--------------------+
|P(sem_multi_clients)| ---> acquire semaphore (one client at a time)
+--------------------+
                   |
                   v (client has semaphore)
+---------------------------+
|  client_command==END_CMD? |
+---------------------------+
                     |
                     v (END_CMD)
+--------------------+
|  break             | ---> exit thread
+--------------------+
                    |
                    v (not END_CMD)
+-------------------+
|  P(sem_stack)     | ---> acquire semaphore (stack access)
+-------------------+
                    |
                    v (stack semaphore acquired)
+----------------------------------+
|  current_stack < MAX_stack_size? |
+----------------------------------+
                   |
                   v (stack full)
+---------------------+
|  current_stack = 0  | ---> reset stack pointer
+---------------------+
                   |
                   v (stack not full)
+---------------------+
|  V(sem_stack)       | ---> release semaphore (stack access)
+---------------------+
                   |
                   v (stack command decoding)
+--------------------------+
|  switch (client_command) |
+--------------------------+
                   |
                   v (case 0)
+----------------------+
|  do client_command 0 |
+----------------------+
                   |
                   v (case 1)
+---------------------+
|  create threads (1) |   | (create pick_place threads)
+---------------------+   |
                          v (threads created)
+-------------------+
|  join threads     |   | (wait for threads to finish)
+-------------------+   |
                        v
+-----------------------+
|  V(sem_multi_clients) | ---> release semaphore (client processing done)
+-----------------------+
                   |
                   v (case 2)
+---------------------+
|  create threads (2) |   | (create pick_place threads)
+---------------------+   |
                          v (threads created)
+--------------------+
|  join threads      |   | (wait for threads to finish)
+--------------------+   |
                         v
+-----------------------+
|  V(sem_multi_clients) | ---> release semaphore (client processing done)
+-----------------------+
                   |
                   v (case 3)
+---------------------+
|  create threads (3) |   | (create pick_place threads)
+---------------------+   |
                          v (threads created)
+--------------------+
|  join threads      |   | (wait for threads to finish)
+--------------------+   |
                         v
+-----------------------+
|  V(sem_multi_clients) | ---> release semaphore (client processing done)
+-----------------------+
                   |
                   v (default)
+-------------------+
|  do nothing       |
+-------------------+
                   |
                   v
+-------------------+
|  close client     |
+-------------------+
                   |
                   v
+-------------------+
|  exit thread      |
+-------------------+
                   |
                   v (client disconnected)

```
