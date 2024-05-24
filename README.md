# eos_final
This is the repo for eos final project.
```
Start Server
    |
    v
Initialize semaphore and mutex
    |
    v
Create robot arm threads
    |
    v
Server Listen for client connections
    |
    v
+--------------------------------+
| Client Connect and send number |
|    of objects to pick          |
+--------------------------------+
    |
    v
Receive command from client
    |
    v
Update task: Set objects_to_pick
and reset objects_picked and
objects_placed
    |
    v
Start picking process (signal sem_pick)
    |
    v
+-----------------------------------------+
|  Robot Arm Threads Operation (Loop)     |
+-----------------------------------------+
    |
    v
Acquire semaphore sem_pick to pick object (Arm 1)
    |
    v
Pick object and increment objects_picked
    |
    v
Release semaphore sem_place to place object (Arm 2)
    |
    v
Acquire semaphore sem_place to place object (Arm 2)
    |
    v
Place object and increment objects_placed
    |
    v
Release semaphore sem_pick for next pick (Arm 1)
    |
    v
Check if task is complete inside loop
    |
    v
End Connection if task complete
    |
    v
Wait for new client connection

```
