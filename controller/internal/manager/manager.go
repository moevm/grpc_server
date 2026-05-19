package manager

import (
	"fmt"
	"log"

	communication "github.com/moevm/grpc_server/pkg/proto/communication"
	"google.golang.org/protobuf/proto"
)

type Manager struct {
	policyManager *PolicyManager
}

func NewManager() (*Manager, error) {
	return &Manager{
		policyManager: NewPolicyManager(),
	}, nil
}

func (m *Manager) HandleGetPolicy(workerID uint64, currentVersion uint64) ([]byte, bool, error) {
	log.Printf("Worker %d requested policy", workerID)

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