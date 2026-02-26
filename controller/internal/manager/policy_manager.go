package manager

import (
	"crypto/sha256"
	"encoding/binary"
	"fmt"
	"log"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/structpb"
	"github.com/pelletier/go-toml"
	pb "github.com/moevm/grpc_server/pkg/proto/communication"
)

type TOMLRules struct {
	BlockCategories []string          `toml:"block_categories"`
	BlockByTrust    map[string]int32  `toml:"block_by_trust"`
	BlockDomains    []string          `toml:"block_domains"`
	AllowDomains    []string          `toml:"allow_domains"`
	MinTrustLevel   int32             `toml:"min_trust_level"`
	Extra map[string]interface{} 	  `toml:",remain"`
}

type TOMLConfig struct {
	Global struct {
		Rules TOMLRules `toml:"rules"`
	} `toml:"global"`
	Filters map[string]TOMLRules `toml:"filters"`
}

type PolicyManager struct {
	config  *TOMLConfig
	version uint64
}

func NewPolicyManager() *PolicyManager {
    return &PolicyManager{
		version: 0,
        config:  &TOMLConfig{},
    }
}

func computeHash(policy *pb.WorkerPolicy) uint64 {
	data, _ := proto.Marshal(policy)
	hash := sha256.Sum256(data)
	return binary.BigEndian.Uint64(hash[:8])
}

func (pm *PolicyManager) GetWorkerPolicyProto(workerID uint64) *pb.WorkerPolicy {
	if pm.config == nil {
		return &pb.WorkerPolicy{}
	}

	policy := &pb.WorkerPolicy{
		BlockCategories: make([]string, len(pm.config.Global.Rules.BlockCategories)),
		BlockByTrust:    make(map[string]int32, len(pm.config.Global.Rules.BlockByTrust)),
		BlockDomains:    make([]string, len(pm.config.Global.Rules.BlockDomains)),
		AllowDomains:    make([]string, len(pm.config.Global.Rules.AllowDomains)),
		MinTrustLevel:   pm.config.Global.Rules.MinTrustLevel,
	}
	copy(policy.BlockCategories, pm.config.Global.Rules.BlockCategories)
	for k, v := range pm.config.Global.Rules.BlockByTrust {
		policy.BlockByTrust[k] = v
	}
	copy(policy.BlockDomains, pm.config.Global.Rules.BlockDomains)
	copy(policy.AllowDomains, pm.config.Global.Rules.AllowDomains)

	filterName := fmt.Sprintf("filter_%d", workerID)
	if filter, ok := pm.config.Filters[filterName]; ok {
		if len(filter.BlockCategories) > 0 {
			existing := make(map[string]bool)
			for _, cat := range policy.BlockCategories {
				existing[cat] = true
			}
			for _, cat := range filter.BlockCategories {
				if !existing[cat] {
					policy.BlockCategories = append(policy.BlockCategories, cat)
				}
			}
		}
		
		for k, v := range filter.BlockByTrust {
			policy.BlockByTrust[k] = v
		}
		
		if len(filter.BlockDomains) > 0 {
			existing := make(map[string]bool)
			for _, d := range policy.BlockDomains {
				existing[d] = true
			}
			for _, d := range filter.BlockDomains {
				if !existing[d] {
					policy.BlockDomains = append(policy.BlockDomains, d)
				}
			}
		}
		
		if len(filter.AllowDomains) > 0 {
			existing := make(map[string]bool)
			for _, d := range policy.AllowDomains {
				existing[d] = true
			}
			for _, d := range filter.AllowDomains {
				if !existing[d] {
					policy.AllowDomains = append(policy.AllowDomains, d)
				}
			}
		}
		if filter.MinTrustLevel != 0 {
			policy.MinTrustLevel = filter.MinTrustLevel
		}

		if len(filter.Extra) > 0 {
			if extraStruct, err := structpb.NewStruct(filter.Extra); err == nil {
				policy.Extra = extraStruct
			}
		} else {
			if len(pm.config.Global.Rules.Extra) > 0 {
				if extraStruct, err := structpb.NewStruct(pm.config.Global.Rules.Extra); err == nil {
					policy.Extra = extraStruct
				}
			}
		}
	}
	policy.PolicyHash = computeHash(policy)
	return policy
}

func (pm *PolicyManager) UpdateConfig(configData []byte) {
	var cfg TOMLConfig
	if err := toml.Unmarshal(configData, &cfg); err != nil {
		log.Printf("Failed to parse TOML in UpdateConfig: %v", err)
		return
	}
	pm.config = &cfg
	pm.version++
	log.Printf("Config updated to version %d", pm.version)
}
