// Package manager implements a client-server model application
// for solving various tasks (currently hash counting) and managing a cluster of workers.
//
// To start the server, simply run the ClusterInit() function.
//
// Before starting, you need to make sure that directory /run/controller/ exists
// and has the correct owner and write/read permissions (this is necessary to create sockets).
//
// All output and errors are logged by [log] package.
//
// [log]: https://pkg.go.dev/log#Logger
package manager

import (
	"fmt"
	"log"
	"net"
	"os"
	"path/filepath"
	"sync"
	"sync/atomic"
	"time"

	"github.com/moevm/grpc_server/internal/conn"
	"google.golang.org/protobuf/proto"

	communication "github.com/moevm/grpc_server/pkg/proto/communication"
)

type WorkerState int

const (
	WorkerBusy WorkerState = iota
	WorkerFree
	WorkerBooting
	WorkerFetch
)

const (
	workerMainSocketPath = "/run/controller/main.sock"
	workerSocketPath     = "/run/controller/"
)

type IManager interface {
	AddTask(taskData []byte, receiver *chan TaskSolution) uint64
	GetTaskSolution() TaskSolution
}

type Manager struct {
	listener net.Listener

	workers      map[uint64]*Worker
	workersMutex sync.Mutex
	workerId     uint64

	tasks      map[uint64]*Task
	tasksMutex sync.Mutex
	taskId     uint64

	freeWorkers   chan uint64
	fetchWorkers  chan uint64
	queuedTasks   chan uint64
	taskSolutions chan TaskSolution
	errorChan     chan error

	shutdown     chan struct{}
	shutdownOnce sync.Once
}

// Task is a general representation of tasks coming to the controller.
// Can be changed if it is necessary to add new info for the Task
// for example: hashType.
type Task struct {
	id       uint64
	body     []byte
	receiver *chan TaskSolution
}

type TaskSolution struct {
	Id   uint64
	Body []byte
}

// Worker contains everything necessary for communication with the worker and his current state.
type Worker struct {
	state     WorkerState
	lastPulse time.Time
	nextPulse time.Duration
	taskId    uint64
}

// Constructor for Task.
func NewTask(taskId uint64, taskBody []byte, receiver *chan TaskSolution) *Task {
	return &Task{
		id:       taskId,
		body:     taskBody,
		receiver: receiver,
	}
}

func sendControlMessage(workerID uint64, msg *communication.ControlMsg, extraData []byte) (*communication.WorkerResponse, []byte, error) {
	msg.ExtraSize = uint64(len(extraData))
	log.Printf("Send control message to worker %v: {%v}\n",
		workerID, msg.String())

	socketPath := fmt.Sprintf("%s%d.sock", workerSocketPath, workerID)
	netConn, err := net.Dial("unix", socketPath)
	if err != nil {
		return nil, nil, fmt.Errorf("dial worker socket: %v", err)
	}
	defer netConn.Close()

	conn := conn.Unix{Conn: netConn}

	msgData, err := proto.Marshal(msg)
	if err != nil {
		return nil, nil, fmt.Errorf("marshal ControlMsg: %v", err)
	}

	if err := conn.WriteMessage(msgData); err != nil {
		return nil, nil, fmt.Errorf("write ControlMsg: %v", err)
	}

	if msg.ExtraSize > 0 {
		if _, err := conn.Conn.Write(extraData); err != nil {
			return nil, nil, fmt.Errorf("write extra data: %v", err)
		}
	}

	respData, err := conn.ReadMessage()
	if err != nil {
		return nil, nil, fmt.Errorf("read WorkerResponse: %v", err)
	}

	resp := &communication.WorkerResponse{}
	if err := proto.Unmarshal(respData, resp); err != nil {
		return nil, nil, fmt.Errorf("unmarshal WorkerResponse: %v", err)
	}

	var respExtra []byte
	if resp.ExtraSize > 0 {
		respExtra = make([]byte, resp.ExtraSize)
		n, err := conn.Conn.Read(respExtra)
		if err != nil {
			return resp, respExtra[:n], fmt.Errorf("read response extra data: %v", err)
		}
		if n != int(resp.ExtraSize) {
			return resp, respExtra[:n], fmt.Errorf("short read on response extra data: expected %d, got %d", resp.ExtraSize, n)
		}
	}

	log.Printf("Recieved control response from worker %v: {%v} + %v\n",
		workerID, resp.String(), respExtra)

	return resp, respExtra, nil
}

func (m *Manager) handleWorkerFailure(workerID uint64) {
	log.Printf("Terminating worker %v session", workerID)
	worker, ok := m.workers[workerID]
	if !ok {
		return
	}

	if worker.taskId != 0 {
		m.requeueTask(worker.taskId)
	}

	delete(m.workers, workerID)
}

func (m *Manager) markWorkerFree(workerID uint64) {
	worker, ok := m.workers[workerID]
	if !ok {
		return
	}

	worker.state = WorkerFree
	worker.taskId = 0
	m.freeWorkers <- workerID
}

func (m *Manager) requeueTask(taskID uint64) {
	if _, ok := m.tasks[taskID]; ok {
		m.queuedTasks <- taskID
	}
}

func (m *Manager) handleRegisterPulse(pulse *communication.WorkerPulse) *communication.PulseResponse {
	m.workersMutex.Lock()
	defer m.workersMutex.Unlock()

	atomic.AddUint64(&m.workerId, 1)
	workerID := m.workerId
	m.workers[workerID] = &Worker{
		state:     WorkerBooting,
		lastPulse: time.Now(),
		nextPulse: time.Duration(pulse.NextPulse) * time.Second,
	}

	return &communication.PulseResponse{
		Error:    communication.ControllerError_CTRL_SUCCESS,
		WorkerId: workerID,
	}
}

func (m *Manager) handleOkPulse(pulse *communication.WorkerPulse) *communication.PulseResponse {
	m.workersMutex.Lock()
	defer m.workersMutex.Unlock()

	worker, ok := m.workers[pulse.WorkerId]

	if !ok {
		return &communication.PulseResponse{
			Error:    communication.ControllerError_CTRL_ERR_UNKNOWN_ID,
			WorkerId: 0,
		}
	}

	if worker.state == WorkerBooting {
		m.markWorkerFree(pulse.WorkerId)
	}

	worker.lastPulse = time.Now()
	worker.nextPulse = time.Duration(pulse.NextPulse) * time.Second

	return &communication.PulseResponse{
		Error:    communication.ControllerError_CTRL_SUCCESS,
		WorkerId: 0,
	}
}

func (m *Manager) handleFetchMePulse(pulse *communication.WorkerPulse) *communication.PulseResponse {
	resp := m.handleOkPulse(pulse)
	if resp.Error != communication.ControllerError_CTRL_SUCCESS {
		return resp
	}

	m.fetchWorkers <- pulse.WorkerId
	return &communication.PulseResponse{
		Error:    communication.ControllerError_CTRL_SUCCESS,
		WorkerId: 0,
	}
}

func (m *Manager) handleShutdownPulse(pulse *communication.WorkerPulse) *communication.PulseResponse {
	workerID := pulse.WorkerId
	worker, ok := m.workers[workerID]
	if !ok {
		return &communication.PulseResponse{
			Error:    communication.ControllerError_CTRL_ERR_UNKNOWN_ID,
			WorkerId: 0,
		}
	}

	if worker.taskId != 0 {
		m.requeueTask(worker.taskId)
	}

	delete(m.workers, workerID)
	return &communication.PulseResponse{
		Error:    communication.ControllerError_CTRL_SUCCESS,
		WorkerId: 0,
	}
}

func (m *Manager) handleConnection(conn conn.Unix) {
	defer conn.Close()

	msgData, err := conn.ReadMessage()
	if err != nil {
		m.errorChan <- fmt.Errorf("read error: %w", err)
		return
	}

	var pulse communication.WorkerPulse
	if err := proto.Unmarshal(msgData, &pulse); err != nil {
		m.errorChan <- fmt.Errorf("unmarshal error: %w", err)
		return
	}

	resp := m.handlePulse(&pulse)
	respData, err := proto.Marshal(resp)
	if err != nil {
		m.errorChan <- fmt.Errorf("marshal error: %w", err)
		return
	}

	if err := conn.WriteMessage(respData); err != nil {
		m.errorChan <- fmt.Errorf("write error: %w", err)
	}
}

func (m *Manager) handlePulse(pulse *communication.WorkerPulse) *communication.PulseResponse {
	switch pulse.Type {
	case communication.PulseType_PULSE_REGISTER:
		return m.handleRegisterPulse(pulse)
	case communication.PulseType_PULSE_OK:
		return m.handleOkPulse(pulse)
	case communication.PulseType_PULSE_FETCH_ME:
		return m.handleFetchMePulse(pulse)
	case communication.PulseType_PULSE_SHUTDOWN:
		return m.handleShutdownPulse(pulse)
	default:
		return &communication.PulseResponse{
			Error: communication.ControllerError_CTRL_ERR_UNKNOWN_TYPE,
		}
	}
}

func (m *Manager) mainLoop() {
	log.Print("Listening on main.sock")
	for {
		select {
		case <-m.shutdown:
			return
		default:
			netConn, err := m.listener.Accept()
			if err != nil {
				if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
					continue
				}
				log.Printf("Fatal accept error: %v", err)
				return
			}
			go m.handleConnection(conn.Unix{Conn: netConn})
		}
	}
}

// errorHandler catches the errors from goroutines and logs them.
// This function should always be run in a goroutine.
func (m *Manager) errorHandler() {
	for err := range m.errorChan {
		log.Println("ERROR:", err)
	}
}

func (m *Manager) assignTaskToWorker(taskID uint64, workerID uint64) {
	task, ok := m.tasks[taskID]
	if !ok {
		log.Printf("Task %d not found", taskID)
		m.freeWorkers <- workerID
		return
	}

	worker, ok := m.workers[workerID]
	if !ok {
		log.Printf("Worker %d not found", workerID)
		m.queuedTasks <- taskID
		return
	}

	if worker.state != WorkerFree {
		log.Printf("Worker %d is not free", workerID)
		m.queuedTasks <- taskID
		m.freeWorkers <- workerID
		return
	}

	msg := &communication.ControlMsg{
		Type:      communication.ControlType_CTRL_SET_TASK,
		TaskId:    taskID,
		ExtraSize: uint64(len(task.body)),
	}

	resp, _, err := sendControlMessage(workerID, msg, task.body)
	if err != nil {
		log.Printf("Failed to send SET_TASK to worker %d: %v", workerID, err)
		m.handleWorkerFailure(workerID)
		m.queuedTasks <- taskID
		return
	}

	if resp.Error != communication.WorkerError_WORKER_SUCCESS {
		log.Printf("Worker %d error on SET_TASK: %v", workerID, resp.Error)
		if resp.Error == communication.WorkerError_WORKER_ERR_BUSY {
			m.freeWorkers <- workerID
		} else {
			m.handleWorkerFailure(workerID)
		}
		m.queuedTasks <- taskID
		return
	}

	worker.state = WorkerBusy
	worker.taskId = taskID
}

func (m *Manager) dispatchTasks() {
	for {
		taskID := <-m.queuedTasks
		workerID := <-m.freeWorkers

		go m.assignTaskToWorker(taskID, workerID)
	}
}

func (m *Manager) fetchTaskResult(workerID uint64) {
	msg := &communication.ControlMsg{
		Type: communication.ControlType_CTRL_FETCH,
	}

	resp, extra, err := sendControlMessage(workerID, msg, nil)
	if err != nil {
		log.Printf("Failed to FETCH from worker %d: %v", workerID, err)
		m.handleWorkerFailure(workerID)
		return
	}

	switch resp.Error {
	case communication.WorkerError_WORKER_ERR_NO_FETCH:
		return
	case communication.WorkerError_WORKER_ERR_TASK_FAILED:
		m.requeueTask(resp.TaskId)
		m.markWorkerFree(workerID)
	case communication.WorkerError_WORKER_SUCCESS:
		task, ok := m.tasks[resp.TaskId]
		desired_chan := &m.taskSolutions
		if !ok {
			log.Printf("Task %v solved but not found. Very weird", resp.TaskId)
		} else {
			log.Printf("Task %v solved", resp.TaskId)
			if task.receiver != nil {
				desired_chan = task.receiver
			}
			delete(m.tasks, resp.TaskId)
		}

		*desired_chan <- TaskSolution{Id: resp.TaskId, Body: extra}
		m.markWorkerFree(workerID)
	default:
		log.Printf("Worker %d FETCH error: %v", workerID, resp.Error)
		m.handleWorkerFailure(workerID)
	}
}

func (m *Manager) handleFetchWorkers() {
	for {
		select {
		case <-m.shutdown:
			return
		case workerID := <-m.fetchWorkers:
			go m.fetchTaskResult(workerID)
		}
	}
}

// very bad
func (m *Manager) checkHealth() {
	ticker := time.NewTicker(30 * time.Second)
	defer ticker.Stop()
	for {
		select {
		case <-m.shutdown:
			return
		case <-ticker.C:
			m.workersMutex.Lock()
			now := time.Now()
			for workerID, worker := range m.workers {
				if now.Sub(worker.lastPulse) > worker.nextPulse {
					log.Printf("Worker %d missed pulse", workerID)
					m.handleWorkerFailure(workerID)
				}
			}
			m.workersMutex.Unlock()
		}
	}
}

// receiver can be nil, then task solution will be piped into taskSolutions and can be received via GetTaskSolution()
func (m *Manager) AddTask(taskData []byte, receiver *chan TaskSolution) uint64 {
	atomic.AddUint64(&m.taskId, 1)
	taskID := m.taskId
	task := &Task{
		id:       taskID,
		body:     taskData,
		receiver: receiver,
	}

	m.tasksMutex.Lock()
	m.tasks[taskID] = task
	m.tasksMutex.Unlock()

	m.queuedTasks <- taskID
	return taskID
}

func (m *Manager) GetTaskSolution() TaskSolution {
	return <-m.taskSolutions
}

func removeContents(dir string) error {
	entries, err := os.ReadDir(dir)
	if err != nil {
		return err
	}

	for _, entry := range entries {
		if err := os.RemoveAll(filepath.Join(dir, entry.Name())); err != nil {
			return err
		}
	}
	return nil
}

func NewManager() (*Manager, error) {
	if err := removeContents(workerSocketPath); err != nil {
		return nil, fmt.Errorf("failed to clean socket directory: %w", err)
	}

	listener, err := net.Listen("unix", workerMainSocketPath)
	if err != nil {
		return nil, fmt.Errorf("failed to create listener: %w", err)
	}

	m := &Manager{
		listener:      listener,
		workers:       make(map[uint64]*Worker),
		tasks:         make(map[uint64]*Task),
		freeWorkers:   make(chan uint64, 32),
		fetchWorkers:  make(chan uint64, 32),
		queuedTasks:   make(chan uint64, 32),
		taskSolutions: make(chan TaskSolution, 100),
		errorChan:     make(chan error, 10),
		shutdown:      make(chan struct{}),
	}

	go m.mainLoop()
	go m.errorHandler()
	go m.dispatchTasks()
	go m.checkHealth()
	go m.handleFetchWorkers()

	return m, nil
}

func (m *Manager) Shutdown() {
	m.shutdownOnce.Do(func() {
		close(m.shutdown)
		m.listener.Close()

		close(m.freeWorkers)
		close(m.fetchWorkers)
		close(m.queuedTasks)
		close(m.taskSolutions)
		close(m.errorChan)
	})
}
