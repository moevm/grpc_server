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
	"errors"
	"fmt"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/moevm/grpc_server/internal/conn"
	"github.com/moevm/grpc_server/pkg/converter"
)

const maxAttempts = 3

var (
	lastWorkerIndex int
	mu              sync.Mutex
)

const (
	// Task states.
	taskFree       = 3
	taskRedirected = 2
	taskWip        = 1
	taskSolved     = 0

	// Worker states.
	workerBusy = 1
	workerFree = 0
	workerDown = -1

	workerInitSocketPath = "/run/controller/init.sock"
	workerSocketPath     = "/run/controller/"

	intByteLen = 8 // default for x86_64

	successfulResp = 1
)

var (
	// Channel for errors returned from goroutines.
	errorChan = make(chan error, 10)

	listener net.Listener

	// Workers map.
	workers  sync.Map
	workerId = 0

	// Tasks map.
	tasks  sync.Map
	taskId = 0
)

// Task is a general representation of tasks coming to the controller.
// Can be changed if it is necessary to add new info for the Task
// for example: hashType.
type Task struct {
	id    int
	state int
	body  []byte // some data with which something needs to be done (a task needs to be solved)
	solve []byte // solution for the task (in general)
}

// Worker contains everything necessary for communication with the worker and his current state.
type Worker struct {
	id       int
	state    int
	taskChan chan *Task // channel for transferring tasks to the worker
	conn     conn.Unix
}

// Constructor for Task.
func NewTask(taskId int, taskBody []byte) *Task {
	return &Task{
		id:    taskId,
		state: taskFree,
		body:  taskBody,
		solve: []byte{},
	}
}

// Constructor for Worker.
func NewWorker(workerId int) *Worker {
	return &Worker{
		id:       workerId,
		state:    workerBusy,
		taskChan: make(chan *Task, 10),
	}
}

func (t *Task) SetTaskState(state int) error {
	if t.state == taskSolved {
		return errors.New("can't set task state: task solved")
	}

	switch state {
	case taskFree:
		t.state = taskFree
		return nil

	case taskRedirected:
		t.state = taskRedirected
		return nil

	case taskWip:
		t.state = taskWip
		return nil

	case taskSolved:
		t.state = taskSolved
		return nil

	default:
		return errors.New("invalid task state")
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

func (w *Worker) SetWorkerState(state int) error {
	if w.state == workerDown {
		return errors.New("can't set worker state: worker down")
	}

	switch state {
	case workerBusy:
		w.state = workerBusy
		return nil

	case workerFree:
		w.state = workerFree
		return nil

	case workerDown:
		w.state = workerDown
		return nil

	default:
		return errors.New("invalid worker state")
	}
}

func (w *Worker) SetWorkerConn(workerConn conn.Unix) {
	w.conn = workerConn
}

// Run includes the initialized worker in the work.
// Worker will wait for the task from the channel.
// This function should always be run in a goroutine.
func (w *Worker) Run() {
	log.Printf("Worker [ID=%d] running and awaiting a task\n", w.id)
	var socketPath strings.Builder
	socketPath.WriteString(workerSocketPath)
	socketPath.WriteString(fmt.Sprintf("%v.sock", w.id))

	if err := w.SetWorkerState(workerFree); err != nil {
		errorChan <- fmt.Errorf("Worker.Run - SetWorkerState: %v", err)
		return
	}

	for {
		task := <-w.taskChan

		netConn, err := net.Dial("unix", socketPath.String())
		if err != nil {
			errorChan <- fmt.Errorf("Worker.Run - net.Dial: %v", err)
			if err := w.SetWorkerState(workerDown); err != nil {
				errorChan <- fmt.Errorf("Worker.Run - SetWorkerState: %v", err)
			}
			if err := task.SetTaskState(taskFree); err != nil {
				errorChan <- fmt.Errorf("Worker.Run - SetTaskState: %v", err)
			}
			continue
		}

		connection := conn.Unix{Conn: netConn}
		w.SetWorkerConn(connection)

		w.AddTask(task)
		connection.Close()

		log.Printf(
			"Worker [ID=%d] completed the task [TaskID=%d, SolutionSize=%d bytes]\n",
			w.id, task.id, len(task.solve),
		)
	}
}

// AddTask function assigns a task to a worker
// (the task and worker states change to taskWip and workerBusy until the task is completed).
// This function sends task.body length and task.body to the worker
// and receives task.solve length and task.solve from him in case of success,
// in case of failure it logs the error through a special channel: errorChan
// and set worker state to workerDown.
// If successful, the worker and task will set the status to workerFree and taskSolved.
func (w *Worker) AddTask(task *Task) {
	if w.taskChan == nil || w.state != workerFree {
		errorChan <- fmt.Errorf("worker is broken")
		if err := w.SetWorkerState(workerDown); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		}
		if err := task.SetTaskState(taskFree); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		}
		return
	}

	if task.body == nil {
		errorChan <- fmt.Errorf("task %v is empty", task.id)
		if err := task.SetTaskState(taskSolved); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		}
		if err := task.SetTaskSolve(nil); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskSolve: %v", err)
		}
		return
	}

	if err := w.SetWorkerState(workerBusy); err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		return
	}
	if err := task.SetTaskState(taskWip); err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		return
	}

	defer func() {
		if err := w.SetWorkerState(workerFree); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		}
	}()

	taskLen := len(task.body)
	var err error
	_, err = w.conn.Write(converter.IntToByteSlice(taskLen))
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Write: %v", err)
		if err := w.SetWorkerState(workerDown); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		}
		if err := task.SetTaskState(taskFree); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		}
		return
	}

	_, err = w.conn.Write(task.body)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Write: %v", err)
		if err := w.SetWorkerState(workerDown); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		}
		if err := task.SetTaskState(taskFree); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		}
		return
	}

	responseLenBuf := make([]byte, intByteLen)
	_, err = w.conn.Read(responseLenBuf)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Read: %v", err)
		if err := w.SetWorkerState(workerDown); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		}
		if err := task.SetTaskState(taskFree); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		}
		return
	}

	responseLen, err := converter.ByteSliceToInt(responseLenBuf)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - converter.ByteSliceToInt: %v", err)
		if err := w.SetWorkerState(workerDown); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		}
		if err := task.SetTaskState(taskFree); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		}
		return
	}

	response := make([]byte, responseLen)
	_, err = w.conn.Read(response)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Read: %v", err)
		if err := w.SetWorkerState(workerDown); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetWorkerState: %v", err)
		}
		if err := task.SetTaskState(taskFree); err != nil {
			errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
		}
		return
	}

	if err := task.SetTaskSolve(response); err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - SetTaskSolve: %v", err)
	}
	if err := task.SetTaskState(taskSolved); err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - SetTaskState: %v", err)
	}

	fmt.Printf("Task solved: %v", task)
}

func genTaskId() int {
	taskId += 1
	return taskId
}

func genWorkerId() int {
	workerId += 1
	return workerId
}

// workerInit initializes a new worker that has already been created and is ready to be connected.
// To connect, the worker must do net.Dial("unix", "/run/controller/init.sock"),
// receives his ID and send successfulResp (1).
// In case of failure it logs the error through a special channel: errorChan
// and set worker state to workerDown.
// If successful, it run worker ( worker.Run() ).
// This function should always be run in a goroutine.
func workerInit() {
	log.Println("The worker initialization server is running. Waiting for connections...")

	for {
		netConn, err := listener.Accept()
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - listener.Accept: %v", err)
			break
		}

		connection := conn.Unix{Conn: netConn}

		worker := NewWorker(genWorkerId())
		workers.Store(worker.id, worker)

		idBuf := converter.IntToByteSlice(worker.id)
		_, err = connection.Write(idBuf)
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - connection.Write: %v", err)
			if err := worker.SetWorkerState(workerDown); err != nil {
				errorChan <- fmt.Errorf("workerInit - SetWorkerState: %v", err)
			}
			break
		}

		responseBuf := make([]byte, intByteLen)
		_, err = connection.Read(responseBuf)
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - conn.Read: %v", err)
			if err := worker.SetWorkerState(workerDown); err != nil {
				errorChan <- fmt.Errorf("workerInit - SetWorkerState: %v", err)
			}
			break
		}

		response, err := converter.ByteSliceToInt(responseBuf)
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - converter.ByteSliceToInt: %v", err)
			if err := worker.SetWorkerState(workerDown); err != nil {
				errorChan <- fmt.Errorf("workerInit - SetWorkerState: %v", err)
			}
			break
		}
		if response != successfulResp {
			errorChan <- fmt.Errorf("wrong response from worker")
			if err := worker.SetWorkerState(workerDown); err != nil {
				errorChan <- fmt.Errorf("workerInit - SetWorkerState: %v", err)
			}
			break
		}

		go worker.Run()

		log.Printf("The worker is connected [ID=%d, Status=%d]\n", worker.id, worker.state)
		connection.Close()
	}
}

// loadBalancer is an implementation of round-robin algorithm for evenly distributing tasks among workers.
// The maximum number of attempts to submit one task is 3.
// In case of failure it logs the error through a special channel: errorChan.
func loadBalancer(task *Task) {
	log.Printf(
		"Finding a worker for a task [TaskID=%d, Size=%d bytes, State=%d]\n",
		task.id, len(task.body), task.state,
	)
	mu.Lock()
	defer mu.Unlock()

	startIndex := lastWorkerIndex
	attempts := 0

	for attempts < maxAttempts {
		for i := 0; i <= workerId; i++ {
			currentIndex := (startIndex + i) % (workerId + 1)

			worker, exist := workers.Load(currentIndex)
			if !exist {
				continue
			}

			w := worker.(*Worker)
			if w.state == workerFree {
				select {
				case w.taskChan <- task:
					if err := task.SetTaskState(taskRedirected); err != nil {
						errorChan <- fmt.Errorf("loadBalancer: task %d: %v", task.id, err)
						if err := task.SetTaskState(taskFree); err != nil {
							errorChan <- fmt.Errorf("loadBalancer: failed to revert task %d to free: %v", task.id, err)
						}
						continue
					}
					log.Printf("The task is distributed: ID=%d -> Worker ID=%d\n", task.id, currentIndex)
					lastWorkerIndex = (currentIndex + 1) % (workerId + 1)
					return
				default:
					continue
				}
			}
		}
		attempts++
		time.Sleep(100 * time.Millisecond)
		log.Printf("There are no available workers for the task [TaskID=%d]\n", task.id)
	}

	errorChan <- fmt.Errorf("no available workers for task %d", task.id)
	if err := task.SetTaskState(taskFree); err != nil {
		errorChan <- fmt.Errorf("loadBalancer: failed to set task %d to free: %v", task.id, err)
	}
}

// taskManager checking new tasks ans send it to loadBalancer.
// This function should always be run in a goroutine.
func taskManager() {
	for {
		for i := 0; i <= taskId; i++ {
			task, exist := tasks.Load(i)
			if exist {
				t := task.(*Task)
				if t.state == taskFree {
					loadBalancer(t)
				}
			}
		}
	}
}

// errorHandler catches the errors from goroutines and logs them.
// This function should always be run in a goroutine.
func errorHandler() {
	for {
		err := <-errorChan
		log.Println(err)
	}
}

// TODO: Maybe it's worth getting rid of this...
func init() {
	err := os.RemoveAll(workerInitSocketPath)
	if err != nil {
		log.Panic(err)
	}

	channel := make(chan os.Signal, 10)

	listener, err = net.Listen("unix", workerInitSocketPath)
	if err != nil {
		log.Panic(err)
	}

	signal.Notify(channel, os.Interrupt, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-channel

		listener.Close()
		os.RemoveAll(workerInitSocketPath)

		os.Exit(1)
	}()
}

// TODO: add doc.
func hasActiveWorkers() bool {
	hasWorkers := false
	workers.Range(func(key, value interface{}) bool {
		worker := value.(*Worker)
		if worker.state != workerDown {
			hasWorkers = true
			return false
		}
		return true
	})
	return hasWorkers
}

func taskReceiver(taskChan <-chan []byte) {
	for content := range taskChan {
		task := NewTask(genTaskId(), content)
		tasks.Store(task.id, task)
		log.Printf("New task received from gRPC: %d", task.id)
	}
}

func InitManager(taskChanSize int) (chan<- []byte, error) {
	taskChan := make(chan []byte, taskChanSize)
	go func() {
		ClusterInit([][]byte{}, taskChan)
	}()
	return taskChan, nil
}

// ClusterInit is the main function that starts the controller (all necessary goroutines)
// and receives data from the server (needs to be implemented in the future,
// currently a stub in the form of taskData is used).
func ClusterInit(taskData [][]byte, taskChan <-chan []byte) {
	log.Println("=== The controller is running ===")
	log.Printf("Received tasks: %d\n", len(taskData))

	go workerInit()
	go errorHandler()
	go taskReceiver(taskChan)

	log.Println("Waiting for at least one worker to connect...")
	for {
		if hasActiveWorkers() {
			log.Println("Active workers have been found. I'm starting the assignment of tasks.")
			break
		}
		time.Sleep(1 * time.Second)
	}

	for i := range taskData {
		task := NewTask(genTaskId(), taskData[i])
		tasks.Store(i, task)
		log.Printf("Task added [ID=%d, Size=%d bytes, State=%d]\n", task.id, len(task.body), task.state)
	}

	go taskManager()

	for {
		time.Sleep(1 * time.Second)
	}
}
