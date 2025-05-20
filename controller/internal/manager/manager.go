// TODO: add Doc comments
package manager

import (
	"errors"
	"fmt"
	"log"
	"net"
	"os"
	"sync"
	"time"

	"github.com/moevm/grpc_server/internal/conn"
	"google.golang.org/protobuf/proto"

	communication "github.com/moevm/grpc_server/pkg/proto/communication"
)

const (
	// Worker states.
	WORKER_BUSY    = 0
	WORKER_FREE    = 1
	WORKER_BOOTING = 2

	workerMainSocketPath = "/run/controller/main.sock"
	workerSocketPath     = "/run/controller/"

	maxAttempts = 3
)

var (
	// Channel for errors returned from goroutines.
	errorChan = make(chan error, 10)

	listener net.Listener

	// Workers map.
	workers         = make(map[uint64]*Worker)
	workerId uint64 = 0

	// Tasks map.
	tasks         = make(map[uint64]*Task)
	taskId uint64 = 0

	freeWorkers  = make(chan uint64)
	fetchWorkers = make(chan uint64)
	queuedTasks  = make(chan uint64)

	solved_tasks sync.Map
)

// Can be changed if it is necessary
// to add new info for the Task
// for example: hashType.
type Task struct {
	id    uint64
	body  []byte
	solve []byte
}

type Worker struct {
	state      int
	last_pulse time.Time
	next_pulse time.Duration
	task_id    uint64
}

// Constructor for Task.
func NewTask(taskId uint64, taskBody []byte) *Task {
	return &Task{
		id:    taskId,
		body:  taskBody,
		solve: []byte{},
	}
}

// Constructor for Worker.
func NewWorker() *Worker {
	return &Worker{
		state:      WORKER_BOOTING,
		last_pulse: time.Now(),
		task_id:    0, // no task
	}
}

func (t *Task) SetTaskSolve(taskSolve []byte) error {
	if len(t.solve) > 0 {
		return errors.New("can't set task solve: task already solved")
	}

	t.solve = append(t.solve, taskSolve...)

	return nil
}

func (t *Task) String() string {
	return fmt.Sprintf("id:%v, solution:%v\n", t.id, t.solve)
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

func genTaskId() uint64 {
	taskId += 1
	return taskId
}

func genWorkerId() uint64 {
	workerId += 1
	return workerId
}

func handleWorkerFailure(workerID uint64) {
	log.Printf("Terminating worker %v session", workerID)
	worker, ok := workers[workerID]
	if !ok {
		return
	}

	if worker.task_id != 0 {
		requeueTask(worker.task_id)
	}

	delete(workers, workerID)
}

func markWorkerFree(workerID uint64) {
	worker, ok := workers[workerID]
	if !ok {
		return
	}

	worker.state = WORKER_FREE
	worker.task_id = 0
	freeWorkers <- workerID
}

func requeueTask(taskID uint64) {
	if _, ok := tasks[taskID]; ok {
		queuedTasks <- taskID
	}
}

func handleRegisterPulse(_ *communication.WorkerPulse, resp *communication.PulseResponse) {
	workerID := genWorkerId()
	workers[workerID] = NewWorker()
	*resp = communication.PulseResponse{
		Error:    communication.ControllerError_CTRL_ERR_OK,
		WorkerId: workerID,
	}
}

func handleOkPulse(pulse *communication.WorkerPulse, resp *communication.PulseResponse) {
	worker, ok := workers[pulse.WorkerId]

	if !ok {
		*resp = communication.PulseResponse{
			Error:    communication.ControllerError_CTRL_ERR_UNKNOWN_ID,
			WorkerId: 0,
		}
		return
	}

	if pulse.TaskId != worker.task_id {
		log.Printf("pulse.TaskId and worker.task_id mismatch (%v and %v)\n", pulse.TaskId, worker.task_id)
		msg := &communication.ControlMsg{
			Type: communication.ControlType_CTRL_RESTART,
		}
		sendControlMessage(pulse.WorkerId, msg, nil)
		handleWorkerFailure(pulse.WorkerId)
	}

	if worker.state == WORKER_BOOTING {
		markWorkerFree(pulse.WorkerId)
	}

	worker.last_pulse = time.Now()
	worker.next_pulse = time.Duration(pulse.NextPulse) * time.Second

	*resp = communication.PulseResponse{
		Error:    communication.ControllerError_CTRL_ERR_OK,
		WorkerId: 0,
	}
}

func handleFetchMePulse(pulse *communication.WorkerPulse, resp *communication.PulseResponse) {
	handleOkPulse(pulse, resp)
	if resp.Error != communication.ControllerError_CTRL_ERR_OK {
		return
	}

	fetchWorkers <- pulse.WorkerId
}

func handleShutdownPulse(pulse *communication.WorkerPulse, resp *communication.PulseResponse) {
	workerID := pulse.WorkerId
	worker, ok := workers[workerID]
	if !ok {
		resp.Error = communication.ControllerError_CTRL_ERR_UNKNOWN_ID
		return
	}

	if worker.task_id != 0 {
		requeueTask(worker.task_id)
	}

	delete(workers, workerID)
	resp.Error = communication.ControllerError_CTRL_ERR_OK
}

func handleConnection(conn conn.Unix) {
	msg_data, err := conn.ReadMessage()
	if err != nil {
		log.Printf("Failed to read message: %v", err)
		conn.Close()
		return
	}

	pulse := communication.WorkerPulse{}
	if err := proto.Unmarshal(msg_data, &pulse); err != nil {
		log.Printf("Failed to unmarshal pulse: %v", err)
		conn.Close()
		return
	}

	resp := communication.PulseResponse{}

	switch pulse.Type {
	case communication.PulseType_PULSE_REGISTER:
		handleRegisterPulse(&pulse, &resp)
	case communication.PulseType_PULSE_OK:
		handleOkPulse(&pulse, &resp)
	case communication.PulseType_PULSE_FETCH_ME:
		handleFetchMePulse(&pulse, &resp)
	case communication.PulseType_PULSE_SHUTDOWN:
		handleShutdownPulse(&pulse, &resp)
	default:
		log.Printf("Unknown pulse type: %v", pulse.Type)
		resp.Error = communication.ControllerError_CTRL_ERR_UNKNOWN_TYPE
	}

	resp_data, err := proto.Marshal(&resp)
	if err != nil {
		log.Printf("Failed to marshal response: %v", err)
		conn.Close()
		return
	}

	if err := conn.WriteMessage(resp_data); err != nil {
		log.Printf("Failed to write response: %v", err)
	}

	log.Printf("Recieved pulse: {%v}. Sent response: {%v}\n", pulse.String(), resp.String())

	if err := conn.Close(); err != nil {
		log.Printf("Error closing connection: %v", err)
	}
}

func mainLoop() {
	log.Print("Ready to receive messages on main.sock")
	for {
		netConn, err := listener.Accept()
		if err != nil {
			log.Printf("Fatal accept error: %v; closing listener", err)
			return
		}

		conn := conn.Unix{Conn: netConn}
		go handleConnection(conn)
	}
}

func errorHandler() {
	for err := range errorChan {
		log.Println("ERROR:", err)
	}
}

func assignTaskToWorker(taskID uint64, workerID uint64) {
	task, ok := tasks[taskID]
	if !ok {
		log.Printf("Task %d not found", taskID)
		freeWorkers <- workerID
		return
	}

	worker, ok := workers[workerID]
	if !ok {
		log.Printf("Worker %d not found", workerID)
		queuedTasks <- taskID
		return
	}

	if worker.state != WORKER_FREE {
		log.Printf("Worker %d is not free", workerID)
		queuedTasks <- taskID
		freeWorkers <- workerID
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
		handleWorkerFailure(workerID)
		queuedTasks <- taskID
		return
	}

	if resp.Error != communication.WorkerError_WORKER_ERR_OK {
		log.Printf("Worker %d error on SET_TASK: %v", workerID, resp.Error)
		if resp.Error == communication.WorkerError_WORKER_ERR_BUSY {
			freeWorkers <- workerID
		} else {
			handleWorkerFailure(workerID)
		}
		queuedTasks <- taskID
		return
	}

	worker.state = WORKER_BUSY
	worker.task_id = taskID
}

func dispanchTasks() {
	for {
		taskID := <-queuedTasks
		workerID := <-freeWorkers

		go assignTaskToWorker(taskID, workerID)
	}
}

func fetchTaskResult(workerID uint64) {
	msg := &communication.ControlMsg{
		Type: communication.ControlType_CTRL_FETCH,
	}

	resp, extra, err := sendControlMessage(workerID, msg, nil)
	if err != nil {
		log.Printf("Failed to FETCH from worker %d: %v", workerID, err)
		handleWorkerFailure(workerID)
		return
	}

	switch resp.Error {
	case communication.WorkerError_WORKER_ERR_NO_FETCH:
		return
	case communication.WorkerError_WORKER_ERR_TASK_FAILED:
		requeueTask(resp.TaskId)
		markWorkerFree(workerID)
	case communication.WorkerError_WORKER_ERR_OK:
		task, ok := tasks[resp.TaskId]
		if !ok {
			log.Printf("Task %d not found", resp.TaskId)
			markWorkerFree(workerID)
			return
		}
		if err := task.SetTaskSolve(extra); err != nil {
			log.Printf("Failed to set task solution: %v", err)
			requeueTask(resp.TaskId)
		} else {
			solved_tasks.Store(resp.TaskId, task)
			delete(tasks, resp.TaskId)
		}
		markWorkerFree(workerID)
	default:
		log.Printf("Worker %d FETCH error: %v", workerID, resp.Error)
		handleWorkerFailure(workerID)
	}
}

// very bad
func checkHealth() {
	for {
		time.Sleep(30 * time.Second)
		now := time.Now()
		for workerID, worker := range workers {
			elapsed := now.Sub(worker.last_pulse)
			if elapsed > worker.next_pulse {
				log.Printf("Worker %d missed pulse", workerID)
				handleWorkerFailure(workerID)
			}
		}
	}
}

func ClusterInit(taskData [][]byte) {
	err := os.RemoveAll(workerMainSocketPath)
	if err != nil {
		log.Panic(err)
	}

	listener, err = net.Listen("unix", workerMainSocketPath)
	if err != nil {
		log.Panic(err)
	}

	go mainLoop()
	go errorHandler()
	go dispanchTasks()
	go checkHealth()

	go func() {
		for workerID := range fetchWorkers {
			go fetchTaskResult(workerID)
		}
	}()

	defer func() {
		listener.Close()
		os.RemoveAll(workerMainSocketPath)
		close(errorChan)
		close(freeWorkers)
		close(queuedTasks)
		close(fetchWorkers)
	}()

	for _, task := range taskData {
		taskId = genTaskId()
		tasks[taskId] = NewTask(taskId, task)
		queuedTasks <- taskId
	}

	for {
		time.Sleep(1 * time.Second)
	}
}
