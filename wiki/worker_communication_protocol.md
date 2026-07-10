## Worker-Controller Communication Protocol
The communication protocol between Worker and Controller is based on gRPC using Protocol Buffers to serialize messages. The interaction is one-way: the Worker always acts as a client, the Controller as a server. The choice of gRPC is justified by the high expected hit rate in the local cache of the Worker, which reduces the frequency of network calls to a minimum.

### Interface Specification

The interaction contract is described in the file `worker/communication.proto`:

```proto
service DataService {
    rpc GetPolicy(GetPolicyRequest) returns (GetPolicyResponse);
    rpc Classify(ClassifyRequest) returns (ClassifyResponse);
}
```

### GetPolicy - getting the filtering policy
Worker requests an up-to-date filtering policy at a specified frequency.

Request
```proto
message GetPolicyRequest {
uint64 worker_id = 1; // Worker ID
    uint64 config_version = 2; // the current configuration version for the Worker
}
```

Answer
```proto
message GetPolicyResponse {
    enum Result {
POLICY_PROVIDED = 0; // policy updated, policy field filled in
        POLICY_UNCHANGED = 1; // the policy is current, the policy field is empty
    }
    Result result = 1;
    WorkerPolicy policy = 2; // full filtering policy
bool filtering_enabled = 3; // filter on/off flag
}
```

The structure of the WorkerPolicy
``` proto
message WorkerPolicy {
repeated string block_categories = 1; // blocked categories (for example, ["Gambling", "Weapons"])
map<string, int32> block_by_trust = 2; // categories with min. the level of trust (for example, {"ENTERTAINMENT": 6})
repeated string block_domains = 3; // blocked domains
    repeated string allow_domains = 4; // allowed domains (priority over blocking)
    repeated string block_ips = 5; // blocked ips (IPv4 and IPv6)
repeated string allow_ips = 6; // allowed ips
    int32 min_trust_level = 7; // global minimum trust level
    int32 ttl_ip = 8; // TTL of the IP cache in seconds
    int32 ttl_domain = 9; // TTL of the domain cache in seconds
    uint64 config_version = 10; // configuration version
of google.protobuf.Struct extra = 11; // additional parameters (arbitrary JSON)
}
```

### Classify - classification of a domain or IP address
Appointment
Worker calls the method when the local cache is missed - when the domain or IP address extracted from the packet is not found in either the domain or IP cache. 

Request
``` proto
message ClassifyRequest {
uint64 worker_id = 1; // Worker ID
    string type = 2; // type of the classified object: "domain" or "ip"
    string target = 3; // string representation of the domain or IP address
}
```

Answer
``` proto
message ClassifyResponse {
    repeated string categories = 1; // list of categories
    int32 trust_level = 2; // minimum level of trust among categories
}
```
