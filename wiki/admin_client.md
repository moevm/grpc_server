# Admin Client

## Installing dependencies
```bash
pip install -r requirements.txt
```

## Proto file generation
```bash
python -m grpc_tools.protoc \
    --python_out=. \
    --grpc_python_out=. \
    -I . \
    admin_service.proto
```

## Launching the controller
```bash
cd ../controller
bazel run //cmd/grpc_server:grpc_server
```

## Example of a toml file with a policy
```toml
[global.rules]                                   # Global rules for all workers
block_categories = ["Gambling", "Weapons"]       # Categories to block
block_domains = ["youtube.com", "tiktok.com"]    # Domains to block
allow_domains = ["github.com ", "stackoverflow.com "] # Allowed domains (priority over blocking)
block_ips = ["192.168.0.1", "2001:0db8:85a3:0000:0000:8a2e:0370:7334"] # Blocking IP (IPv4 and IPv6)
allow_ips = ["8.8.8.8"] # Allowed IP 
ttl_ip = 604800 # TTL of the IP cache 
ttl_domain = 604800 # TTL of the domain cache 
min_trust_level = 5 # Min. the level of trust 

[global.rules.block_by_trust] # Blocking by trust level
ENTERTAINMENT = 6                                 
NEWS = 4                                          

[filters.filter_1] # Rules for worker #1
block_categories = ["Weapons", "Malware"]         # Additional categories to global ones
block_domains = ["instagram.com"]                 # Additional domains to global
allow_domains = ["vk.com "]                        # Additional allowed domains
min_trust_level = 0 # Overrides the global 

[filters.filter_1.block_by_trust] # Trust levels for the worker #1
SOCIAL = 8                                       
ENTERTAINMENT = 7                                

[filters.filter_2] # Rules for the worker #2
block_categories = ["Malware"]                    # Additional categories to global
allow_domains = ["github.com ", "gitlab.com "]      # Additional allowed domains

```

### Possible categories
- Adult Content (Pornography and adult entertainment)
- Gambling (Online casinos, betting, lotteries)
- Drugs (Illegal substances)
- Violence (Violent content)
- Weapons (Firearms, explosives)
- Malware (Viruses and malicious software)
- Social media 
- Hate Speech (Discrimination, extremism)
- Anonymizers (VPN, proxies to bypass blocks)
- Online shop


## Launching the client
### Send a new policy to the controller

```bash
python admin.py load --file <your_policy>.toml
```

### Get the current policy from the controller

```bash
python admin.py get --save <your_policy>.toml
```

### Enable/disable filtering on the worker

```bash
python admin.py toggle --id 1 --on  #id - worker id
python admin.py toggle --id 1 --off #id - worker id
```


## Description of the policy application from the config
The policy for each filter is formed by combining global rules and individual settings of a specific filter.

### Global level 
First, the shared TOML configuration file is loaded. It has a [global.rules] section that contains rules that apply to all filters without exception.:
- Which categories of sites are always blocked
- Which domains and ip addresses are blacklisted
- Which domains and ip addresses are on the whitelist
- Trust thresholds for different categories
- Minimum trust level by default
- Cache lifetime for ip and domain

These rules form a global policy that is the same for all filters.

### Local level
There is a [filters] section in the same file where you can set your own rules for each filter, for example, [filters.filter_1], [filters.filter_2], etc.

Local rules do not replace global ones, but complement and clarify them.:
- If the local rules specify additional categories for blocking, they are added to the global ones.
- If additional domains or ip addresses are specified, they are added to the corresponding lists.
- Trust thresholds for categories can be redefined (if a new value is specified for the same category)
- The minimum trust level can be changed for a specific filter

### Final policy
When the filter requests a policy, the controller does the following:
1. Takes global rules as a basis
2. Imposes local rules of a specific filter on top of them.
3. Combines lists (adds new items without deleting old ones)
4. Overwrites individual parameters if they are set locally.

Thus, each filter receives an individual policy that inherits the general rules, but may be stricter or more lenient depending on the settings of its network segment.
