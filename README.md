# Network traffic filtering system
The network traffic filtering system consists of three components:

- The controller is a management node that stores policies, processes requests for domain and IP address classification, and accesses external categorization providers.

- Worker is a filtering node that intercepts and filters traffic through DPDK.

- Admin CLI is a console interface for managing policies and enabling/disabling filtering.

The Worker is located between the client's subnet and the central router, intercepts all incoming packets and decides whether to skip or block based on the policies received from the controller.

## Assembly and launch on a test stand

To run a full-fledged test stand with RISC-V virtual machines that emulate the operation of filters and a controller, see the instructions:
[test bench launch](wiki/using_test_stand.md).

## Assembly and launch on real boards


## Policy management

Detailed instructions on how to administer policies via the CLI, a description of the TOML configuration, and a list of categories: [policy management](wiki/admin_client.md).

## Worker-Controller Communication Protocol

The communication protocol between Worker (C++) and Controller (Go) is based on gRPC with Protocol Buffers for message serialization. The interaction is one-way: Worker always acts as client, Controller as server. [Full protocol description](wiki/worker_controller_communication_protocol.md).
