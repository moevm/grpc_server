// TODO: add Doc comments
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

// Can be changed if it is necessary
// to add new info for the Task
// for example: hashType.
type Task struct {
	id    int
	state int
	body  []byte
	solve []byte
}

type Worker struct {
	id       int
	state    int
	taskChan chan *Task
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

// Run new Worker.
// Worker will wait for the task from the channel.
func (w *Worker) Run() {
	var socketPath strings.Builder
	socketPath.WriteString(workerSocketPath)
	socketPath.WriteString(fmt.Sprintf("%v.sock", w.id))

	w.SetWorkerState(workerFree)

	for {
		task := <-w.taskChan

		netConn, err := net.Dial("unix", socketPath.String())
		if err != nil {
			errorChan <- fmt.Errorf("Worker.Run - net.Dial: %v", err)
			w.SetWorkerState(workerDown)
			task.SetTaskState(taskFree)
		}

		connection := conn.Unix{Conn: netConn}
		w.SetWorkerConn(connection)

		fmt.Printf("Task %v accepted by worker %v\n", task.id, w.id)

		w.AddTask(task)
		connection.Close()
	}
}

// Sends Task.body to a worker and receive a Task.solution.
func (w *Worker) AddTask(task *Task) {
	if w.taskChan == nil || w.state != workerFree {
		errorChan <- fmt.Errorf("worker is broken")
		w.SetWorkerState(workerDown)
		task.SetTaskState(taskFree)
		return
	}

	if task.body == nil {
		errorChan <- fmt.Errorf("task %v is empty", task.id)
		task.SetTaskState(taskSolved)
		task.SetTaskSolve(nil)
	}

	w.SetWorkerState(workerBusy)
	task.SetTaskState(taskWip)

	defer func() {
		w.SetWorkerState(workerFree)
	}()

	taskLen := len(task.body)
	var err error
	// Send task len to worker.
	_, err = w.conn.Write(converter.IntToByteSlice(taskLen))
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Write: %v", err)
		w.SetWorkerState(workerDown)
		task.SetTaskState(taskFree)
		return
	}
	// Send task.body to worker.
	_, err = w.conn.Write(task.body)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Write: %v", err)
		w.SetWorkerState(workerDown)
		task.SetTaskState(taskFree)
		return
	}

	responseLenBuf := make([]byte, intByteLen)
	// Receive response len.
	_, err = w.conn.Read(responseLenBuf)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Read: %v", err)
		w.SetWorkerState(workerDown)
		task.SetTaskState(taskFree)
		return
	}

	responseLen, err := converter.ByteSliceToInt(responseLenBuf)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - converter.ByteSliceToInt: %v", err)
		w.SetWorkerState(workerDown)
		task.SetTaskState(taskFree)
		return
	}

	response := make([]byte, responseLen)
	// Receive response (solution for task).
	_, err = w.conn.Read(response)
	if err != nil {
		errorChan <- fmt.Errorf("Worker.AddTask - w.conn.Read: %v", err)
		w.SetWorkerState(workerDown)
		task.SetTaskState(taskFree)
		return
	}

	task.SetTaskSolve(response)
	task.SetTaskState(taskSolved)

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

func workerInit() {
	fmt.Println("Ready for initialization Workers...")

	for {
		netConn, err := listener.Accept()
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - listener.Accept: %v", err)
			break
		}

		connection := conn.Unix{Conn: netConn}

		// Create new worker.
		worker := NewWorker(genWorkerId())
		workers.Store(worker.id, worker)

		// Send worker his id.
		idBuf := converter.IntToByteSlice(worker.id)
		_, err = connection.Write(idBuf)
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - connection.Write: %v", err)
			worker.SetWorkerState(workerDown)
			break
		}

		responseBuf := make([]byte, intByteLen)
		// Get an response.
		_, err = connection.Read(responseBuf)
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - conn.Read: %v", err)
			worker.SetWorkerState(workerDown)
			break
		}

		response, err := converter.ByteSliceToInt(responseBuf)
		if err != nil {
			errorChan <- fmt.Errorf("workerInit - converter.ByteSliceToInt: %v", err)
			worker.SetWorkerState(workerDown)
			break
		}
		// Check the response.
		if response != successfulResp {
			errorChan <- fmt.Errorf("wrong response from worker")
			worker.SetWorkerState(workerDown)
			break
		}
		// If everything is Ok
		// start worker.
		go worker.Run()

		fmt.Printf("Worker %v started\n", worker.id)
		connection.Close()
	}
}


func loadBalancer(task *Task) {
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
                    task.SetTaskState(taskRedirected)
                    fmt.Printf("Task %v sent to worker %v\n", task.id, currentIndex)
                    lastWorkerIndex = (currentIndex + 1) % (workerId + 1)
                    return
                default:
                    continue
                }
            }
        }
        attempts++
        time.Sleep(100 * time.Millisecond)
    }
    
    errorChan <- fmt.Errorf("no available workers for task %d", task.id)
    task.SetTaskState(taskFree)
}

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

func errorHandler() {
	for {
		err := <-errorChan
		log.Println(err)
	}
}

// Maybe it's worth getting rid of this...
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

// taskData is a stub for input data.
func ClusterInit(taskData [][]byte) {
    go workerInit()
    go errorHandler()

    fmt.Println("Waiting for at least one worker to connect...")
    for {
        if hasActiveWorkers() {
            break
        }
        time.Sleep(1 * time.Second)
    }

    for i := range taskData {
        task := NewTask(genTaskId(), taskData[i])
        tasks.Store(i, task)
    }

    go taskManager()

    for {
        time.Sleep(1 * time.Second)
    }
}