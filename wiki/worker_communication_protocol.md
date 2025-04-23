# Worker-Controller communication protocol

## Worker registration

* The controller listens for a connection on the unix-socket `/run/controller/init.sock`.
* When the connection is established, the worker sends its identifier (int64) via this socket to the controller.
* Controller sends a response (int64). The worker receives it. "1" indicates that registration was successful. Otherwise, registration did not occur and there will be no further communication between worker and controller.
* If worker is successfully registered, the connection is closed by controller and further communication will occur on the socket `/run/controller/WORKER_ID.sock`.

## Task receiving

The worker's work cycle looks like this:

* The worker listens for a connection on the socket `/run/controller/WORKER_ID.sock`
* The controller transfers task in a certain format via this socket (format is not restrcted by anything, only requirement is it should be the same on both sides to successfully communicate).
* The worker transfers a solution via this socket in a certain format (format is not restrcted by anything, only requirement is it should be the same on both sides to successfully communicate).
* The controller closes the connection.
