## Overview:
Distribicom is a Cmake project that uses grpc and SEAL as dependencies.

## Project Structure:
 All source code is contained in src.
 `src\concurrency` contains concurrency primitives for application logic
 `src\internals` contains `.proto` files used to define the client-server communication API
 `src\marshal` contains source code for marshaling objects that are sent between the client and server
 `src\math_utils` contains a low level math library over seal objects that implements functionality necessary for PIR and verification. The library is optimised for multithreaded use.
 `src\services` contains the application logic. see description below



- worker:
    receives chunks of completed work from workers.
    then unifies the work

- work distributer component:
  num workers, num rows in DB.
  Due to the caching - we need to have the workers hold ALL queries (side effect of reducing amount of computations on the server).

- manager + server
   networking layer which knows how to distribute the work to all components.
    has access to:
        to verifer.
        to work-distributer.
    receive processed work on multiple threads -> forward to verifier