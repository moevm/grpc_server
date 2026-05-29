package manager

import (
	"fmt"
	"log"
	"sync"

	communication "github.com/moevm/grpc_server/pkg/proto/communication"
	"google.golang.org/protobuf/proto"
)

type Manager struct {
	policyManager    *PolicyManager
	filteringEnabled map[uint64]bool
	mu               sync.RWMutex
}

func NewManager() (*Manager, error) {
	return &Manager{
		policyManager:    NewPolicyManager(),
		filteringEnabled: make(map[uint64]bool),
	}, nil
}

func (m *Manager) HandleGetPolicy(workerID uint64, currentVersion uint64) ([]byte, bool, error) {
	log.Printf("Worker %d requested policy", workerID)
	m.mu.Lock()
	if _, exists := m.filteringEnabled[workerID]; !exists {
		m.filteringEnabled[workerID] = true
	}
	m.mu.Unlock()

	m.policyManager.mu.RLock()
	currentPolicyVersion := m.policyManager.version
	m.policyManager.mu.RUnlock()

	if currentPolicyVersion == currentVersion {
		log.Printf("Worker %d already has latest policy", workerID)
		return nil, false, nil
	}

	policyProto := m.policyManager.GetWorkerPolicyProto(workerID)

	policyBytes, err := proto.Marshal(policyProto)
	if err != nil {
		return nil, false, err
	}

	log.Printf("Sending new policy to worker %d", workerID)
	return policyBytes, true, nil
}

func (m *Manager) GetWorkerPolicy(workerID uint64) *communication.WorkerPolicy {
	return m.policyManager.GetWorkerPolicyProto(workerID)
}

func (m *Manager) UpdateConfig(configData []byte) error {
	if m.policyManager == nil {
		return fmt.Errorf("policyManager is nil")
	}
	return m.policyManager.UpdateConfig(configData)
}

func (m *Manager) SetFilteringEnabled(workerID uint64, enabled bool) {
	m.mu.Lock()
	defer m.mu.Unlock()

	if workerID == 0 {
		for id := range m.filteringEnabled {
			m.filteringEnabled[id] = enabled
		}
	} else {
		m.filteringEnabled[workerID] = enabled
	}
	log.Printf("Filtering %s for worker %d",
		map[bool]string{true: "enabled", false: "disabled"}[enabled], workerID)
}

func (m *Manager) IsFilteringEnabled(workerID uint64) bool {
	m.mu.RLock()
	defer m.mu.RUnlock()
	enabled, exists := m.filteringEnabled[workerID]
	if !exists {
		return true
	}
	return enabled
}
