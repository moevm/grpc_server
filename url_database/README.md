# Research URL databases

## Kaspersky Threat Intelligence Portal

Free subscribe:
 * Daily quota: 2000 lookups / day

### Search domain

Request method: `GET`
Endpoint: `https://opentip.kaspersky.com/api/v1/search/domain`

Headers:
* `x-api-key`: token

Params:
* `request`: domain

Successful server response:

| Field | Type | Description |
|-------|------|-------------|
| Zone | string | The color zone of the domain. Available values: Red – the domain contains malicious objects and can be classified as a Dangerous object. Orange – the domain can be classified as Untrusted and contain malicious objects. Yellow – the domain is classified as Advertising and other programs. Grey – there is no or insufficient information to classify the domain. Green – the domain has the status Safe object or No threats detected. |
| DomainGeneralInfo | object | General information about the requested domain. |
| FilesCount | integer | Number of known malicious files. |
| UrlsCount | integer | Number of known malicious web addresses. |
| HitsCount | integer | Number of IP addresses related to the domain. |
| Domain | string | Name of the requested domain. |
| Ipv4Count | integer | Number of IP addresses (IPv4) related to the requested domain. |
| Categories | array of strings | Categories of the requested domain. |
| CategoriesWithZone | array of objects | Categories of the requested domain and the zones to which these categories belong. |
| DomainWhoIsInfo | object | WHOIS information for the requested domain. |
| DomainName | string | Name of the requested domain. |
| Created | string | Registration date of the requested domain. |
| Updated | string | Date of last update of the requested domain registration data. |
| Expires | string | Expiration date of the requested domain. |
| NameServers | array of strings | Name servers of the requested domain. |
| Contacts | array of strings | Contact information of the requested domain owner. |
| Registrar | object | Information about the registrar of the requested domain. |
| DomainStatus | array of strings | Statuses of the requested domain. |
| RegistrationOrganization | string | Name of the registering organization. |

### Search IP

Endpoint: `https://opentip.kaspersky.com/api/v1/search/ip`

Everything else is the same as for "Search domain"

## SkyNS

Free subscribe:
* 10 free requests per minute

### Search domain

Request method: `GET`
Endpoint: `http://{site}/domain/{domain}`
where `site`:
 * `z.api.skydns.ru` - if user is not authorized, 10 free requests per minute
 * `x.api.skydns.ru` - if user is authorized, unlimited requests

Authorization
For requests to x.api.skydns.ru, Basic Authorization must be used.
A special HTTP Authorization header must be passed in each request.
The Authorization header contains the string <client_id>:<client_secret>,
encoded using the base64 method.
The Basic authorization method should be specified.

Successful response:

| Field | Type | Description |
|-------|------|-------------|
| category | array of integers | Site category codes: 3 - "Virus distributing sites", 6 - "Drugs" |
| bad | boolean | Flag indicating whether the site is malicious/dangerous (true - dangerous, false - safe) |
| category_name | array of strings | Human-readable category names in Russian language |

## VirusTotal

Free subscribe:
* Request rate: 4 lookups / min
* Daily quota: 500 lookups / day
* Monthly quota: 15.5 K lookups / month

### Search domain

Request method: `GET`
Endpoint: `https://www.virustotal.com/api/v3/domains/{domain}`

Headers:
 * `accept`: `application/json`

Successful response:

| Field | Type | Description |
|-------|------|-------------|
| categories | dictionary | Mapping that relates categorisation services with the category it assigns the domain to. These services are, among others: Alexa, BitDefender, TrendMicro, Websense ThreatSeeker, etc. |
| creation_date | integer | Creation date extracted from the Domain's whois (UTC timestamp). |
| favicon | dictionary | Dictionary including difference hash and md5 hash of the domain's favicon. Only available for premium users. |
| dhash | string | Difference hash |
| raw_md5 | string | Favicon's MD5 hash. |
| jarm | string | Domain's JARM hash. |
| last_analysis_date | integer | UTC timestamp representing last time the domain was scanned. |
| last_analysis_results | dictionary | Result from URL scanners. dict with scanner name as key and a dict with notes/result from that scanner as value. |
| category | string | Normalised result. Can be: "harmless" (site is not malicious), "undetected" (scanner has no opinion about this site), "suspicious" (scanner thinks the site is suspicious), "malicious" (scanner thinks the site is malicious). |
| engine_name | string | Complete name of the URL scanning service. |
| engine_version | string | Engine version value, in case it reports that data. |
| method | string | Type of service given by that URL scanning service (i.e. "blacklist"). |
| result | string | Raw value returned by the URL scanner ("clean", "malicious", "suspicious", "phishing"). It may vary from scanner to scanner, hence the need for the "category" field for normalisation. |
| last_analysis_stats | dictionary | Number of different results from this scans. |
| harmless | integer | Number of reports saying that is harmless. |
| malicious | integer | Number of reports saying that is malicious. |
| suspicious | integer | Number of reports saying that is suspicious. |
| timeout | integer | Number of timeouts when checking this URL. |
| undetected | integer | Number of reports saying that is undetected. |
| last_dns_records | list of dictionaries | Domain's DNS records on its last scan. Every entry contains: expire (integer), flag (integer), minimum (integer), priority (integer), refresh (integer), rname (string), retry (integer), serial (integer), tag (string), ttl (integer), type (string), value (string). |
| last_dns_records_date | integer | Date when the DNS records list was retrieved by VirusTotal (UTC timestamp). |
| last_https_certificate | SSL Certificate | SSL Certificate object retrieved last time the domain was analysed. |
| last_https_certificate_date | integer | Date when the certificate was retrieved by VirusTotal (UTC timestamp). |
| last_modification_date | integer | Date when any of domain's information was last updated. |
| last_update_date | integer | Updated date extracted from whois (UTC timestamp). |
| popularity_ranks | dictionary | Domain's position in popularity ranks such as Alexa, Quantcast, Statvoo, etc. Each entry contains: rank (integer), timestamp (integer UTC timestamp when the rank was ingested). |
| registrar | string | Company that registered the domain. |
| reputation | integer | Domain's score calculated from the votes of the VirusTotal's community. |
| tags | list of strings | List of representative attributes. |
| total_votes | dictionary | Unweighted number of total votes from the community, divided in "harmless" and "malicious": harmless (integer number of positive votes), malicious (integer number of negative votes). |
| whois | string | Whois information as returned from the pertinent whois server. |
| whois_date | integer | Date of the last update of the whois record in VirusTotal. |

### Search IP

Request method: `GET`
Endpoint: `https://www.virustotal.com/api/v3/ip_addresses/{ip}`

Headers:
 * `accept`: `application/json`

Successful response:

| Field | Type | Description |
|-------|------|-------------|
| as_owner | string | Owner of the autonomous system to which the IP belongs. |
| asn | integer | Autonomous system number to which the IP belongs. |
| continent | string | Continent where the IP is located (ISO-3166 continent code). |
| country | string | Country where the IP is located (ISO-3166 country code). |
| jarm | string | JARM hash of the IP address. |
| last_analysis_date | integer | UTC timestamp representing the last time the IP address was scanned. |
| last_analysis_results | dictionary | Results from URL scanners. Dictionary with scanner name as key and a dictionary with notes/results from that scanner as value. |
| category | string | Normalized result. Can be: "harmless" (site is not malicious), "undetected" (scanner has no opinion about this site), "suspicious" (scanner thinks the site is suspicious), "malicious" (scanner thinks the site is malicious). |
| engine_name | string | Full name of the URL scanning service. |
| method | string | Type of service provided by that URL scanning service (i.e. "blacklist"). |
| result | string | Raw value returned by the URL scanner ("clean", "malicious", "suspicious", "phishing"). May vary from scanner to scanner, hence the need for the "category" field for normalization. |
| last_analysis_stats | dictionary | Number of different results from this scan. |
| harmless | integer | Number of reports saying it is harmless. |
| malicious | integer | Number of reports saying it is malicious. |
| suspicious | integer | Number of reports saying it is suspicious. |
| timeout | integer | Number of timeouts when checking this URL. |
| undetected | integer | Number of reports saying it is undetected. |
| last_https_certificate | SSL Certificate | SSL Certificate object information for this IP address. |
| last_https_certificate_date | integer | Date when the certificate shown in last_https_certificate was retrieved by VirusTotal. UTC timestamp. |
| last_modification_date | integer | Date when any IP address information was last updated. UTC timestamp. |
| network | string | IP network range to which the IP belongs. |
| regional_internet_registry | string | RIR (one of the current RIRs: AFRINIC, ARIN, APNIC, LACNIC or RIPE NCC). |
| reputation | integer | IP score calculated from the votes of the VirusTotal community. |
| tags | list of strings | Identifying attributes. |
| total_votes | dictionary | Unweighted total number of votes from the community, divided into "harmless" and "malicious". |
| harmless | integer | Number of positive votes. |
| malicious | integer | Number of negative votes. |
| whois | string | Whois information as returned from the pertinent whois server. |
| whois_date | integer | Date of the last update of the whois record in VirusTotal. UTC timestamp. |

## Yandex Safe Browsing
Free subscribe:
* Daily quota: available with Yandex.Webmaster API key

Request method: GET
Endpoint: https://sba.yandex.net/v4/threatLists?key={API_KEY}

Successful response:
| Field | Type | Description |
|-------|------|-------------|
| threatLists | array | List of available threat lists |
| threatType | string | Type of threat. Values: `MALWARE`, `SOCIAL_ENGINEERING`, `UNWANTED_SOFTWARE`, `POTENTIALLY_HARMFUL_APPLICATION`, `THREAT_TYPE_KIDS_MODE`, `THREAT_TYPE_TURBO_APPS` |
| platformType | string | Platform affected by the threat. Values: `WINDOWS`, `LINUX`, `ANDROID`, `OSX`, `IOS`, `CHROME`, `ANY_PLATFORM`, `ALL_PLATFORMS` |
| threatEntryType | string | Type of object representing the threat. Values: `URL`, `EXECUTABLE`, `CHROME_EXTENSION` |


Conclusion:
For website category detection (Gambling, Games, Adult, etc.), the following services are recommended:

Kaspersky Threat Intelligence Portal — high accuracy, color-coded threat zones, 2,000 requests/day
VirusTotal — aggregates data from 70+ antivirus engines, detailed statistics, 500 requests/day
SkyNS — Russian database with categories in Russian language, 10 requests/minute
For website security checks (reputation, malware, phishing), it is advisable to use:

Google Safe Browsing — authoritative source, used in Chrome browser
Yandex Safe Browsing — relevant for Russian-language sites, accounts for local specifics
Thus, for comprehensive analysis, it is recommended to combine both approaches:

Kaspersky + VirusTotal + SkyNS — for category classification
Google + Yandex — for security and reputation assessment


