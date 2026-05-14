package manager

import (
	"fmt"
	"log"
	"sync"
	"time"

	communication "github.com/moevm/grpc_server/pkg/proto/communication"
	"google.golang.org/protobuf/proto"
)

type Manager struct {
	policyManager *PolicyManager
	statsStore    map[uint64]*WorkerStats // Хранилище статистики воркеров
	statsMutex    sync.RWMutex
}

type WorkerStats struct {
	LastUpdate      time.Time
	PacketsReceived uint64
	PacketsPassed   uint64
	PacketsDropped  uint64
	TotalBlocked    uint64
	TotalAllowed    uint64
	Resources       map[string]*ResourceStats // Статистика по доменам/IP
}

type ResourceStats struct {
	Domain  string
	Blocked uint64
	Allowed uint64
}

func NewManager() (*Manager, error) {
	return &Manager{
		policyManager: NewPolicyManager(),
		statsStore:    make(map[uint64]*WorkerStats),
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

func (m *Manager) HandleStatsReport(report *communication.StatsReport) error {
	m.statsMutex.Lock()
	defer m.statsMutex.Unlock()

	log.Printf("Processing stats from worker %d: received=%d passed=%d dropped=%d",
		report.WorkerId,
		report.PacketsReceived,
		report.PacketsPassed,
		report.PacketsDropped)

	stats, exists := m.statsStore[report.WorkerId]
	if !exists {
		stats = &WorkerStats{
			Resources: make(map[string]*ResourceStats),
		}
		m.statsStore[report.WorkerId] = stats
	}

	stats.LastUpdate = time.Unix(int64(report.Time), 0)
	stats.PacketsReceived = report.PacketsReceived
	stats.PacketsPassed = report.PacketsPassed
	stats.PacketsDropped = report.PacketsDropped
	stats.TotalBlocked = report.TotalBlocked
	stats.TotalAllowed = report.TotalAllowed

	for _, resource := range report.Resources {
		if _, ok := stats.Resources[resource.Domain]; !ok {
			stats.Resources[resource.Domain] = &ResourceStats{
				Domain: resource.Domain,
			}
		}
		stats.Resources[resource.Domain].Blocked = resource.Blocked
		stats.Resources[resource.Domain].Allowed = resource.Allowed
	}

	log.Printf("Stats saved for worker %d", report.WorkerId)
	return nil
}

func (m *Manager) GetWorkerStats(workerID uint64) (*WorkerStats, bool) {
	m.statsMutex.RLock()
	defer m.statsMutex.RUnlock()
	stats, ok := m.statsStore[workerID]
	return stats, ok
}

func (m *Manager) GetAllStats() map[uint64]*WorkerStats {
	m.statsMutex.RLock()
	defer m.statsMutex.RUnlock()

	result := make(map[uint64]*WorkerStats)
	for k, v := range m.statsStore {
		result[k] = v
	}
	return result
}

func (m *Manager) GetAggregatedStats() *WorkerStats {
	m.statsMutex.RLock()
	defer m.statsMutex.RUnlock()

	aggregated := &WorkerStats{
		Resources: make(map[string]*ResourceStats),
	}

	for _, stats := range m.statsStore {
		aggregated.PacketsReceived += stats.PacketsReceived
		aggregated.PacketsPassed += stats.PacketsPassed
		aggregated.PacketsDropped += stats.PacketsDropped
		aggregated.TotalBlocked += stats.TotalBlocked
		aggregated.TotalAllowed += stats.TotalAllowed

		for domain, resource := range stats.Resources {
			if _, ok := aggregated.Resources[domain]; !ok {
				aggregated.Resources[domain] = &ResourceStats{Domain: domain}
			}
			aggregated.Resources[domain].Blocked += resource.Blocked
			aggregated.Resources[domain].Allowed += resource.Allowed
		}
	}

	aggregated.LastUpdate = time.Now()
	return aggregated
}